// ============================================================================
// TerrainRender.inc (D3D9Client)
//
// Terrain/shadow rendering features that are kept outside the official sources.
// The file is included by the official .cpp files at the hook points; a compile
// time selector decides which part of the file belongs to which translation unit:
//
//   VVessel.cpp : #define TERRAINRENDER_VESSEL  -> legacy projected ground shadow
//   Scene.cpp   : #define TERRAINRENDER_SCENE   -> unified scene shadow system (G1)
//   VBase.cpp   : #define TERRAINRENDER_BASE    -> base structure shadow casters
//
// Current features:
//  1. Legacy projected ground shadow: the reference plane is taken from the
//     surface that is ACTUALLY rendered (base surface if the vessel is over a
//     base, otherwise the renderer terrain elevation), not a mean-sphere tangent
//     plane. This fixes floating/misplaced ground shadows on high resolution
//     terrain. The plane is cached per vessel.
//  2. Vessel shadow mode tags (oapiGetVesselShadowMode): skip the legacy path
//     for objects that are rendered into the shadow map (wreck debris), and for
//     objects that must not cast any ground shadow.
//  3. Unified scene shadow system (G1, "scene shadow" group): one rendering
//     algorithm for every caster type (terrain tiles, vessels, wreck debris,
//     base structures) with one group configuration. The casters are the
//     geometry that is actually rendered - no projection LOD - so the shadow
//     map geometry matches the rendered geometry and shadows can not drift or
//     leak due to a LOD mismatch.
//     - no separate source file: map resources, terrain caster shader, per frame
//       generation and the Scene/VBase hooks all live here.
//     - receivers: terrain (NewPlanet.hlsl) and base/vessel meshes (Vessel.fx /
//       PBR.fx via Common.hlsl). The near field vessel self shadow (G0,
//       Scene::RenderShadowMap) keeps its existing implementation/resolution.
//     - shadow map depth convention matches the near field map: value = 1 - z/w.
//     - the scene shadow map caster shader entries (ShadowVS/ShadowPS) are kept
//       in NewPlanet.hlsl to avoid an additional shader file.
// ============================================================================
#ifndef D3D9CLIENT_TERRAINRENDER_INC
#define D3D9CLIENT_TERRAINRENDER_INC


// ============================================================================
// PART 1: legacy projected ground shadow (VVessel.cpp)
// ============================================================================
#if defined(TERRAINRENDER_VESSEL)

// Returns true if the legacy projected ground shadow should be skipped.
static bool TerrainRender_SkipGroundShadow (OBJHANDLE hVessel)
{
	// Unified scene shadow system: when the scene shadow map (G1) is enabled the
	// vessel shadows are rendered into the shadow map and received by the
	// terrain/base shaders. The legacy projected path is only used as a fallback
	// (config TerrainShadowLegacy) to avoid double shadows.
	if (Config->TerrainSelfShadow != 0 && Config->ShadowMapMode != 0 && Config->TerrainShadowLegacy == 0)
		return true;

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


#endif // TERRAINRENDER_VESSEL


// ============================================================================
// PART 2: unified scene shadow system (G1) - Scene.cpp
// ============================================================================
#if defined(TERRAINRENDER_SCENE)

#include <d3d9.h>
#include <d3dx9.h>
#include "Tilemgr2.h"
#include "Surfmgr2.h"
#include "Spherepatch.h"
#include "D3D9Effect.h"
#include "VBase.h"
#include "D3D9Config.h"

struct VBMESH;
class ShaderClass;

// ---------------------------------------------------------------------------
// Scene shadow group (G1) resource / shader management
// ---------------------------------------------------------------------------

class ShadowSystem {
public:
	ShadowSystem();
	~ShadowSystem();

	bool Init(LPDIRECT3DDEVICE9 pDev);
	void Exit();

	bool IsEnabled() const;
	bool IsReady() const { return pShadowMap != NULL; }

	LPDIRECT3DTEXTURE9 GetMap() const { return pShadowMap; }
	LPDIRECT3DSURFACE9 GetRT() const { return pRTSurface; }
	LPDIRECT3DSURFACE9 GetDS() const { return pDSSurface; }
	int  GetSize() const { return mapSize; }
	int  GetUpdateInterval() const { return updateInterval; }

	// Makes sure the shadow map is created with the requested resolution. The
	// terrain receiver binds to the map size, so this is used to follow the
	// terrain render resolution (back buffer size).
	bool EnsureSize(int size);

	// Terrain caster drawing (called from the shadow pass in Scene::RenderSceneShadow)
	void BeginTerrainCast(const D3DXMATRIX &mViewProj);
	void DrawTerrainTile(VBMESH *pMesh, const D3DXMATRIX &mWorld);
	void EndTerrainCast();

private:
	struct VSConst {
		D3DXMATRIX mVP;
		D3DXMATRIX mW;
	} vsConst;

	LPDIRECT3DDEVICE9   pDev;
	LPDIRECT3DTEXTURE9  pShadowMap;
	LPDIRECT3DSURFACE9  pRTSurface;
	LPDIRECT3DSURFACE9  pDSSurface;
	class ShaderClass  *pTerrainShader;
	HANDLE              hVSC;
	int                 mapSize;
	int                 updateInterval;
	bool                bTerrainCastFirst;
};


// ---------------------------------------------------------------------------
// ShadowSystem (G1) resource / shader management
// ---------------------------------------------------------------------------

ShadowSystem::ShadowSystem()
{
	pDev = NULL;
	pShadowMap = NULL;
	pRTSurface = NULL;
	pDSSurface = NULL;
	pTerrainShader = NULL;
	hVSC = NULL;
	mapSize = 0;
	updateInterval = 1;
	bTerrainCastFirst = true;
	memset(&vsConst, 0, sizeof(vsConst));
}

ShadowSystem::~ShadowSystem()
{
	Exit();
}

bool ShadowSystem::IsEnabled() const
{
	if (!Config) return false;
	if (Config->TerrainSelfShadow == 0) return false;
	if (Config->ShadowMapMode == 0) return false;
	return (pShadowMap != NULL);
}

bool ShadowSystem::Init(LPDIRECT3DDEVICE9 dev)
{
	pDev = dev;

	if (!Config) return false;
	if (Config->TerrainSelfShadow == 0) return false;
	if (Config->ShadowMapMode == 0) return false;

	mapSize = Config->TerrainShadowSize;
	if (mapSize <= 0) mapSize = Config->ShadowMapSize;
	if (mapSize < 512) mapSize = 512;
	if (mapSize > 4096) mapSize = 4096;

	updateInterval = Config->TerrainShadowUpdate;
	if (updateInterval < 1) updateInterval = 1;
	if (updateInterval > 8) updateInterval = 8;

	HR(pDev->CreateTexture(mapSize, mapSize, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &pShadowMap, NULL));
	HR(pShadowMap->GetSurfaceLevel(0, &pRTSurface));
	HR(pDev->CreateDepthStencilSurface(mapSize, mapSize, D3DFMT_D24X8, D3DMULTISAMPLE_NONE, 0, true, &pDSSurface, NULL));

	// the shadow map caster shader entries (ShadowVS/ShadowPS) are kept in
	// NewPlanet.hlsl to avoid an additional shader file
	pTerrainShader = new ShaderClass(pDev, "Modules/D3D9Client/NewPlanet.hlsl", "ShadowVS", "ShadowPS", "TerrainShadow", "");
	if (pTerrainShader) hVSC = pTerrainShader->GetVSHandle("shd_const");

	oapiWriteLogV("ShadowSystem: scene shadow map %dx%d (update interval %d)", mapSize, mapSize, updateInterval);

	return (pShadowMap != NULL);
}

void ShadowSystem::Exit()
{
	SAFE_DELETE(pTerrainShader);
	SAFE_RELEASE(pRTSurface);
	SAFE_RELEASE(pDSSurface);
	SAFE_RELEASE(pShadowMap);
	hVSC = NULL;
	mapSize = 0;
}

bool ShadowSystem::EnsureSize(int size)
{
	if (!pDev) return false;
	if (size < 512) size = 512;
	if (size > 4096) size = 4096;
	if (size == mapSize && pShadowMap) return true;

	SAFE_RELEASE(pRTSurface);
	SAFE_RELEASE(pDSSurface);
	SAFE_RELEASE(pShadowMap);
	mapSize = size;

	HR(pDev->CreateTexture(mapSize, mapSize, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &pShadowMap, NULL));
	if (!pShadowMap) return false;
	HR(pShadowMap->GetSurfaceLevel(0, &pRTSurface));
	HR(pDev->CreateDepthStencilSurface(mapSize, mapSize, D3DFMT_D24X8, D3DMULTISAMPLE_NONE, 0, true, &pDSSurface, NULL));

	oapiWriteLogV("ShadowSystem: scene shadow map resized to %dx%d", mapSize, mapSize);
	return (pShadowMap != NULL && pRTSurface != NULL && pDSSurface != NULL);
}

void ShadowSystem::BeginTerrainCast(const D3DXMATRIX &mViewProj)
{
	if (!pTerrainShader || !pDev) return;
	vsConst.mVP = mViewProj;
	memset(&vsConst.mW, 0, sizeof(vsConst.mW));
	bTerrainCastFirst = true;
	HR(pDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
}

void ShadowSystem::DrawTerrainTile(VBMESH *pMesh, const D3DXMATRIX &mWorld)
{
	if (!pMesh || !pMesh->pVB || !pMesh->pIB || pMesh->nv == 0) return;
	if (bTerrainCastFirst) {
		pTerrainShader->ClearTextures();
		pTerrainShader->Setup(pPatchVertexDecl, true, 0);
		bTerrainCastFirst = false;
	}
	pDev->SetStreamSource(0, pMesh->pVB, 0, sizeof(VERTEX_2TEX));
	pDev->SetIndices(pMesh->pIB);
	vsConst.mW = mWorld;
	pTerrainShader->SetVSConstants(hVSC, &vsConst, sizeof(vsConst));
	pDev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, pMesh->nv, 0, pMesh->nf);
}

void ShadowSystem::EndTerrainCast()
{
	if (!pDev) return;
	HR(pDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW));
}


// ---------------------------------------------------------------------------
// Coverage collection: walk the current tile render tree and merge the screen
// visible tiles into a bounding box. This way the shadow map covers exactly the
// terrain that is rendered this frame (same tiles, same visible range). Tiles
// that are not visible in the camera are skipped, but visible "Active" tiles are
// descended into their child trees (the children are the rendered ones).
// ---------------------------------------------------------------------------

static void SceneShadowCoverageNode(const QuadTreeNode<SurfTile> *node, Scene *scn,
	D3DXVECTOR3 &bmin, D3DXVECTOR3 &bmax, bool &any)
{
	if (!node || !scn) return;
	SurfTile *tile = node->Entry();
	if (!tile) return;
	if ((tile->State() & TILE_VALID) == 0) return;
	// inactive tiles are only preloaded by the terrain LOD preloading and are not
	// part of the rendered representation: ignore them for the coverage bounds
	if (tile->State() == Tile::Inactive) return;

	VBMESH *pMesh = tile->GetMesh();
	bool bVisible = true;

	if (pMesh) {
		D3DXMATRIX mWorld;
		tile->GetWorldMatrix(&mWorld);
		D3DXVECTOR3 bc;
		D3DXVec3TransformCoord(&bc, &pMesh->bsCnt, &mWorld);
		bVisible = scn->IsVisibleInCamera(&bc, pMesh->bsRad);
		if (bVisible) {
			float r = pMesh->bsRad;
			bmin.x = min(bmin.x, bc.x - r); bmin.y = min(bmin.y, bc.y - r); bmin.z = min(bmin.z, bc.z - r);
			bmax.x = max(bmax.x, bc.x + r); bmax.y = max(bmax.y, bc.y + r); bmax.z = max(bmax.z, bc.z + r);
			any = true;
		}
	}

	if (tile->State() == Tile::Active) {
		if (bVisible || !pMesh) {
			for (int i = 0; i < 4; i++) {
				const QuadTreeNode<SurfTile> *child = node->Child(i);
				if (child) SceneShadowCoverageNode(child, scn, bmin, bmax, any);
			}
		}
	}
}


// ---------------------------------------------------------------------------
// Terrain caster collection: walk the current tile render tree and draw every
// tile that intersects the shadow coverage sphere. Rendered tiles (ForRender)
// and off-screen but loaded tiles (Invisible) are used directly, Active tiles
// are descended into their child trees.
// ---------------------------------------------------------------------------

static void SceneShadowTerrainNode(const QuadTreeNode<SurfTile> *node, ShadowSystem *pSys, const D3DXVECTOR3 &center, float radius, int *pnTiles)
{
	if (!node) return;
	SurfTile *tile = node->Entry();
	if (!tile) return;
	if ((tile->State() & TILE_VALID) == 0) return;
	// inactive tiles are only preloaded by the terrain LOD preloading and are not
	// part of the rendered representation: they are not shadow casters
	if (tile->State() == Tile::Inactive) return;

	VBMESH *pMesh = tile->GetMesh();
	D3DXMATRIX mWorld;

	if (pMesh) {
		tile->GetWorldMatrix(&mWorld);
		D3DXVECTOR3 bc;
		D3DXVec3TransformCoord(&bc, &pMesh->bsCnt, &mWorld);
		D3DXVECTOR3 d = bc - center;
		if (D3DXVec3Length(&d) > (radius + pMesh->bsRad)) return;	// coverage sphere miss
	}

	if (tile->State() == Tile::Active) {
		bool any = false;
		for (int i = 0; i < 4; i++) {
			const QuadTreeNode<SurfTile> *child = node->Child(i);
			if (child) { any = true; SceneShadowTerrainNode(child, pSys, center, radius, pnTiles); }
		}
		if (!any && pMesh) {	// safety: no children but flagged active
			pSys->DrawTerrainTile(pMesh, mWorld);
			(*pnTiles)++;
		}
		return;
	}

	// ForRender / Invisible: this tile is a leaf of the rendered representation
	if (pMesh) {
		pSys->DrawTerrainTile(pMesh, mWorld);
		(*pnTiles)++;
	}
}


// ---------------------------------------------------------------------------
// Scene::UpdateEffectShadows
//
// Hands the shadow group parameters over to the D3D9Effect receivers (vessel
// hull self shadowing G0, base surfaces / base buildings / vessels G0+G1).
// Replaces the inline parameter setup that used to live in vPlanet::Render().
// ---------------------------------------------------------------------------

void Scene::UpdateEffectShadows(vPlanet *vp)
{
	if (!D3D9Effect::FX) return;

	const bool bMainScene = (GetRenderPass() == RENDERPASS_MAINSCENE);

	// ---- G0 near-field vessel self shadow (existing behaviour) -----------------
	bool bG0 = (smap.pShadowMap != NULL) && bMainScene && (Config->TerrainShadowing == 2) && (Config->ShadowMapMode != 0);
	if (bG0) {
		if (!((GetCameraAltitude() < 10e3) || (vp && vp->IsMesh()))) bG0 = false;
	}

	if (bG0) {
		float s = float(smap.size);
		float is = 1.0f / s;
		float qw = 1.0f / float(Config->ShadowMapSize);
		HR(D3D9Effect::FX->SetMatrix(D3D9Effect::eLVP, &smap.mViewProj));
		HR(D3D9Effect::FX->SetTexture(D3D9Effect::eShadowMap, smap.pShadowMap));
		HR(D3D9Effect::FX->SetVector(D3D9Effect::eSHD, ptr(D3DXVECTOR4(s, is, qw, 0))));
		HR(D3D9Effect::FX->SetBool(D3D9Effect::eShadowToggle, true));
	}
	else {
		HR(D3D9Effect::FX->SetBool(D3D9Effect::eShadowToggle, false));
	}

	// ---- G1 scene shadow (terrain self shadowing, distant vessel/base shadows) -
	bool bG1 = smapTValid && (smapT.pShadowMap != NULL) && pShadows && pShadows->IsEnabled() && bMainScene;
	if (bG1) {
		float s = float(smapT.size);
		float is = 1.0f / s;
		float texel = 2.0f * smapT.rad * is;
		float idepth = (smapT.depth > 1.0f ? 1.0f / smapT.depth : 0.0f);
		HR(D3D9Effect::FX->SetMatrix(D3D9Effect::eLVP2, &smapT.mViewProj));
		HR(D3D9Effect::FX->SetTexture(D3D9Effect::eShadowMap2, smapT.pShadowMap));
		HR(D3D9Effect::FX->SetVector(D3D9Effect::eSHD2, ptr(D3DXVECTOR4(texel, is, idepth, 0))));
		HR(D3D9Effect::FX->SetVector(D3D9Effect::eSHDT2, ptr(D3DXVECTOR4(smapT.pos.x, smapT.pos.y, smapT.pos.z, smapT.rad))));
	}
	else {
		// coverage radius 0 = disabled (checked by the shader, saves a bool constant)
		HR(D3D9Effect::FX->SetVector(D3D9Effect::eSHDT2, ptr(D3DXVECTOR4(0, 0, 0, 0))));
	}
}


// ---------------------------------------------------------------------------
// Scene::RenderSceneShadow
//
// Generates the scene shadow map (G1). Called from the terrain tile manager
// right after the tile tree has been processed (so the tile world matrices
// are up to date) and before the terrain is rendered.
// ---------------------------------------------------------------------------

void Scene::RenderSceneShadow(vPlanet *vp)
{
	if (vp == NULL) return;
	if (!pShadows) return;
	if (!pShadows->IsReady()) return;
	if (!pShadows->IsEnabled()) return;
	if (GetRenderPass() != RENDERPASS_MAINSCENE) return;
	if (Camera.vProxy != vp) return;
	if (Camera.hObj_proxy == NULL) return;

	const int interval = pShadows->GetUpdateInterval();
	const bool bUpdate = (interval <= 1) || ((dwFrameId % (DWORD)interval) == 0);

	if (!bUpdate && smapTValid) return;	// keep the previous map this frame

	smapTValid = false;

	// ---- resolution -----------------------------------------------------------
	// The shadow map resolution follows the terrain render resolution: by default
	// it matches the back buffer size (next power of two), so the terrain is
	// rendered into the shadow map at the same pixel density as on the screen.
	{
		int want = Config->TerrainShadowSize;
		if (want <= 0) {
			LPDIRECT3DSURFACE9 pBack = gc->GetBackBuffer();
			D3DSURFACE_DESC desc;
			if (pBack && SUCCEEDED(pBack->GetDesc(&desc))) {
				int m = (int)(desc.Width > desc.Height ? desc.Width : desc.Height);
				want = 1024;
				while (want < m) want <<= 1;
			}
		}
		if (want > 0) pShadows->EnsureSize(want);
	}

	// ---- sunlight direction / caster orientation ------------------------------
	D3DXVECTOR3 ld = sunLight.Dir;			// light propagation direction
	VECTOR3 gpos, upv3;
	oapiGetGlobalPos(Camera.hObj_proxy, &gpos);
	upv3 = Camera.pos - gpos;
	double dn = length(upv3);
	if (dn < 1.0) return;
	upv3 /= dn;

	// the sun must be above the horizon: without direct sunlight there is no shadow
	VECTOR3 ldv = _V(ld.x, ld.y, ld.z);
	if (dotp(-ldv, upv3) < 0.03) return;

	double alt = Camera.alt_proxy;
	if (alt > double(Config->TerrainShadowMaxAlt)) return;
	if (alt < -200.0) return;

	// ---- coverage -------------------------------------------------------------
	// The shadow map covers exactly the terrain that is rendered this frame: the
	// screen visible terrain tiles are collected from the same render tree that
	// the terrain renderer is using and merged into a bounding box. When no
	// visible tile can be found (e.g. camera in space) the altitude based
	// fallback is used. A fixed coverage radius from the config overrides both.
	VECTOR3 ctr;
	float R = float(Config->TerrainShadowRange);

	if (R <= 0.0f) {
		D3DXVECTOR3 bmin(1e30f, 1e30f, 1e30f), bmax(-1e30f, -1e30f, -1e30f);
		bool any = false;
		if (vp->SurfMgr2()) {
			for (int i = 0; i < 2; i++) {
				const QuadTreeNode<SurfTile> *root = vp->SurfMgr2()->TileTree(i);
				if (root) SceneShadowCoverageNode(root, this, bmin, bmax, any);
			}
		}

		if (any) {
			D3DXVECTOR3 c = (bmin + bmax) * 0.5f;
			R = 0.5f * D3DXVec3Length(&(bmax - bmin)) + 500.0f;
			ctr = _V(c.x, c.y, c.z);
		} else {
			R = float(max(2500.0, min(60000.0, alt * 6.0)));
			ctr = upv3 * (Camera.elev - alt);
		}
	} else {
		// fixed radius from the config: centre below the camera, moved along view
		ctr = upv3 * (Camera.elev - alt);
		VECTOR3 vdir = Camera.dir;
		double vd = dotp(vdir, upv3);
		VECTOR3 hdir = vdir - upv3 * vd;
		if (length(hdir) > 0.01) {
			normalise(hdir);
			double shift = min(alt * 2.0, double(R) * 0.4);
			if (shift > 1.0) ctr += hdir * shift;
		}
	}

	R = max(R, 300.0f);

	D3DXVECTOR3 pos = D3DXVECTOR3(float(ctr.x), float(ctr.y), float(ctr.z));

	// ---- ortho projection -----------------------------------------------------
	float H = float(vp->GetMaxElevation() - vp->GetMinElevation());
	H = max(H, 200.0f) + 2000.0f;
	float depth = 2.0f * R + 2.0f * H + 500.0f;
	float dist = -(R + H + 55.0f);

	D3DXVECTOR3 lp = pos + ld * dist;
	D3DXVECTOR3 upv = D3DXVECTOR3(0, 1, 0);
	if (fabs(D3DXVec3Dot(&upv, &ld)) > 0.95f) upv = D3DXVECTOR3(1, 0, 0);
	if (fabs(D3DXVec3Dot(&upv, &ld)) > 0.95f) upv = D3DXVECTOR3(0, 0, 1);

	D3DXMatrixLookAtRH(&smapT.mView, &lp, &pos, &upv);
	D3DXMatrixOrthoOffCenterRH(&smapT.mProj, -R, R, R, -R, 50.0f, 50.0f + depth);
	D3DXMatrixMultiply(&smapT.mViewProj, &smapT.mView, &smapT.mProj);

	smapT.pos = pos;
	smapT.ld = ld;
	smapT.rad = R;
	smapT.dist = dist;
	smapT.depth = depth;
	smapT.lod = 0;
	smapT.size = pShadows->GetSize();
	smapT.pShadowMap = pShadows->GetMap();


	// ---- render the scene shadow map -----------------------------------------
	DWORD tStart = GetTickCount();
	int nTiles = 0;
	int nVessels = 0;
	int nBases = 0;

	// Present the G1 parameters to the vessel/base shadow casters (they use the
	// near field shadow parameters from D3D9Effect while rendering)
	{
		float cs = float(smapT.size);
		float cis = 1.0f / cs;
		float cqw = 1.0f / float(Config->ShadowMapSize);
		HR(D3D9Effect::FX->SetMatrix(D3D9Effect::eLVP, &smapT.mViewProj));
		HR(D3D9Effect::FX->SetTexture(D3D9Effect::eShadowMap, smapT.pShadowMap));
		HR(D3D9Effect::FX->SetVector(D3D9Effect::eSHD, ptr(D3DXVECTOR4(cs, cis, cqw, 0))));
		HR(D3D9Effect::FX->SetBool(D3D9Effect::eShadowToggle, true));
	}

	gc->PushRenderTarget(pShadows->GetRT(), pShadows->GetDS(), RENDERPASS_SHADOWMAP);
	HR(pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0));
	BeginPass(RENDERPASS_SHADOWMAP);
	RecallDefaultState();

	// 1) terrain tiles (rendered representation, no projection LOD)
	pShadows->BeginTerrainCast(smapT.mViewProj);
	if (vp->SurfMgr2()) {
		for (int i = 0; i < 2; i++) {
			const QuadTreeNode<SurfTile> *root = vp->SurfMgr2()->TileTree(i);
			if (root) SceneShadowTerrainNode(root, pShadows, pos, R, &nTiles);
		}
	}
	pShadows->EndTerrainCast();

	// 2) vessels and wreck debris (use the near field renderer, temporarily
	//    presenting the G1 parameters to it)
	SHADOWMAPPARAM smapSave = smap;
	smap = smapT;
	for (auto v : Casters) {
		if (!v) continue;
		if (!v->IsActive()) continue;
		if (oapiGetVesselShadowMode(v->Object()) == SHADOWMODE_NONE) continue;
		if (!v->IntersectShadowVolume()) continue;
		v->Render(pDevice, false);
		nVessels++;
	}
	smap = smapSave;

	// 3) base structures (static objects, culled by the coverage sphere)
	if (vp->GetBaseCount() > 0) {
		for (DWORD i = 0; i < vp->GetBaseCount(); i++) {
			vBase *pb = vp->GetBaseByIndex(i);
			if (!pb) continue;
			if (!pb->IsActive()) continue;
			VECTOR3 bpos;
			oapiGetGlobalPos(pb->Object(), &bpos);
			VECTOR3 brel = bpos - Camera.pos;	// camera relative coordinates
			if (length(brel - ctr) > (double(R) + 5000.0)) continue;
			pb->RenderShadowCasters(smapT.mViewProj);
			nBases++;
		}
	}

	// Frame profiler: number of terrain tiles rendered into the scene shadow map
	Perf_ShadowTiles(nTiles);

	PopPass();
	gc->PopRenderTargets();

	// Restore the main scene render states: the shadow caster pass (especially the
	// vessel rendering with the D3D9Effect techniques) can leave cull/blend/z
	// states behind. Without this the subsequently rendered terrain can disappear
	// completely because it is culled with the wrong winding.
	RecallDefaultState();

	smapTValid = true;


	// hand the fresh parameters to the mesh receivers (base surfaces etc.)
	UpdateEffectShadows(vp);

	// ---- diagnostics ----------------------------------------------------------
	if (Config->DebugLvl > 2) {
		DWORD tEnd = GetTickCount();
		static int nLogCount = 0;
		if (nLogCount++ % 120 == 0) {
			oapiWriteLogV("SceneShadow: R=%.0f size=%d tiles=%d vessels=%d bases=%d time=%u ms",
				R, smapT.size, nTiles, nVessels, nBases, (unsigned)(tEnd - tStart));
		}
	}
}



// ---------------------------------------------------------------------------
// Scene::InitShadowSystem / Scene::ExitShadowSystem
//
// One-time resource creation / release. Declared in Scene.h, called from
// Scene::Initialise() and the Scene destructor.
// ---------------------------------------------------------------------------

void Scene::InitShadowSystem()
{
	if (!pShadows) {
		pShadows = new ShadowSystem();
		pShadows->Init(pDevice);
	}
}

void Scene::ExitShadowSystem()
{
	SAFE_DELETE(pShadows);
}

#endif // TERRAINRENDER_SCENE


// ============================================================================
// PART 3: base structure shadow casters (VBase.cpp)
// ============================================================================
#if defined(TERRAINRENDER_BASE)

void vBase::RenderShadowCasters(D3DXMATRIX mViewProj)
{
	if (!nstructure_as) return;
	if (!structure_as) return;

	for (DWORD i = 0; i < nstructure_as; i++) {
		if (!structure_as[i]) continue;
		if (!structure_as[i]->HasShadow()) continue;
		structure_as[i]->RenderShadowMap(&mWorld, &mViewProj, 0);
	}
}


#endif // TERRAINRENDER_BASE

#endif // D3D9CLIENT_TERRAINRENDER_INC

