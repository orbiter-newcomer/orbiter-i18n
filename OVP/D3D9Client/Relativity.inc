// ==============================================================
// Relativity visual effects, phase 1: Doppler shift + aberration.
//
// This file is #included at the end of Scene.cpp. It implements the Scene
// member functions declared in Scene.h, so the official scene rendering code
// only keeps four call sites:
//     Scene::Scene()            -> InitRelativityEffects()
//     Scene::~Scene()           -> ExitRelativityEffects()
//     Scene::RenderMainScene()  -> ApplyRelativityEffects()
//     (Scene.h declares the members; the shader is shaders/Relativity.hlsl)
//
// The pass is inserted into the existing light-glow post-processing chain:
//     scene -> pOffscreenTarget -> GBUF_COLOR -> [Relativity pass] -> GBUF_REL
//           -> LightBlur (samples GBUF_REL) -> back buffer
// It does not create any multisampled target or touch the render target
// setup of the main scene (see HANDOVER_RELATIVITY_ZH.md section 3.2).
// ==============================================================

// --------------------------------------------------------------------------
// Standalone trace log. The D3D9Client HTML log is opened exclusively and
// cannot be read while the simulation is running, so the relativity
// diagnostics use their own plain text file. The file is opened in shared
// mode and without buffering, which makes it readable at any time.
// --------------------------------------------------------------------------
#include <share.h>
#include <cstdarg>

static FILE *s_relLog = NULL;

void RelLog(const char *fmt, ...)
{
	if (!Config->RelLensWireframe) return;   // trace only in debug wireframe mode (avoids per-frame log spam)
	if (!s_relLog) {
		s_relLog = _fsopen("RelativityTrace.log", "w", _SH_DENYNO);
		if (!s_relLog) return;
		setvbuf(s_relLog, NULL, _IONBF, 0);
	}
	va_list ap;
	va_start(ap, fmt);
	char buf[512];
	_vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
	va_end(ap);
	fprintf_s(s_relLog, "%s\n", buf);
}

static const double REL_C_LIGHT = 299792458.0;

// --------------------------------------------------------------------------
// Observer velocity for the relativistic aberration.
// The observer velocity is the velocity of the camera's *tracking target*
// (oapiCameraTarget), i.e. whoever the camera is set to track - for a
// target-relative camera this is also the camera's own velocity. If there is
// no tracking target, the differentiated camera velocity is used as a fallback.
// --------------------------------------------------------------------------
bool Scene::GetRelAberrationVel(VECTOR3 &vel) const
{
	OBJHANDLE hTgt = oapiCameraTarget();
	if (hTgt) {
		oapiGetGlobalVel(hTgt, &vel);
		return true;
	}
	vel = camVel;
	return (sqrt(dotp(vel, vel)) > 1e-3);
}

// --------------------------------------------------------------------------
// Relativistic aberration parameters for renderer-side background rendering
// (star field and celestial sphere background image). The velocity direction
// is returned in the world/global frame, which is the frame the background
// geometry is expressed in.
// --------------------------------------------------------------------------
bool Scene::GetRelativityAberration(float &beta, D3DXVECTOR3 &vdir) const
{
	static int dbgState = -1;		// diagnostic: log state changes only

	beta = 0.0f;
	vdir = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
	if (!Config->RelAberration) {
		if (dbgState != 0) { dbgState = 0; LogAlw("RelAberr: OFF (Config->RelAberration=0)"); }
		return false;
	}

	// Observer velocity: velocity of the body the camera is aiming at (or the
	// differentiated camera velocity if the view centre is empty), see
	// GetRelAberrationVel(). This avoids a per-frame rotating aberration
	// direction that makes bright bodies appear several times (ghosting).
	VECTOR3 ovel;
	if (!GetRelAberrationVel(ovel)) return false;
	double vmag = sqrt(ovel.x*ovel.x + ovel.y*ovel.y + ovel.z*ovel.z);

	beta = (float)min(vmag / REL_C_LIGHT, 0.9999);
	vdir = D3DXVECTOR3((float)ovel.x, (float)ovel.y, (float)ovel.z);
	D3DXVec3Normalize(&vdir, &vdir);
	if (dbgState != 1) { dbgState = 1; LogAlw("RelAberr: ON beta=%.4f", beta); }
	return true;
}

// --------------------------------------------------------------------------
// Collect the celestial bodies used for the gravitational lensing of the star
// field: camera-relative direction, distance and mass parameter
// mu = GM/c^2 (half the Schwarzschild radius of the body).
// --------------------------------------------------------------------------
void Scene::ComputeLensBodies()
{
	relLensOn = 0.0f;
	relLensCount = 0;
	relLensModel = (Config->RelGravityMethod == 1) ? 1.0f : 0.0f;

	if (!Config->RelLensing) return;

	relLensOn = 1.0f;

	const double G = 6.67428e-11;
	const double c2 = 8.9875517873681764e16;

	for (VOBJREC *pv = vobjFirst; pv && relLensCount < REL_MAX_LENS; pv = pv->next) {
		vObject *vo = pv->vobj;
		if (!vo) continue;
		int tp = vo->Type();
		if (tp != OBJTP_STAR && tp != OBJTP_PLANET) continue;
		OBJHANDLE hBody = vo->GetObjectA();
		if (!hBody) continue;

		double mass = oapiGetMass(hBody);
		if (mass <= 0.0) continue;
		double mu = G * mass / c2;
		if (mu < 1e-2) continue;			// negligible deflection source

		VECTOR3 gpos;
		oapiGetGlobalPos(hBody, &gpos);
		VECTOR3 rel = gpos - Camera.pos;
		double dst = length(rel);
		if (dst < 1e3) continue;			// camera inside the body

		LENSBODY &b = relLensBodies[relLensCount++];
		b.dir[0] = (float)(rel.x / dst);
		b.dir[1] = (float)(rel.y / dst);
		b.dir[2] = (float)(rel.z / dst);
		b.dist = (float)min(dst, 1e16);
		b.mu = (float)min(mu, 1e16);

		// Kerr spin parameter and spin axis, same data the physics engine uses
		double aM = 0.0;
		VECTOR3 axis = _V(0, 1, 0);
		if (oapiGetRelativitySpin(hBody, &aM, &axis)) {
			b.aM = (float)min(aM, 1e3);
			b.axis[0] = (float)axis.x;
			b.axis[1] = (float)axis.y;
			b.axis[2] = (float)axis.z;
		}
		else {
			b.aM = 0.0f;
			b.axis[0] = 0.0f; b.axis[1] = 1.0f; b.axis[2] = 0.0f;
		}
	}

	// Diagnostic (sampled)
	{
		static int dbgLens = 0;
		if (dbgLens++ % 300 == 0) {
			RelLog("RelLens: on=%d model=%.0f bodies=%d", (int)relLensOn, relLensModel, relLensCount);
		}
	}
}

// --------------------------------------------------------------------------
// Gravitational lensing: star field cubemap.
//
// The star field (background image + stars) is rendered once into a cubemap
// in the "source frame" (no aberration, no lensing). A full screen pass then
// resolves every screen pixel back to a source direction (inverse aberration
// and inverse light deflection) and samples the cubemap. A cubemap covers all
// directions, so no off-screen data is needed even for strong fields (black
// holes), where the deflection can exceed 90 degrees.
// --------------------------------------------------------------------------

struct STARS_LENS_CONST {
	D3DXMATRIX mVP;
	D3DXVECTOR4 vTan;
	D3DXVECTOR4 vCamX, vCamY, vCamZ;
	D3DXVECTOR4 vVelDir;
	D3DXVECTOR4 fParams;		// x = beta, y = aberration, z = lens count, w = lens model
	D3DXVECTOR4 vLens[12];
	float fLensMu[12];
	D3DXVECTOR4 vLensSpin[12];	// xyz = spin axis, w = Kerr a/M
};

void Scene::InitLensCubemap()
{
	pLensCube = NULL;
	pStarsLens = NULL;
	relLensCubeSize = 0;
	bLensCubeReady = false;
	relLensCubeBrightness = -1.0f;

	int size = Config->LensCubemapSize;
	if (size != 512 && size != 1024 && size != 2048) size = 1024;

	if (D3DXCreateCubeTexture(pDevice, size, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pLensCube) != S_OK) {
		LogErr("Creation of the lensing star field cubemap failed");
		pLensCube = NULL;
		return;
	}
	relLensCubeSize = size;

	pStarsLens = new ShaderClass(pDevice, "Modules/D3D9Client/StarsLens.hlsl", "VSMain", "PSMain", "StarsLens", "");

	bLensCubeReady = true;
	LogAlw("Lens cubemap created: %dx%d x6", size, size);
}

void Scene::ExitLensCubemap()
{
	SAFE_DELETE(pStarsLens);
	SAFE_RELEASE(pLensCube);
	bLensCubeReady = false;
}

// --------------------------------------------------------------------------
// Render the star field (source frame: no aberration, no lensing) into the
// cubemap, six faces.
// --------------------------------------------------------------------------
void Scene::RenderLensCubemap()
{
	if (!bLensCubeReady || !pLensCube || !m_celSphere) return;

	RelLog("LensCube: render start (brightness=%.3f)", m_celSphere->GetSkyBrightnessPublic());

	LPDIRECT3DSURFACE9 pOldRT = NULL, pOldDS = NULL;
	HR(pDevice->GetRenderTarget(0, &pOldRT));
	HR(pDevice->GetDepthStencilSurface(&pOldDS));

	D3DXMATRIX mProj, mView, mVP;
	D3DXMatrixPerspectiveFovLH(&mProj, D3DX_PI * 0.5f, 1.0f, 0.1f, 1e10f);

	static const D3DXVECTOR3 dirs[6] = {
		{1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1} };
	static const D3DXVECTOR3 ups[6] = {
		{0,1,0}, {0,1,0}, {0,0,-1}, {0,0,1}, {0,1,0}, {0,1,0} };

	D3DVIEWPORT9 vp = { 0, 0, (DWORD)relLensCubeSize, (DWORD)relLensCubeSize, 0.0f, 1.0f };

	for (int f = 0; f < 6; f++) {
		LPDIRECT3DSURFACE9 pFace = NULL;
		if (pLensCube->GetCubeMapSurface((D3DCUBEMAP_FACES)f, 0, &pFace) != S_OK) break;

		HR(pDevice->SetRenderTarget(0, pFace));
		HR(pDevice->SetDepthStencilSurface(NULL));
		HR(pDevice->SetViewport(&vp));
		HR(pDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0));

		HR(pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));

		D3DXVECTOR3 eye(0.0f, 0.0f, 0.0f);
		D3DXVECTOR3 at = dirs[f];
		D3DXVECTOR3 up = ups[f];
		D3DXMatrixLookAtLH(&mView, &eye, &at, &up);
		D3DXMatrixMultiply(&mVP, &mView, &mProj);

		VECTOR3 camDir = _V(at.x, at.y, at.z);
		int level = 8;
		RelLog("LensCube: face %d start (level %d)", f, level);
		m_celSphere->RenderBackground(pDevice, &mVP, &camDir, level);
		RelLog("LensCube: face %d done", f);

		pFace->Release();
	}

	HR(pDevice->SetRenderTarget(0, pOldRT));
	HR(pDevice->SetDepthStencilSurface(pOldDS));
	SAFE_RELEASE(pOldRT);
	SAFE_RELEASE(pOldDS);

	D3DVIEWPORT9 vpMain = { 0, 0, (DWORD)viewW, (DWORD)viewH, 0.0f, 1.0f };
	HR(pDevice->SetViewport(&vpMain));
	RecallDefaultState();
	RelLog("LensCube: render done");
}

// --------------------------------------------------------------------------
// Composite the lensing cubemap onto the screen. The pass uses additive
// blending so that an atmospheric sky background (drawn by the scene clear)
// is preserved underneath the star field.
// --------------------------------------------------------------------------
void Scene::CompositeStarField()
{
	if (!bLensCubeReady || !pLensCube || !pStarsLens) return;

	RelLog("Composite: start (bodies=%d)", relLensCount);

	STARS_LENS_CONST c;
	memset(&c, 0, sizeof(c));

	// full screen quad in clip space (independent of the viewport size)
	D3DXMatrixIdentity(&c.mVP);

	// ensure the viewport covers the whole screen (it may have been left
	// smaller by a previous render stage or a device reset)
	D3DVIEWPORT9 vpFull = { 0, 0, (DWORD)viewW, (DWORD)viewH, 0.0f, 1.0f };
	HR(pDevice->SetViewport(&vpFull));
	c.vTan = D3DXVECTOR4(Camera.vw, Camera.vh, 0.0f, 0.0f);
	c.vCamX = D3DXVECTOR4(Camera.x.x, Camera.x.y, Camera.x.z, 0.0f);
	c.vCamY = D3DXVECTOR4(Camera.y.x, Camera.y.y, Camera.y.z, 0.0f);
	c.vCamZ = D3DXVECTOR4(Camera.z.x, Camera.z.y, Camera.z.z, 0.0f);

	// observer velocity (aberration): used by the star field shader
	double beta = 0.0;
	D3DXVECTOR3 vdir(0.0f, 0.0f, 1.0f);
	{
		VECTOR3 ovel;
		if (GetRelAberrationVel(ovel)) {
			double vmag = sqrt(ovel.x*ovel.x + ovel.y*ovel.y + ovel.z*ovel.z);
			beta = min(vmag / REL_C_LIGHT, 0.9999);
			vdir = D3DXVECTOR3((float)ovel.x, (float)ovel.y, (float)ovel.z);
			D3DXVec3Normalize(&vdir, &vdir);
		}
	}
	c.vVelDir = D3DXVECTOR4(vdir.x, vdir.y, vdir.z, 0.0f);

	// lensing bodies
	const LENSBODY *bodies = NULL;
	int n = GetLensBodies(&bodies);
	if (n > REL_MAX_LENS) n = REL_MAX_LENS;
	for (int i = 0; i < REL_MAX_LENS; i++) {
		if (i < n) {
			c.vLens[i] = D3DXVECTOR4(bodies[i].dir[0], bodies[i].dir[1], bodies[i].dir[2], bodies[i].dist);
			c.fLensMu[i] = bodies[i].mu;
			c.vLensSpin[i] = D3DXVECTOR4(bodies[i].axis[0], bodies[i].axis[1], bodies[i].axis[2], bodies[i].aM);
		}
	}
	// aberration and gravitational lensing for the star field
	c.fParams = D3DXVECTOR4((float)beta, Config->RelAberration ? 1.0f : 0.0f, (float)n, relLensModel);

	static const SMVERTEX vtx[4] = {
		{ -1.0f, -1.0f, 0.0f, 0.0f, 1.0f },
		{ -1.0f,  1.0f, 0.0f, 0.0f, 0.0f },
		{  1.0f,  1.0f, 0.0f, 1.0f, 0.0f },
		{  1.0f, -1.0f, 0.0f, 1.0f, 1.0f }
	};
	static const WORD cIndex[6] = { 0, 1, 2, 0, 2, 3 };

	pStarsLens->Setup(pPosTexDecl, false, 0);
	pStarsLens->ClearTextures();
	pStarsLens->SetVSConstants("Const", &c, sizeof(c));
	pStarsLens->SetPSConstants("Const", &c, sizeof(c));

	// equirectangular panorama texture (bound manually, ShaderClass handles
	// only its own textures). Longitude wraps around, so U uses WRAP.
	HR(pDevice->SetTexture(0, pLensCube));
	HR(pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP));
	HR(pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP));
	HR(pDevice->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP));
	HR(pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
	HR(pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
	HR(pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE));

	// additive blending, no depth
	HR(pDevice->SetRenderState(D3DRS_ZENABLE, FALSE));
	HR(pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE));
	HR(pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE));
	HR(pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE));
	HR(pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE));
	HR(pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));
	HR(pDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE));

	HR(pDevice->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 4, 2, cIndex, D3DFMT_INDEX16, vtx, sizeof(SMVERTEX)));

	HR(pDevice->SetTexture(0, NULL));
	RecallDefaultState();
	RelLog("Composite: done");
}

// --------------------------------------------------------------------------
// Relativistic aberration of the celestial bodies (star and planets).
// The star field and the background image are handled by the renderer
// (CelSphere.hlsl / SceneTech.fx). The bodies are displaced here: their
// camera-relative direction is shifted from the rest frame into the observer
// frame while keeping the distance (apparent size) and the body orientation
// unchanged. The aberration of each body uses the observer velocity RELATIVE
// TO THAT BODY, so bodies the observer is co-moving with (e.g. standing on a
// planet) are not displaced.
// --------------------------------------------------------------------------
void Scene::ApplyRelativityToBodies()
{
	// Gravitational lensing data is independent of the aberration switch
	ComputeLensBodies();

	if (!Config->RelAberration) return;

	// Skip the scene initialisation frames: the visuals are not fully set up
	// yet and the object handles may be incomplete.
	if (dwFrameId < 2) return;

	// Diagnostic: list the visual objects once, so the planet handling can be
	// verified (names, types and activation state).
	{
		static bool dbgListed = false;
		if (!dbgListed) {
			dbgListed = true;
			int n = 0;
			for (VOBJREC *pv = vobjFirst; pv && n < 40; pv = pv->next, n++) {
				vObject *vo = pv->vobj;
				char nm[64] = "";
				OBJHANDLE h = vo ? vo->GetObjectA() : NULL;
				if (h) oapiGetObjectName(h, nm, 64);
				RelLog("RelVis: [%d] %s type=%d active=%d",
					n, nm, pv->type, vo ? (int)vo->IsActive() : -1);
			}
			RelLog("RelVis: total recorded %d objects", n);
		}
	}

	// Observer velocity: velocity of the aimed-at body (see
	// GetRelAberrationVel), falling back to the camera velocity. Each body uses
	// the observer velocity relative to that body, so bodies the observer is
	// co-moving with are not displaced.
	VECTOR3 vObs;
	GetRelAberrationVel(vObs);

	for (VOBJREC *pv = vobjFirst; pv; pv = pv->next) {
		vObject *vo = pv->vobj;
		if (!vo) continue;

		int tp = vo->Type();
		if (tp != OBJTP_STAR && tp != OBJTP_PLANET) continue;

		OBJHANDLE hBody = vo->GetObjectA();
		if (!hBody) continue;
		int btp = oapiGetObjectType(hBody);
		if (btp != OBJTP_STAR && btp != OBJTP_PLANET) continue;

		VECTOR3 vBody;
		oapiGetGlobalVel(hBody, &vBody);
		VECTOR3 vrel = vObs - vBody;
		double vm = sqrt(dotp(vrel, vrel));
		double beta = min(vm / REL_C_LIGHT, 0.9999);
		if (beta < 1e-6) continue;                       // effect negligible
		VECTOR3 vdir = vrel / vm;

		// Use the TRUE body position and the camera position for the direction,
		// not vo->PosFromCamera(): the latter is overwritten by this function on
		// every call, so repeated calls within the same frame would displace the
		// body cumulatively and the body appeared at several positions
		// (visual ghosting).
		VECTOR3 gposBody;
		oapiGetGlobalPos(hBody, &gposBody);
		VECTOR3 rel = gposBody - Camera.pos;
		double dist = length(rel);
		if (!(dist > 1.0)) continue;                     // NaN guard / too close
		VECTOR3 dir = rel / dist;

		// aberration: rest frame direction -> observer frame direction
		double c0 = dotp(dir, vdir);
		double gamma = 1.0 / sqrt(max(1e-12, 1.0 - beta * beta));

		double c2 = (c0 + beta) / max(1.0 + beta * c0, 1e-9);
		c2 = max(-1.0, min(1.0, c2));

		VECTOR3 perp = dir - vdir * c0;
		double pl = length(perp);
		if (pl > 1e-12) perp = perp / pl;
		else perp = _V(0.0, 0.0, 0.0);

		VECTOR3 ndir = vdir * c2 + perp * sqrt(max(0.0, 1.0 - c2 * c2));
		double nl = length(ndir);
		if (!(nl > 1e-12)) continue;
		ndir = ndir / nl;

		// Apparent angular size scale: d(theta')/d(theta) == delta, the same
		// factor as the Doppler factor. Bodies in the receding direction appear
		// enlarged, bodies in the approach direction appear shrunk.
		double delta = 1.0 / max(gamma * (1.0 + beta * c0), 1e-6);
		delta = max(0.01, min(100.0, delta));

		vo->RepositionFromCamera(ndir * dist, delta);

		// Diagnostic: report the processed bodies (sampled, readable at runtime)
		{
			static int dbgBodies = 0;
			dbgBodies++;
			if ((dbgBodies % 60) == 1) {
				char nm[64] = "";
				oapiGetObjectName(hBody, nm, 64);
				double angShift = acos(max(-1.0, min(1.0, c2))) - acos(max(-1.0, min(1.0, c0)));
				RelLog("RelBody: %s type=%d beta=%.3f shift=%.2f deg scale=%.3f dist=%.3e",
					nm, btp, beta, angShift * DEG, delta, dist);
			}
		}
	}
}

// --------------------------------------------------------------------------
// Synchronise the cached relativity settings from the Orbiter core visual
// parameters. The switches live in Options -> Visual settings (Orbiter.cfg),
// so that they can be changed without leaving the simulation.
// --------------------------------------------------------------------------
void Scene::SyncRelativityConfig()
{
	if (!gc) return;
	const bool *pPhys = (const bool *)gc->GetConfigParam(CFGPRM_PHYSRELATIVITY);
	const bool *pD = (const bool *)gc->GetConfigParam(CFGPRM_RELDOPPLER);
	const bool *pA = (const bool *)gc->GetConfigParam(CFGPRM_RELABERRATION);
	const bool *pL = (const bool *)gc->GetConfigParam(CFGPRM_RELLENSING);
	const bool *pW = (const bool *)gc->GetConfigParam(CFGPRM_RELLENSWIREFRAME);
	const double *pE = (const double *)gc->GetConfigParam(CFGPRM_RELEXPOSURE);
	const int *pM = (const int *)gc->GetConfigParam(CFGPRM_RELGRAVMETHOD);

	// The visual effects are bound to the main physics relativity switch:
	// when relativity is disabled in the physics engine all visual effects are
	// disabled as well. The per-effect visual switches still allow to turn
	// individual effects off while relativity is enabled.
	bool bPhysRel = (pPhys && *pPhys);
	Config->RelDoppler    = (bPhysRel && pD && *pD) ? 1 : 0;
	Config->RelAberration = (bPhysRel && pA && *pA) ? 1 : 0;
	Config->RelLensing    = (bPhysRel && pL && *pL) ? 1 : 0;
	Config->RelLensWireframe = (pW && *pW);
	if (pE) Config->RelExposureWhite = *pE;
	if (pM) Config->RelGravityMethod = *pM;
}

// --------------------------------------------------------------------------
// Create the visual effect resources. Called unconditionally at the end of
// the Scene constructor (before the psgBuffer[] surface table is built) and
// lazily from ApplyRelativityEffects() when the user enables an effect from
// the Options dialog during the simulation.
// --------------------------------------------------------------------------
void Scene::InitRelativityEffects()
{
	// (initial state also used before InitRelativityEffects is called)
	pRelativity = NULL;
	bRelInitTried = false;
	pLensCube = NULL;
	pStarsLens = NULL;
	relLensCubeSize = 0;
	bLensCubeReady = false;
	relLensCubeBrightness = -1.0f;
	relLensCount = 0;
	relLensOn = 0.0f;
	relLensModel = 0.0f;
	if (pRelativity) return;

	// Sun glare relativistic state (see ComputeRelSunVisual)
	bRelSunValid = false;
	relSunBeam = 1.0f;
	relSunTint = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
	relSunDirVis = _V(0.0, 0.0, 0.0);

	SyncRelativityConfig();

	if (!(Config->RelDoppler || Config->RelAberration)) return;

	// Do not retry the creation every frame if it has already failed.
	if (bRelInitTried) return;
	bRelInitTried = true;

	// The effect relies on the light-glow post-processing path, which already
	// provides the offscreen scene target and the fullscreen colour buffer.
	if (Config->PostProcess != PP_DEFAULT || !pOffscreenTarget || !ptgBuffer[GBUF_COLOR]) {
		LogAlw("Relativity visual effects require Post Processing = Light glow; effects disabled");
		return;
	}

	// Use the same format as the (possibly HDR) colour buffer.
	D3DSURFACE_DESC cdesc;
	ptgBuffer[GBUF_COLOR]->GetLevelDesc(0, &cdesc);

	if (D3DXCreateTexture(pDevice, viewW, viewH, 1, D3DUSAGE_RENDERTARGET, cdesc.Format, D3DPOOL_DEFAULT, &ptgBuffer[GBUF_REL]) != S_OK) {
		LogErr("Creation of relativity colour buffer failed");
		return;
	}

	pRelativity = new ImageProcessing(pDevice, "Modules/D3D9Client/Relativity.hlsl", "PSMain");
	if (!pRelativity->IsOK()) {
		LogErr("Relativity shader failed to initialise");
		SAFE_DELETE(pRelativity);
		return;
	}

	LogAlw("Relativity visual effects enabled [Doppler=%d Aberration=%d]", Config->RelDoppler, Config->RelAberration);
}

// --------------------------------------------------------------------------
// Release the visual effect resources. Called from the Scene destructor.
// --------------------------------------------------------------------------
void Scene::ExitRelativityEffects()
{
	SAFE_DELETE(pRelativity);

	// Release the relativity colour buffer explicitly (it is not owned by the
	// post-processing chain and must not outlive the scene).
	SAFE_RELEASE(psgBuffer[GBUF_REL]);
	SAFE_RELEASE(ptgBuffer[GBUF_REL]);
}

// --------------------------------------------------------------------------
// Per-frame shader constants for Relativity.hlsl.
// Every uniform below is referenced by the shader (an unused uniform would be
// optimised out and every SetFloat() call would log an error).
// Returns false when the pass has nothing to do in the current view.
// --------------------------------------------------------------------------
bool Scene::SetupRelativityPass()
{
	// The camera basis vectors (Camera.x/y/z) are given in the global frame
	// and the rendered scene is oriented accordingly, so the view direction of
	// a pixel can be reconstructed from the screen uv in the same frame.
	D3DXVECTOR2 vTan = D3DXVECTOR2(Camera.vw, Camera.vh);

	// Velocity of the observer (aimed-at body or camera), see
	// GetRelAberrationVel().
	VECTOR3 gvel;
	GetRelAberrationVel(gvel);
	double vmag = sqrt(gvel.x*gvel.x + gvel.y*gvel.y + gvel.z*gvel.z);
	double beta = min(vmag / REL_C_LIGHT, 0.9999);
	double gamma = 1.0 / sqrt(1.0 - beta*beta);

	// Velocity direction expressed in the camera frame.
	D3DXVECTOR3 vdir(0.0f, 0.0f, 1.0f);
	if (vmag > 1e-6) {
		vdir.x = float(gvel.x * Camera.x.x + gvel.y * Camera.x.y + gvel.z * Camera.x.z);
		vdir.y = float(gvel.x * Camera.y.x + gvel.y * Camera.y.y + gvel.z * Camera.y.z);
		vdir.z = float(gvel.x * Camera.z.x + gvel.y * Camera.z.y + gvel.z * Camera.z.z);
		D3DXVec3Normalize(&vdir, &vdir);
	}

	float fDop = (Config->RelDoppler    ? 1.0f : 0.0f);
	// Aberration is handled by the renderer now (star field and celestial
	// sphere background); the screen-space path keeps it disabled.
	float fAb  = 0.0f;

	// Doppler may be suppressed in the cockpit/internal view: the cockpit
	// geometry is at rest with respect to the observer.
	if (oapiCameraInternal()) {
		if (Config->RelInternalMode == 1 || Config->RelInternalMode == 2) fDop = 0.0f;
	}
	if (fDop <= 0.0f && fAb <= 0.0f) return false;

	pRelativity->SetFloat("vTan", &vTan, sizeof(vTan));
	pRelativity->SetFloat("vVelCam", &vdir, sizeof(vdir));
	pRelativity->SetFloat("fBeta", (float)beta);
	pRelativity->SetFloat("fGamma", (float)gamma);
	pRelativity->SetFloat("fDoppler", fDop);
	pRelativity->SetFloat("fAberr", fAb);
	pRelativity->SetFloat("fExposure", (float)Config->RelExposureWhite);

	// Near-field protection: exclude pixels belonging to the observer's own
	// vessel / cockpit from the effects. Requires the vessel depth buffer,
	// which is only created when sun or local glares are enabled.
	float fGuard = 0.0f;
	if (Config->RelNearGuard > 0.0 && ptgBuffer[GBUF_DEPTH]) fGuard = (float)Config->RelNearGuard;
	pRelativity->SetFloat("fNearGuard", fGuard);
	return true;
}

// --------------------------------------------------------------------------
// Sun glare relativistic state.
// The Sun glare sprite (Scene::RenderGlares) is drawn AFTER the relativity
// post-processing pass, so it does not receive the Doppler/aberration effects
// automatically. Here we compute, for the Sun direction:
//   - the aberration-shifted apparent direction (so the glare follows the
//     aberration-displaced Sun disc),
//   - the Doppler colour tint and beaming factor (so the glare dims/reddens
//     in the receding direction and brightens/blues in the approaching one).
// The tint formula mirrors DopplerTint() in Relativity.hlsl.
// --------------------------------------------------------------------------
void Scene::ComputeRelSunVisual()
{
	// The effect is off: clear the state so the glare falls back to the true
	// Sun direction.
	if (!(Config->RelDoppler || Config->RelAberration)) {
		bRelSunValid = false;
		relSunBeam = 1.0f;
		relSunTint = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
		relSunDirVis = _V(0.0, 0.0, 0.0);
		return;
	}

	// On transient failures keep the previous state (a momentary data glitch
	// must not make the aberration-shifted glare pop out of existence).
	OBJHANDLE hSun = oapiGetObjectByIndex(0);
	if (!hSun) return;

	VECTOR3 gsun, vSun, gvel;
	oapiGetGlobalPos(hSun, &gsun);
	oapiGetGlobalVel(hSun, &vSun);
	// Observer velocity (aimed-at body or camera) relative to the Sun.
	GetRelAberrationVel(gvel);
	VECTOR3 vrel = gvel - vSun;

	VECTOR3 sunRel = gsun - Camera.pos;
	double sdst = sqrt(dotp(sunRel, sunRel));
	if (sdst < 1.0) return;
	VECTOR3 usun = sunRel / sdst;

	double vmag = sqrt(dotp(vrel, vrel));
	double beta = min(vmag / REL_C_LIGHT, 0.9999);
	VECTOR3 vdir = (vmag > 1e-6 ? vrel / vmag : _V(0.0, 0.0, 1.0));

	// Aberration: apparent (observed frame) direction of the Sun.
	VECTOR3 uvis = usun;
	if (Config->RelAberration && beta > 1e-6) {
		double cosT = dotp(usun, vdir);
		double cosT2 = (cosT + beta) / max(1.0 + beta * cosT, 1e-9);
		cosT2 = max(-1.0, min(1.0, cosT2));
		VECTOR3 perp = usun - vdir * cosT;
		double pl = sqrt(dotp(perp, perp));
		if (pl > 1e-12) perp = perp / pl;
		uvis = vdir * cosT2 + perp * sqrt(max(0.0, 1.0 - cosT2 * cosT2));
		uvis = uvis / sqrt(dotp(uvis, uvis));
	}
	relSunDirVis = uvis;

	// Doppler factor of the Sun in the observed direction.
	if (Config->RelDoppler && beta > 1e-6) {
		double cosE = dotp(uvis, vdir);
		double gamma = 1.0 / sqrt(1.0 - beta * beta);
		double delta = 1.0 / max(gamma * (1.0 - beta * cosE), 1e-6);
		double d = max(0.1, min(10.0, delta));
		const double xR = 3.68916, xG = 4.02454, xB = 4.91888;
		const double nR = 39.028, nG = 54.868, nB = 135.546;
		double rR = nR / max(exp(xR / d) - 1.0, 1e-6);
		double rG = nG / max(exp(xG / d) - 1.0, 1e-6);
		double rB = nB / max(exp(xB / d) - 1.0, 1e-6);
		double lum = 0.2126 * rR + 0.7152 * rG + 0.0722 * rB;
		if (lum > 1e-6) {
			relSunTint.x = (float)max(0.05, min(20.0, rR / lum));
			relSunTint.y = (float)max(0.05, min(20.0, rG / lum));
			relSunTint.z = (float)max(0.05, min(20.0, rB / lum));
		}
		relSunBeam = (float)max(0.02, min(64.0, pow(fabs(delta), 4.0)));
	}
	bRelSunValid = true;

	// Diagnostic: report the angle between the true and the aberration-shifted
	// Sun direction (should be equal to the aberration displacement).
	{
		static float dbgAng = -1.0f;
		double ang = acos(max(-1.0, min(1.0, dotp(usun, uvis)))) * DEG;
		if (fabs(ang - dbgAng) > 0.5) {
			dbgAng = (float)ang;
			RelLog("RelSun: real=(%.3f,%.3f,%.3f) vis=(%.3f,%.3f,%.3f) beta=%.3f ang=%.2f deg",
				usun.x, usun.y, usun.z, uvis.x, uvis.y, uvis.z, beta, ang);
		}
	}
}

// --------------------------------------------------------------------------
// Run the relativity post-processing pass. Returns the colour texture that
// LightBlur has to sample (the processed buffer, or the source if skipped).
// Execute(true) is used because we are inside an active BeginScene/EndScene
// pair; ImageProcessing backs up and restores the render target state itself.
// --------------------------------------------------------------------------
LPDIRECT3DTEXTURE9 Scene::ApplyRelativityEffects(LPDIRECT3DTEXTURE9 pSrc)
{
	// The switches live in Options -> Visual settings and can be changed at
	// runtime; pick them up every frame and create the resources lazily when
	// the effect is enabled during a session.
	SyncRelativityConfig();

	// The Sun glare is drawn after this pass (RenderGlares) and has to be
	// adjusted explicitly. Its state is independent of the post-process pass:
	// it must also be computed when only the aberration switch is enabled and
	// the Doppler pass is skipped.
	ComputeRelSunVisual();

	if (!pRelativity && !bRelInitTried && (Config->RelDoppler || Config->RelAberration))
		InitRelativityEffects();

	if (!pRelativity || !ptgBuffer[GBUF_REL] || !pRelativity->IsOK()) return pSrc;
	if (!SetupRelativityPass()) return pSrc;

	pRelativity->SetTextureNative("tBack", pSrc, IPF_LINEAR | IPF_CLAMP);
	// Vessel depth buffer for the near-field protection (alpha = distance [m])
	if (Config->RelNearGuard > 0.0 && ptgBuffer[GBUF_DEPTH])
		pRelativity->SetTextureNative("tDepth", ptgBuffer[GBUF_DEPTH], IPF_POINT | IPF_CLAMP);
	pRelativity->SetOutputNative(0, psgBuffer[GBUF_REL]);
	if (!pRelativity->Execute(true)) {
		LogErr("pRelativity Execute Failed");
		return pSrc;
	}
	return ptgBuffer[GBUF_REL];
}
