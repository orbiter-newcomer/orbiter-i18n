// ============================================================================
// TerrainRender.inc (D3D9Client)
//
// Terrain/shadow rendering features that are kept outside the official sources.
// The file is included by the official .cpp files at the hook points; a compile
// time selector decides which part of the file belongs to which translation unit:
//
//   Scene.cpp   : #define TERRAINRENDER_SCENE   -> unified scene shadow system (G1)
//   VBase.cpp   : #define TERRAINRENDER_BASE    -> base structure shadow casters
//
// Current features:
//  1. Unified scene shadow system (G1, "scene shadow" group): one rendering
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
//
// NOTE: the official projected ground shadow code (vVessel::RenderGroundShadow,
// vBase::RenderGroundShadow) lives in the OFFICIAL source files VVessel.cpp and
// VBase.cpp again. It was moved back there so that this file only contains the
// fork-specific G1 scene shadow system.
// ============================================================================
#ifndef D3D9CLIENT_TERRAINRENDER_INC
#define D3D9CLIENT_TERRAINRENDER_INC




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

	// 2) vessels / wrecks: NOT rendered into the G1 shadow map. The vessel and
	//    base shadow systems stay on the official near field (G0) path; the G1
	//    system only serves the terrain (terrain self-shadowing + terrain
	//    receivers). Casting vessels here would also make the terrain sample
	//    vessel shadows via G1, which is not wanted.

	// 3) base structures: NOT rendered into the G1 shadow map (official path).

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



	// ---- diagnostics ----------------------------------------------------------
	if (Config->DebugLvl > 2) {
		DWORD tEnd = GetTickCount();
		static int nLogCount = 0;
		if (nLogCount++ % 120 == 0) {
			oapiWriteLogV("SceneShadow: R=%.0f size=%d tiles=%d time=%u ms",
				R, smapT.size, nTiles, (unsigned)(tEnd - tStart));
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
// PART 3: base structure shadow casters (VBase.cpp) - REMOVED
//
// The G1 shadow system only serves the terrain. Base structures (and vessels)
// keep using the official near field shadow system, so they are not rendered
// into the G1 shadow map and no vBase::RenderShadowCasters hook is required.
// ============================================================================


// ============================================================================
// PART: SURF - surface tile LOD morph / fade / shadow receive (Surfmgr2.cpp)
// ============================================================================
#if defined(TERRAINRENDER_SURF)

// -----------------------------------------------------------------------
// Geometric LOD morph (CPU vertex interpolation, thread safe)
//
// PrepareMorph() only marks the transition start (called from the render
// thread while the tile tree is locked). All vertex data processing happens
// lazily in UpdateMorph() from the tile OWN vertices, so no cross-thread
// access to the parent tile (and no loader race) is involved.
//
// Restored verbatim from commit 04fe67f5. The two known failure modes of an
// earlier version are handled by the "sticky" morphHoldLevel (a morph is done
// only once per level, so small LOD flips cannot restart it and the terrain
// does not pulsate) and the 3 second cooldown. A morph must only be started on
// a real LOD switch - see the PrepareMorph call site in Tilemgr2_imp.hpp.
// -----------------------------------------------------------------------
void SurfTile::PrepareMorph ()
{
	if (!mesh || !mesh->vtx || lvl <= 0) return;
	if (!(Config && Config->TerrainTransitionMs > 0)) return;
	if (morphT0 > 0.0) return;			// already running
	// Sticky morph: a morph is done only once per level. Small LOD flips around the
	// same level (caused by the tile count limiter oscillating) must NOT restart it,
	// otherwise the terrain visibly "pulses". The hold level is reset when the tile
	// object is destroyed (= real loading/removal from the tree).
	if (morphHoldLevel >= 0 && lvl <= morphHoldLevel) return;
	if (lastMorphEnd > 0.0 && (oapiGetSysTime() - lastMorphEnd) < 3.0) return;	// cooldown (second line of defence)
	morphHoldLevel = lvl;
	morphT0 = oapiGetSysTime();
}

void SurfTile::UpdateMorph ()
{
	if (morphT0 <= 0.0) return;
	if (!mesh || !mesh->vtx) { morphT0 = 0.0; return; }

	int res = mgr->GridRes();
	int n = (res+1)*(res+1);
	if ((int)mesh->nv < n) { morphT0 = 0.0; return; }

	double dur = (Config && Config->TerrainTransitionMs > 0) ? Config->TerrainTransitionMs * 0.001 : 0.0;
	if (dur <= 0.0) { morphT0 = 0.0; return; }

	// first call: build the low pass filtered ("coarse") vertex positions from the
	// tile own full detail geometry. No other tile is touched here.
	// The filter radius must match the resolution of the parent tile that was
	// displayed before the switch: the parent's mesh vertices in this tile's area
	// are 2 grid steps apart (each linear tile dimension is half of the parent's),
	// so the coarse geometry is a 2x2 downsample - a 3x3 average (filt=1) is the
	// closest approximation. The previous filt=2 (5x5 average) over-smoothed the
	// start geometry and showed up as a geometric "flattening" at the start of
	// every morph.
	if (!morphLow || !morphHigh) {
		if (!morphLow) morphLow = new float[n*3];
		if (!morphHigh) morphHigh = new float[n*3];
		const int filt = 1;
		for (int i = 0; i <= res; i++) {
			for (int j = 0; j <= res; j++) {
				int ci = i*(res+1)+j;
				VERTEX_2TEX &v = mesh->vtx[ci];
				morphHigh[3*ci+0] = v.x; morphHigh[3*ci+1] = v.y; morphHigh[3*ci+2] = v.z;
				// low pass elevation around this vertex
				float sum = 0.0f; int cnt = 0;
				for (int di = -filt; di <= filt; di++) {
					int ii = i + di; if (ii < 0 || ii > res) continue;
					for (int dj = -filt; dj <= filt; dj++) {
						int jj = j + dj; if (jj < 0 || jj > res) continue;
						sum += mesh->vtx[ii*(res+1)+jj].e; cnt++;
					}
				}
				float eLow = (cnt > 0 ? sum / (float)cnt : v.e);
				// Low resolution position: move the vertex along the planet radius
				// direction by the elevation difference (eLow - v.e).
				//
				// The vertex coordinates are stored RELATIVE TO THE TILE ORIGIN
				// (shift_origin is used for lvl>=4, see Tile::CreateMesh_quadpatch),
				// so the radial direction must be computed from the planet-centred
				// position (v + vtxshift). Using v alone (the old implementation)
				// produced a displacement in the wrong direction, which showed up as
				// a geometric "twitch" at the start of every morph.
				float wx = (float)(v.x + vtxshift.x);
				float wy = (float)(v.y + vtxshift.y);
				float wz = (float)(v.z + vtxshift.z);
				float len = sqrtf(wx*wx + wy*wy + wz*wz);
				float k = (len > 1.0f ? (eLow - v.e) / len : 0.0f);
				morphLow[3*ci+0] = v.x + wx * k;
				morphLow[3*ci+1] = v.y + wy * k;
				morphLow[3*ci+2] = v.z + wz * k;
			}
		}
	}

	double el = oapiGetSysTime() - morphT0;
	float m = (float)(el / dur);
	if (m < 0.0f) m = 0.0f;
	if (m > 1.0f) m = 1.0f;
	// Only INTERIOR vertices are morphed. Border vertices keep their full detail
	// position so that they always match the neighbouring tiles (no tearing).
	for (int mi = 1; mi < res; mi++) {
		for (int mj = 1; mj < res; mj++) {
			int ci = mi*(res+1)+mj;
			mesh->vtx[ci].x = morphLow[3*ci+0] + (morphHigh[3*ci+0]-morphLow[3*ci+0])*m;
			mesh->vtx[ci].y = morphLow[3*ci+1] + (morphHigh[3*ci+1]-morphLow[3*ci+1])*m;
			mesh->vtx[ci].z = morphLow[3*ci+2] + (morphHigh[3*ci+2]-morphLow[3*ci+2])*m;
		}
	}
	mesh->MapVertices(mgr->Dev());
	if (m >= 1.0f) {
		// restore the full detail positions and release the morph buffers
		for (int ci = 0; ci < n; ci++) {
			mesh->vtx[ci].x = morphHigh[3*ci+0];
			mesh->vtx[ci].y = morphHigh[3*ci+1];
			mesh->vtx[ci].z = morphHigh[3*ci+2];
		}
		mesh->MapVertices(mgr->Dev());
		delete[] morphLow; morphLow = NULL;
		delete[] morphHigh; morphHigh = NULL;
		morphT0 = 0.0;
		lastMorphEnd = oapiGetSysTime();
	}
}

#endif // TERRAINRENDER_SURF

// ============================================================================
// PART: TILES - tile manager LOD counter / cross-fade bookkeeping (Tilemgr2.cpp)
// ============================================================================
#if defined(TERRAINRENDER_TILES)

void TileManager2Base::UpdateTileCountSmooth ()
{
	// Dynamic LOD limiter input. The limiter must limit the number of tiles that
	// are actually rendered - using the number of ALLOCATED tile objects
	// (TilesLoaded) was wrong: it is dominated by cached/preloaded tiles and
	// changes in big steps while the terrain streams in. That injected a step of
	// up to ~0.7 LOD levels into the target level, enough to push tiles across
	// the hysteresis dead band, so they flipped back and forth between the
	// coarse parent and the refined children (visible as pumping / flicker).
	//
	// This function is called once per render pass (from the templated Render
	// methods). The rendered leaf tile count is collected in ProcessNode during
	// the main scene pass only, so the update and the reset are also restricted
	// to the main scene pass (other passes render different views / env maps and
	// must not distort the count).
	Scene *scene = GetScene();
	if (!scene || scene->GetRenderPass() != RENDERPASS_MAINSCENE) return;

	int rendered = tilesRenderedCur;
	tilesRenderedCur = 0;	// start counting the next main scene pass

	double now = oapiGetSysTime();
	double dt = now - tileCountT;
	if (tileCountSmooth < 0.0 || dt <= 0.0 || dt > 1.0) {
		tileCountSmooth = (double)rendered;
		tileCountT = now;
		return;
	}
	double a = (dt > 0.25 ? 0.25 : dt);	// ~1 s time constant at frame rate
	tileCountSmooth += a * ((double)rendered - tileCountSmooth);
	tileCountT = now;
}

void TileManager2Base::AddFadeTile (Tile *tile, double t0)
{
	if (!tile) return;
	for (size_t i = 0; i < fadeTiles.size(); i++) {
		if (fadeTiles[i].tile == tile) { fadeTiles[i].t0 = t0; return; }
	}
	FADETILE ft;
	ft.tile = tile;
	ft.t0   = t0;
	fadeTiles.push_back(ft);
}

void TileManager2Base::RemoveFadeTile (Tile *tile)
{
	for (size_t i = 0; i < fadeTiles.size(); i++) {
		if (fadeTiles[i].tile == tile) { fadeTiles.erase(fadeTiles.begin() + i); return; }
	}
}

bool TileManager2Base::IsFadeTile (Tile *tile) const
{
	for (size_t i = 0; i < fadeTiles.size(); i++) {
		if (fadeTiles[i].tile == tile) return true;
	}
	return false;
}

void TileManager2Base::UpdateFadeTiles ()
{
	if (fadeTiles.empty()) return;
	double dur = (Config && Config->TerrainTransitionMs > 0) ? Config->TerrainTransitionMs * 0.001 : 0.0;
	double now = oapiGetSysTime();
	for (size_t i = 0; i < fadeTiles.size();) {
		Tile *tile = fadeTiles[i].tile;
		bool drop = (tile == NULL) || (tile->state != Tile::Active) ||
			(dur <= 0.0) || (now - fadeTiles[i].t0 > dur);
		if (drop) fadeTiles.erase(fadeTiles.begin() + i);
		else i++;
	}
}

#endif // TERRAINRENDER_TILES

// ============================================================================
// PART: LOADWAIT - terrain load wait state machine (D3D9Client.cpp)
// ============================================================================
#if defined(TERRAINRENDER_LOADWAIT)

// ============================================================================
// TerrainLoadWait.inc (D3D9Client)
//
// Terrain load wait feature:
//  - pause the simulation after the start until the initial terrain around the
//    camera has been loaded (rendering continues while paused, so the tile
//    loaders keep working), then resume automatically;
//  - re-run the wait on camera switches (event driven, decided by the
//    suborbital/near-switch rules in ShouldWaitForCameraLoad()).
//
// Kept outside the official sources: D3D9Client.cpp only calls into this file
// at the hook points (clbkRenderScene, constructor member init). The tile
// quad-tree access is provided by TileManager2::TileTree() (1-line getter).
// ============================================================================

// ---- terrain load inspection ------------------------------------------------

// Counts the tiles in a quad-tree that are queued or currently loading.
static int TerrainLoadCountPendingNode (const QuadTreeNode<SurfTile> *node)
{
	if (!node) return 0;
	int n = 0;
	const SurfTile *t = node->Entry();
	if (t && (t->State() == Tile::InQueue || t->State() == Tile::Loading)) n++;
	for (int i = 0; i < 4; i++) n += TerrainLoadCountPendingNode (node->Child (i));
	return n;
}

// Returns the number of pending (queued or loading) terrain tiles of a tile manager.
static int TerrainLoadPendingTileCount (TileManager2<SurfTile> *mgr)
{
	if (!mgr) return 0;
	return TerrainLoadCountPendingNode (mgr->TileTree (0)) + TerrainLoadCountPendingNode (mgr->TileTree (1));
}

// ---- terrain load wait state machine ----------------------------------------

// Resets the wait state machine when a new simulation session is created
// (called from clbkPostCreation, i.e. once per scenario start, before the first
// frame is rendered). This makes the initial "wait for terrain" pause happen
// deterministically for every new scenario, including fast scenario switches
// where the simulation-time based new-session detection could miss it.
void D3D9Client::ResetTerrainLoadWait ()
{
	terrainWaitState    = 0;
	terrainWaitFrames   = 0;
	terrainWaitPaused   = false;
	terrainWaitT0       = 0.0;
	terrainWaitStable   = 0;
	terrainWaitCooldown = 0.0;
	lastSimTime         = 0.0;
	camTargetLast       = 0;
	focusLast           = 0;
	camModeLast         = -1;
	lastTargetValid     = false;
	lastTargetRefBody   = 0;
}

void D3D9Client::UpdateTerrainLoadWait ()
{
	if (!Config || Config->TerrainLoadWait == 0) return;
	if (!scene || !bRunning) return;

	// A new simulation is detected when the simulation time jumps backwards
	// (e.g. after reloading a scenario). Simulation time does not advance while
	// paused, so the wait itself can never trigger a false restart here.
	double simt = oapiGetSimTime();
	bool newSim = (lastSimTime > 1.0 && simt < lastSimTime - 1.0);
	lastSimTime = simt;

	if (terrainWaitState == 3) {
		// New simulation (scenario reload): restart the wait.
		if (newSim) {
			terrainWaitState  = 0;
			terrainWaitFrames = 0;
			terrainWaitPaused = false;
			terrainWaitStable = 0;
		}
		else {
			// Event-driven camera switch handling: only a change of the camera
			// target, the camera mode, or the focus object is a switch event.
			// Whether the terrain load wait is started is decided by the
			// trajectory/position rules in ShouldWaitForCameraLoad().
			if (Config->WaitTerrainLoadCamera != 0) {
				int       camMode = oapiCameraMode();
				OBJHANDLE hTarget = oapiCameraTarget();
				OBJHANDLE hFocus  = oapiGetFocusObject();
				bool camEvent = (hTarget != camTargetLast) || (camMode != camModeLast) || (hFocus != focusLast);
				camTargetLast = hTarget;
				focusLast     = hFocus;
				camModeLast   = camMode;

				bool doWait = false;
				if (camEvent) doWait = ShouldWaitForCameraLoad ();

				// Always advance the previous-target record, so that the next
				// switch compares against the target that was active before it.
				UpdateCameraTargetRecord ();

				if (doWait && !oapiGetPause() && oapiGetSysTime() - terrainWaitCooldown > 2.0) {
					oapiWriteLogV ("TerrainLoadWait: camera switch (mode=%d target=%p focus=%p)", camMode, hTarget, hFocus);
					terrainWaitState  = 0;
					terrainWaitFrames = 0;
					terrainWaitPaused = false;
					terrainWaitStable = 0;
				}
			}
			return;
		}
	}

	switch (terrainWaitState) {
	case 0:		// first frame of this wait cycle: start the warm-up phase
		terrainWaitState  = 1;
		terrainWaitFrames = 0;
		terrainWaitT0     = oapiGetSysTime();
		camTargetLast     = oapiCameraTarget();
		focusLast         = oapiGetFocusObject();
		camModeLast       = oapiCameraMode();
		lastTargetValid   = false;   // sampled again at the next camera check
		break;

	case 1:		// let the scene render one frame so that the initial tile
				// requests have been issued before the simulation is paused
		terrainWaitFrames++;
		if (terrainWaitFrames < 1) break;
		if (oapiGetPause()) {
			// The simulation is already paused (e.g. the previous session was
			// paused, or the scenario was started paused). Still wait for the
			// terrain instead of skipping the wait; only the ownership of the
			// pause state differs, so the initial terrain wait stays visible
			// on every scenario start (including reloading the same scenario).
			terrainWaitPaused = false;
		}
		else {
			oapiSetPause (true);
			terrainWaitPaused = true;
		}
		terrainWaitState  = 2;
		terrainWaitStable = 0;
		oapiWriteLogV ("TerrainLoadWait: waiting for terrain (paused_by_wait=%d)", terrainWaitPaused ? 1 : 0);
		break;

	case 2: {	// wait until the surface tiles around the camera have been loaded
		if (!oapiGetPause()) { terrainWaitState = 3; break; }	// resumed by the user
		int pending = 0;
		bool haveSurface = false;
		vPlanet *vp = scene->GetCameraProxyVisual();
		if (vp && vp->SurfMgr2()) {
			pending = TerrainLoadPendingTileCount (vp->SurfMgr2());
			haveSurface = true;
		}
		bool done = false;
		if (!haveSurface || pending == 0) {
			if (++terrainWaitStable >= 10) done = true;	// stable for 10 frames: terrain is loaded
		}
		else {
			terrainWaitStable = 0;
			if (oapiGetSysTime() - terrainWaitT0 > (double)Config->TerrainLoadWaitTimeout)
				done = true;	// timeout: don't block the user
		}
		if (done) {
			// Minimum wait duration: even if the terrain tiles are already
			// cached (repeated loading of the same scenario) the pause must
			// stay visible for a moment instead of disappearing within a frame.
			if (Config->TerrainLoadWaitMinMs > 0 &&
				oapiGetSysTime() - terrainWaitT0 < Config->TerrainLoadWaitMinMs * 0.001)
				done = false;
		}
		if (done) {
			if (terrainWaitPaused) oapiSetPause (false);
			terrainWaitPaused   = false;
			terrainWaitCooldown = oapiGetSysTime();
			terrainWaitState    = 3;
			oapiWriteLogV ("TerrainLoadWait: done after %.2f s", oapiGetSysTime() - terrainWaitT0);
		}
		break; }

	default:
		break;
	}
}

// ==============================================================
// Camera switch decision for the terrain load wait
//
// Rules:
//  1. A vessel in a stable orbit (periapsis altitude > 0) never triggers.
//  2. If the camera is not aiming at a vessel, or the ground observer mode is
//     active, nothing is triggered.
//  3. A vessel in a suborbital trajectory continues with rules 4/5.
//  4. If the central body of the vessel changed, the wait is triggered.
//  5. Otherwise the wait is triggered only if the ground distance between the
//     previous and the new target is larger than 1 km. The distance is
//     computed from longitude/latitude/altitude, not from global coordinates:
//     the planet's orbital and rotational motion would otherwise dominate.
// ==============================================================

bool D3D9Client::ShouldWaitForCameraLoad ()
{
	// Rule 2: camera must aim at a vessel, and not be in ground observer mode.
	if (oapiCameraMode() == CAM_GROUNDOBSERVER) return false;
	OBJHANDLE hTarget = oapiCameraTarget();
	VESSEL *v = (hTarget ? oapiGetVesselInterface (hTarget) : NULL);
	if (!v) return false;

	// Rule 1: stable orbit (periapsis altitude > 0) -> no wait.
	double pedist = 0.0;
	OBJHANDLE hRef = v->GetPeDist (pedist);
	if (hRef == NULL) return false;
	double R = oapiGetSize (hRef);
	if (pedist - R > 0.0) return false;

	// Rule 4: central body changed -> terrain belongs to another body -> wait.
	if (hRef != lastTargetRefBody) return true;

	// Rule 5: same central body -> compare the planet-relative positions.
	if (!lastTargetValid) return true;
	VECTOR3 gpos;
	v->GetGlobalPos (gpos);
	double lng, lat, rad;
	oapiGlobalToEqu (hRef, gpos, &lng, &lat, &rad);
	double alt    = rad - R;
	double ground = oapiOrthodome (lastTargetLng, lastTargetLat, lng, lat) * R;
	double dh     = alt - lastTargetAlt;
	return (::sqrt (ground*ground + dh*dh) >= 1000.0);
}

void D3D9Client::UpdateCameraTargetRecord ()
{
	OBJHANDLE lastTarget = oapiCameraTarget ();
	lastTargetValid = false;
	VESSEL *v = (lastTarget ? oapiGetVesselInterface (lastTarget) : NULL);
	if (!v) return;
	double pedist = 0.0;
	OBJHANDLE hRef = v->GetPeDist (pedist);
	if (!hRef) return;
	VECTOR3 gpos;
	v->GetGlobalPos (gpos);
	double lng, lat, rad;
	oapiGlobalToEqu (hRef, gpos, &lng, &lat, &rad);
	lastTargetRefBody = hRef;
	lastTargetLng     = lng;
	lastTargetLat     = lat;
	lastTargetAlt     = rad - oapiGetSize (hRef);
	lastTargetValid   = true;
}

void D3D9Client::DrawTerrainLoadWaitLabel ()
{
	if (terrainWaitState != 2 || !terrainWaitPaused) return;
	if (!pDevice || !pFramework || !pFramework->GetLargeFont()) return;

	wchar_t wbuf[256] = L"";
	oapiUtf8ToWide (oapiTr("Loading terrain..."), wbuf, 256);
	RECT rect = _RECT (0, viewH / 2 - 20, viewW, viewH / 2 + 20);
	pDevice->BeginScene();
	pFramework->GetLargeFont()->DrawTextW (NULL, wbuf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE, D3DCOLOR_XRGB (255, 255, 255));
	pDevice->EndScene();
}

#endif // TERRAINRENDER_LOADWAIT
#endif // D3D9CLIENT_TERRAINRENDER_INC

