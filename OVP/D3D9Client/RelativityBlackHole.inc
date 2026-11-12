// ==============================================================
// RelativityBlackHole.inc
// Black-hole (strong-field body) rendering extension for vStar.
//
// Included by VStar.cpp. Defines the vStar::RelativityRender() member and the
// procedural event-horizon glow texture. A body whose Schwarzschild radius is
// comparable to or larger than its visual size (black holes; for ordinary
// stars rs is many orders of magnitude smaller than the photosphere radius) is
// rendered as a semi-transparent glowing sphere instead of the solar disc.
// ==============================================================

static LPDIRECT3DTEXTURE9 s_bhGlowTex = NULL;   // black hole: warm orange ring
static LPDIRECT3DTEXTURE9 s_nsGlowTex = NULL;   // neutron star: blue-white ring

// Creates the event-horizon glow texture. bNeutron selects the colour palette:
// black holes use a warm orange glow, neutron stars a blue-white one.
static LPDIRECT3DTEXTURE9 RelativityCreateCompactGlowTexture (LPDIRECT3DDEVICE9 dev, bool bNeutron)
{
	const int S = 256;
	LPDIRECT3DTEXTURE9 tex = NULL;
	if (FAILED(D3DXCreateTexture (dev, S, S, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex)))
		return NULL;
	D3DLOCKED_RECT lr;
	if (FAILED(tex->LockRect (0, &lr, NULL, 0))) { tex->Release(); return NULL; }
	for (int y = 0; y < S; y++) {
		DWORD *row = (DWORD*)((BYTE*)lr.pBits + y*lr.Pitch);
		for (int x = 0; x < S; x++) {
			double dx = (x+0.5)/S*2.0 - 1.0;
			double dy = (y+0.5)/S*2.0 - 1.0;
			double r = sqrt (dx*dx + dy*dy);
			double a = 0.0;
			if (r < 0.72) a = 0.06;                              // dim centre
			else if (r < 1.0) a = 0.06 + 0.80*pow((r-0.72)/0.28, 2.0); // horizon ring
			if (r > 0.97) a *= max(0.0, (1.0 - r)/0.03);          // soft outer edge
			if (a < 0.0) a = 0.0; if (a > 1.0) a = 1.0;
			BYTE A = (BYTE)(a*255.0);
			// glow colour, slightly brighter at the ring
			double k = (a > 0.06) ? (a-0.06)/0.8 : 0.0;
			BYTE R, G, B;
			if (bNeutron) {
				// blue-white neutron star
				R = (BYTE)(150.0 + 80.0*k);
				G = (BYTE)(200.0 + 45.0*k);
				B = 255;
			} else {
				// warm orange black hole
				R = 255;
				G = (BYTE)(120.0 + 90.0*k);
				B = (BYTE)(30.0 + 50.0*k);
			}
			row[x] = D3DCOLOR_ARGB (A, R, G, B);
		}
	}
	tex->UnlockRect(0);
	return tex;
}

static void RelativityBlackHoleCleanup ()
{
	if (s_bhGlowTex) { s_bhGlowTex->Release(); s_bhGlowTex = NULL; }
	if (s_nsGlowTex) { s_nsGlowTex->Release(); s_nsGlowTex = NULL; }
}

bool vStar::RelativityRender (LPDIRECT3DDEVICE9 dev)
{
	const double C_LIGHT = 299792458.0;
	const double G_GRAV  = 6.67428e-11;

	double bhMass = oapiGetMass (Object());
	double bhRs   = 2.0*G_GRAV*bhMass/(C_LIGHT*C_LIGHT);
	// Compact objects: black holes (rs >= size) and neutron stars (physical radius
	// of a few km, rs/size ~ 0.3) are rendered as glowing spheres. For an ordinary
	// star rs/size is many orders of magnitude smaller and this test fails.
	if (!(bhRs > 0.05*size)) return false;

	// apparent sphere radius: the Schwarzschild radius for black holes, the
	// physical radius for neutron stars (which is outside their event horizon)
	float rad_scale = float(max(bhRs, size) * relScale);

	VECTOR3 bdir (unit(cpos));
	double hz = std::hypot (bdir.x, bdir.z);
	mWorld._11 =  (FLOAT)bdir.x;
	mWorld._12 =  (FLOAT)bdir.y;
	mWorld._13 =  (FLOAT)bdir.z;
	mWorld._31 = -(FLOAT)(bdir.z/hz);
	mWorld._32 =  0;
	mWorld._33 =  (FLOAT)(bdir.x/hz);
	mWorld._21 =  -(mWorld._12*mWorld._33 - mWorld._32*mWorld._13);
	mWorld._22 =  -(mWorld._13*mWorld._31 - mWorld._33*mWorld._11);
	mWorld._23 =  -(mWorld._11*mWorld._32 - mWorld._31*mWorld._12);

	maxdist = 0.1*scn->GetCameraFarPlane();

	if (cdist > maxdist) {
		double dist_scale = maxdist/cdist;
		rad_scale *= float(dist_scale);
		mWorld._41 = float(cpos.x*dist_scale);
		mWorld._42 = float(cpos.y*dist_scale);
		mWorld._43 = float(cpos.z*dist_scale);
	}

	// scale up sphere radius from 1 to the Schwarzschild radius
	mWorld._11 *= rad_scale; mWorld._12 *= rad_scale; mWorld._13 *= rad_scale;
	mWorld._21 *= rad_scale; mWorld._22 *= rad_scale; mWorld._23 *= rad_scale;
	mWorld._31 *= rad_scale; mWorld._32 *= rad_scale; mWorld._33 *= rad_scale;

	// colour palette: neutron stars are blue-white, black holes warm orange
	char nm[64] = "";
	oapiGetObjectName (Object(), nm, 64);
	bool bNeutron = (strstr(nm, "Neutron") != NULL);
	LPDIRECT3DTEXTURE9 &tex = (bNeutron ? s_nsGlowTex : s_bhGlowTex);
	if (!tex) tex = RelativityCreateCompactGlowTexture (dev, bNeutron);
	if (tex) D3D9Effect::RenderBillboard(&mWorld, tex, 1.0f);
	return true;
}