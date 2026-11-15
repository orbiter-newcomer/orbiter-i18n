// ============================================================================
// TerrainRender.inc (D3D9Client)
//
// Terrain/shadow rendering features that are kept outside the official
// sources. VVessel.cpp includes this file and only calls into it at the hook
// points, so that the official sources stay close to upstream and all terrain
// related features can be extended here in one place.
//
// Current features:
//  - Projected ground shadow: the reference plane is taken from the surface
//    that is ACTUALLY rendered (flat base surface if the vessel is over a base,
//    otherwise the terrain elevation from the renderer's data), instead of a
//    mean-sphere tangent plane. This fixes floating/misplaced ground shadows on
//    high-resolution terrain. The plane is cached per vessel.
//  - Vessel shadow mode tags (oapiGetVesselShadowMode): skip the legacy
//    projected ground shadow for objects that are rendered into the shadow map
//    (wreck debris), and for objects that must not cast any ground shadow.
// ============================================================================
#ifndef D3D9CLIENT_TERRAINRENDER_INC
#define D3D9CLIENT_TERRAINRENDER_INC

// Returns true if the legacy projected ground shadow should be skipped.
static bool TerrainRender_SkipGroundShadow (OBJHANDLE hVessel)
{
	int mode = oapiGetVesselShadowMode (hVessel);
	if (mode == SHADOWMODE_NONE) return true;	// no ground shadow at all
	if (mode == SHADOWMODE_SHADOWMAP && Config->ShadowMapMode >= 1 && Config->TerrainShadowing == 2)
		return true;	// covered by the shadow-map path (e.g. wreck debris)
	return false;
}

// Computes (and caches) the reference plane for the projected ground shadow from
// the surface that is actually rendered. The result is cached in the global frame
// and only recomputed when the vessel has moved or the cache has expired, so that
// a parked vessel does not trigger terrain queries in every frame.
void vVessel::UpdateShadowPlane (OBJHANDLE hPlanet)
{
	VECTOR3 vpos;
	oapiGetGlobalPos (Object(), &vpos);
	if (shd_plane_valid && (oapiGetSimTime() - shd_plane_t) <= 2.0 &&
		dist (vpos, shd_plane_pos) <= 1.0) return;	// cache still valid

	VECTOR3 pp;
	oapiGetGlobalPos (hPlanet, &pp);
	double prad = oapiGetSize (hPlanet);
	double surfElev = vessel->GetSurfaceElevation();
	VECTOR3 nrm, pnt;
	bool bBaseSurf = false;
	double vlng, vlat, vrad;
	oapiGlobalToEqu (hPlanet, vpos, &vlng, &vlat, &vrad);
	MATRIX3 Rpl;
	oapiGetRotationMatrix (hPlanet, &Rpl);
	for (DWORD i = 0, n = oapiGetBaseCount (hPlanet); i < n; i++) {
		OBJHANDLE hb = oapiGetBaseByIndex (hPlanet, i);
		if (!hb) continue;
		// ground distance to the base centre: official great-circle distance
		double blng, blat;
		oapiGetBaseEquPos (hb, &blng, &blat);
		double bdist = oapiOrthodome (vlng, vlat, blng, blat) * prad;
		if (bdist < oapiGetSize (hb)) {
			// flat base surface plane (runway/pad): normal is the base local vertical
			VECTOR3 bp;
			oapiGetGlobalPos (hb, &bp);
			VECTOR3 brel = bp - pp;
			double br = length (brel);
			if (br < 1.0) continue;
			bBaseSurf = true;
			surfElev = max (surfElev, br - prad);
			oapiEquToGlobal (hPlanet, vlng, vlat, prad + surfElev, &pnt);
			nrm = brel / br;
			break;
		}
	}
	if (!bBaseSurf) {
		// terrain surface: elevation from the renderer's terrain data, normal from
		// the official surface-elevation API (returned in the local horizon frame)
		double el0 = surfElev, e;
		bool bOk = false;
		vPlanet *vP = static_cast<vPlanet*>(scn->GetVisObject (hPlanet));
		if (vP) {
			int r0 = vP->GetElevation (vlng, vlat, &e);
			if (r0 > 0) { el0 = e; bOk = true; }
		}
		if (!bOk) el0 = oapiSurfaceElevation (hPlanet, vlng, vlat);
		surfElev = el0;
		VECTOR3 nml;
		nml.x = nml.y = nml.z = 0.0;
		oapiSurfaceElevationEx (hPlanet, vlng, vlat, 0, 0, &nml);
		double slng = sin (vlng), clng = cos (vlng);
		double slat = sin (vlat), clat = cos (vlat);
		VECTOR3 n_pf;
		n_pf.x = -slng*nml.x + clat*clng*nml.y - slat*clng*nml.z;
		n_pf.y =            + slat*nml.y        + clat*nml.z;
		n_pf.z =  clng*nml.x + clat*slng*nml.y - slat*slng*nml.z;
		nrm = mul (Rpl, n_pf);
		VECTOR3 vdir = vpos - pp;
		normalise (vdir);
		if (dotp (nrm, vdir) < 0.0) nrm = -nrm;	// plane normal must point upwards
		oapiEquToGlobal (hPlanet, vlng, vlat, prad + el0, &pnt);
	}
	shd_plane_valid = true;
	shd_plane_t     = oapiGetSimTime();
	shd_plane_pos   = vpos;
	shd_plane_nrm   = nrm;
	shd_plane_pnt   = pnt;
	shd_plane_elev  = surfElev;
}

// Projected ground shadow of the vessel onto the actually rendered surface.
void vVessel::RenderGroundShadow (LPDIRECT3DDEVICE9 dev, OBJHANDLE hPlanet, float alpha)
{
	if (!bStencilShadow && scn->GetRenderPass() == RENDERPASS_MAINSCENE) return;
	if (Config->TerrainShadowing == 0) return;
	if (TerrainRender_SkipGroundShadow (Object())) return;

	static const double eps = 1e-2;
	static const double shadow_elev_limit = 0.07;

	VECTOR3 pp, sd, pvr, vpos;
	oapiGetGlobalPos (hPlanet, &pp);	// planet global pos
	vessel->GetGlobalPos (vpos);	// vessel global pos
	pvr = vpos - pp;	// planet-relative vessel position
	double d = length (pvr);	// vessel-planet distance
	double prad = oapiGetSize (hPlanet);	// planet mean radius

	UpdateShadowPlane (hPlanet);	// cached reference plane of the rendered surface

	double R   = prad + shd_plane_elev;
	double alt = d - R;	// altitude above the reference surface
	if (alt*eps > vessel->GetSize()) return;	// too high to cast a shadow

	sd = vpos;	// shadow projection direction = local solar direction
	normalise (sd);

	// calculate the intersection of the vessel's shadow with the planet surface
	double fac1 = dotp (sd, pvr);
	if (fac1 > 0.0) return;	// shadow doesn't intersect planet surface
	double csun = -fac1 / d;	// sun elevation above horizon
	if (csun < shadow_elev_limit) return;	// sun too low to cast shadow

	MATRIX3 vR;
	vessel->GetRotationMatrix (vR);
	VECTOR3 sdv = tmul (vR, sd);	// projection direction in vessel frame

	// ---- build the shadow projection reference plane (in vessel frame) ----
	VECTOR3 hn = tmul (vR, shd_plane_nrm);	// plane normal
	VECTOR3 pnt = shd_plane_pnt, l0;
	vessel->Global2Local (pnt, l0);	// plane reference point
	float nr0 = float(dotp (hn, l0));	// plane offset: dot(hn, x) = nr0

	double nd = dotp (hn, sdv);
	if (nd > -0.01) return;	// sun too low relative to the local reference plane
	VECTOR3 sdvs = sdv / nd;

	D3DXVECTOR4 nrml = D3DXVECTOR4 (float(hn.x), float(hn.y), float(hn.z), -nr0);

	// build shadow projection matrix
	D3DXMATRIX mProj, mProjWorld, mProjWorldShift;

	mProj._11 = 1.0f - (float)(sdvs.x*hn.x);
	mProj._12 = -(float)(sdvs.y*hn.x);
	mProj._13 = -(float)(sdvs.z*hn.x);
	mProj._14 = 0;
	mProj._21 = -(float)(sdvs.x*hn.y);
	mProj._22 = 1.0f - (float)(sdvs.y*hn.y);
	mProj._23 = -(float)(sdvs.z*hn.y);
	mProj._24 = 0;
	mProj._31 = -(float)(sdvs.x*hn.z);
	mProj._32 = -(float)(sdvs.y*hn.z);
	mProj._33 = 1.0f - (float)(sdvs.z*hn.z);
	mProj._34 = 0;
	mProj._41 = (float)(sdvs.x*nr0);
	mProj._42 = (float)(sdvs.y*nr0);
	mProj._43 = (float)(sdvs.z*nr0);
	mProj._44 = 1;

	D3DXMatrixMultiply (&mProjWorld, &mProj, &mWorld);

	float scale = (csun - shadow_elev_limit) * 25.0f;
	alpha = (1.0f - alpha) * saturate (scale);

	// project all vessel meshes. This should be replaced by a dedicated shadow mesh
	for (UINT i = 0; i<nmesh; i++) {

		if (meshlist[i].mesh == NULL) continue;
		if (!(meshlist[i].vismode & MESHVIS_EXTERNAL)) continue;	// only externally visible meshes
		if (meshlist[i].mesh->HasShadow() == false) continue;

		D3D9Mesh *mesh = meshlist[i].mesh;

		if (meshlist[i].trans) {
			VECTOR3 of;
			vessel->GetMeshOffset (i, of);
			nrml.w += float(dotp (of, hn));	// sift a local ground level
			D3DXMatrixMultiply (&mProjWorldShift, meshlist[i].trans, &mProjWorld);
			mesh->RenderStencilShadows (alpha, &mWorld, &mProjWorldShift, false, &nrml);
		}
		else mesh->RenderStencilShadows (alpha, &mWorld, &mProjWorld, false, &nrml);
	}
}

#endif // D3D9CLIENT_TERRAINRENDER_INC
