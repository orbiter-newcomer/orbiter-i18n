// ============================================================================
// TerrainRender.inc (D3D9Client)
//
// Terrain-rendering features kept outside the official sources. The file is
// included by the official .cpp files at the hook points; a compile-time
// selector decides which part belongs to which translation unit:
//
//   Surfmgr2.cpp  : #define TERRAINRENDER_SURF     -> tile LOD morph / fade
//   Tilemgr2.cpp  : #define TERRAINRENDER_TILES    -> LOD counter / fade bookkeeping
//   D3D9Client.cpp: #define TERRAINRENDER_LOADWAIT -> terrain load wait state machine
//
// The official shadow systems are used unchanged (Scene::RenderShadowMap and
// the projected ground shadow in VVessel.cpp/VBase.cpp). The fork-specific
// unified scene shadow (G1) has been removed.
// ============================================================================
#ifndef D3D9CLIENT_TERRAINRENDER_INC
#define D3D9CLIENT_TERRAINRENDER_INC


// ----------------------------------------------------------------------------
// Terrain self shadowing overlay (horizon angle field).
//
// The implementation lives in the D3D9Client translation unit (part
// TERRAINRENDER_HORIZON below). The terrain tile manager calls the mask pass
// after the terrain has been rendered; the client calls TerrainHorizon_Exit()
// when the simulation/session is closed (device resources must be released
// while the device is still alive).
// ----------------------------------------------------------------------------
class SurfTile;
class vPlanet;
template<class T> class TileManager2;

void TerrainHorizon_RenderMask (TileManager2<SurfTile> *mgr, bool use_zbuf);
void TerrainHorizon_Exit ();






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

// ============================================================================
// PART: HORIZON - terrain self shadowing via horizon angle field (D3D9Client.cpp)
//
// Terrain self shadowing ("mountain shadows") computed analytically instead of
// with a shadow map:
//
//  1. For every rendered terrain tile a per-tile "horizon field" is baked on
//     worker threads: for R x R field points the terrain horizon elevation
//     angle is computed in K azimuth directions (max over distance, including
//     the planetary curvature drop d^2/(2R)). The data is sun independent.
//  2. In the renderer the field is uploaded to a small R32F texture
//     (width R, height R*K) and applied in a second terrain pass as a
//     multiplicative "sun visibility mask": the sun altitude above the local
//     horizontal plane at each pixel is compared with the horizon angle for the
//     pixel's sun azimuth. The pass is independent of the official shadow
//     systems (the official near field shadow map and the projected vessel
//     shadows are used unchanged).
//
// Config: TerrainHorizon* (see TerrainRenderMembers_D3D9Config.inc).
// Debug: TerrainHorizonDebug = 1 -> red/yellow sun visibility map + tile grid,
//        2 -> horizon angle field. Blue = no horizon field (not baked yet).
// ============================================================================
#if defined(TERRAINRENDER_HORIZON)

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <deque>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include "Spherepatch.h"	// VBMESH
#include "Log.h"			// LogAlw / LogErr

namespace THZ {

// ----------------------------------------------------------------------------
// configuration snapshot
// ----------------------------------------------------------------------------
struct Cfg {
	bool   enabled;
	int    debug;
	int    fieldSize;		// R
	int    dirs;			// K
	double rangeM;			// maximum horizon distance [m]
	double strength;		// 0..1
	int    threads;
	size_t cacheBytes;
	double maxAlt;
	int    minLevel;
	int    updateFrames;
	double contourM;		// debug contour line interval [m] (0=off)
};

static Cfg  g_cfg;
static bool g_cfgValid = false;

static void ReadCfg ()
{
	if (!Config) return;
	Cfg c;
	c.enabled     = (Config->TerrainHorizonShadow != 0);
	c.debug       = Config->TerrainHorizonDebug;
	c.fieldSize   = Config->TerrainHorizonFieldSize;
	c.dirs        = Config->TerrainHorizonDirs;
	c.rangeM      = double(Config->TerrainHorizonRangeKm) * 1000.0;
	c.strength    = double(Config->TerrainHorizonStrength) * 0.01;
	c.threads     = Config->TerrainHorizonThreads;
	c.cacheBytes  = (size_t)Config->TerrainHorizonCacheMB * 1024u * 1024u;
	c.maxAlt      = double(Config->TerrainHorizonMaxAlt);
	c.minLevel    = Config->TerrainHorizonMinLevel;
	c.updateFrames = Config->TerrainHorizonUpdate;
	c.contourM    = double(Config->TerrainHorizonContour);
	if (c.fieldSize < 16)  c.fieldSize = 16;
	if (c.fieldSize > 128) c.fieldSize = 128;
	if (c.dirs < 8)  c.dirs = 8;
	if (c.dirs > 32) c.dirs = 32;
	if (c.dirs & 1)  c.dirs++;			// keep the number of dirs even
	if (c.threads < 1) c.threads = 1;
	if (c.threads > 3) c.threads = 3;
	if (c.updateFrames < 1) c.updateFrames = 1;
	g_cfg = c;
	g_cfgValid = true;
}

// ----------------------------------------------------------------------------
// tile key
// ----------------------------------------------------------------------------
struct Key {
	int lvl, ilat, ilng;
	Key () : lvl(-1), ilat(0), ilng(0) {}
	Key (int l, int a, int g) : lvl(l), ilat(a), ilng(g) {}
	bool operator== (const Key &o) const { return lvl==o.lvl && ilat==o.ilat && ilng==o.ilng; }
};

struct KeyHash {
	size_t operator() (const Key &k) const {
		return ((size_t)(unsigned)k.lvl * 73856093u) ^
		       ((size_t)(unsigned)k.ilat * 19349663u) ^
		       ((size_t)(unsigned)k.ilng * 83492791u);
	}
};

// ============================================================================
// Elevation pyramid sampler (bake threads only)
//
// Reads the planet elevation archives (Textures2\<planet>\Archive\Elev.tree and
// Elev_mod.tree) through an own ZTreeMgr instance, decodes the ELEVFILEHEADER
// grid (INT16 raw * scale + offset), applies the Elev_mod layer and bilinearly
// samples the global height field at arbitrary lat/lng. A small LRU tile cache
// keeps the hot tiles resident.
// ============================================================================
class ElevationSampler
{
public:
	struct TileData {
		ELEVFILEHEADER hdr;
		std::vector<float> data;	// TILE_ELEVSTRIDE^2 values [m]
		bool valid;
		TileData () : valid(false) { memset(&hdr, 0, sizeof(hdr)); }
	};

	ElevationSampler ()
		: treeElev(NULL), treeMod(NULL), rp(0.0), elevRes(1.0), fineLvl(-1),
		  cacheLimit(48), lastTile(NULL) {}

	bool Init (const std::string &dataRoot, double rplanet, double elevRes);
	void Exit ();

	// Bilinear height sample [m]. 'lvl' is the preferred elevation pyramid
	// level; if the tile does not exist there a coarser level is used.
	float Sample (double lat, double lng, int lvl);

	int FinestLevel () const { return fineLvl; }
	double PlanetRadius () const { return rp; }

	int LevelForResolution (double meters) const
	{
		// Archive level L stores 256x256 cells over a tile covering PI/2^(L-3)
		// radians (the archive root tiles are at level 4 = two hemispheres).
		if (meters <= 0.0) return fineLvl;
		double need = PI * rp / (256.0 * meters);	// 2^(L-3)
		int k = 0;
		while ((double)(1u << k) < need && k < 20) k++;
		int l = k + 3;
		if (l > fineLvl) l = fineLvl;
		if (l < 4) l = 4;
		return l;
	}

	static void TileIdx (double lat, double lng, int lvl, int &ilat, int &ilng)
	{
		// The elevation archives store their root tiles at archive level 4 (two
		// hemispheres), and a node at archive level L has the tile index range of
		// render level L-4: nlat = 1<<(L-4), nlng = 2<<(L-4).
		int renderLvl = lvl - 4;
		if (renderLvl < 0) renderLvl = 0;
		int nlat = 1 << renderLvl;
		int nlng = 2 << renderLvl;
		ilat = (int)floor((PI05 - lat) / PI * (double)nlat);
		ilng = (int)floor((lng + PI) / PI2 * (double)nlng);
		if (ilat < 0) ilat = 0;
		if (ilat >= nlat) ilat = nlat - 1;
		if (ilng < 0) ilng = 0;
		if (ilng >= nlng) ilng = nlng - 1;
	}

private:
	ZTreeMgr *treeElev;
	ZTreeMgr *treeMod;
	double    rp;
	double    elevRes;
	int       fineLvl;
	int       cacheLimit;
	std::string root;

	std::unordered_map<Key, TileData, KeyHash> cache;
	std::deque<Key> lru;
	Key       lastKey;
	TileData *lastTile;

	TileData *GetTile (int lvl, int ilat, int ilng);
	int  FindLevel (double lat, double lng, int lvlFrom);
	static bool ParseElev (const BYTE *buf, DWORD nbytes, ELEVFILEHEADER &hdr, std::vector<float> &data);
	static void ApplyElevMod (const BYTE *buf, DWORD nbytes, std::vector<float> &data);
};

bool ElevationSampler::Init (const std::string &dataRoot, double rplanet, double _elevRes)
{
	root = dataRoot;
	rp = rplanet;
	elevRes = _elevRes;
	fineLvl = -1;
	cacheLimit = 48;
	lastTile = NULL;
	lastKey = Key();

	if (root.empty()) return false;
	treeElev = ZTreeMgr::CreateFromFile(root.c_str(), ZTreeMgr::LAYER_ELEV);
	if (!treeElev) return false;
	treeMod = ZTreeMgr::CreateFromFile(root.c_str(), ZTreeMgr::LAYER_ELEVMOD);

	// Archive level limit: the finest level present at ANY of the probe points.
	// (Local availability can be coarser, Sample() falls back level by level.)
	// Note: the probe must NOT require the level at all probe points: the
	// archives contain finer levels only in parts of the planet, and using the
	// common denominator would throw away most of the available detail.
	static const double plats[5] = { 0.6, -0.6, 0.0, 0.35, -0.35 };
	static const double plngs[5] = { 0.0, 1.2, -2.0, 2.6, -1.1 };
	fineLvl = -1;
	for (int l = 16; l >= 4; l--) {
		bool any = false;
		for (int i = 0; i < 5 && !any; i++) {
			int ilat, ilng;
			TileIdx(plats[i], plngs[i], l, ilat, ilng);
			if (treeElev->Idx(l, ilat, ilng) != (DWORD)-1) any = true;
		}
		if (any) { fineLvl = l; break; }
	}
	if (fineLvl < 4) return false;
	return true;
}

void ElevationSampler::Exit ()
{
	cache.clear();
	lru.clear();
	lastTile = NULL;
	if (treeElev) { delete treeElev; treeElev = NULL; }
	if (treeMod)  { delete treeMod;  treeMod = NULL; }
}

bool ElevationSampler::ParseElev (const BYTE *buf, DWORD nbytes, ELEVFILEHEADER &hdr, std::vector<float> &data)
{
	if (!buf || nbytes < sizeof(ELEVFILEHEADER)) return false;
	memcpy(&hdr, buf, sizeof(ELEVFILEHEADER));
	if (hdr.hdrsize < (int)sizeof(ELEVFILEHEADER) || hdr.hdrsize > (int)nbytes) return false;

	const int n = TILE_ELEVSTRIDE * TILE_ELEVSTRIDE;
	data.assign(n, 0.0f);
	const BYTE *p = buf + hdr.hdrsize;
	const DWORD avail = nbytes - hdr.hdrsize;
	const float scale = (float)hdr.scale;
	const float offset = (float)hdr.offset;

	switch (hdr.dtype) {
	case 0:		// flat tile
		for (int i = 0; i < n; i++) data[i] = offset;
		return true;
	case 8:
		if (avail < (DWORD)n) return false;
		for (int i = 0; i < n; i++) data[i] = p[i] * scale + offset;
		return true;
	case -16: {
		if (avail < (DWORD)(n * 2)) return false;
		const INT16 *s = (const INT16*)p;
		for (int i = 0; i < n; i++) data[i] = (float)s[i] * scale + offset;
		return true; }
	case 16: {
		if (avail < (DWORD)(n * 2)) return false;
		const UINT16 *s = (const UINT16*)p;
		for (int i = 0; i < n; i++) data[i] = (float)s[i] * scale + offset;
		return true; }
	}
	return false;
}

void ElevationSampler::ApplyElevMod (const BYTE *buf, DWORD nbytes, std::vector<float> &data)
{
	if (!buf || nbytes < sizeof(ELEVFILEHEADER)) return;
	ELEVFILEHEADER hdr;
	memcpy(&hdr, buf, sizeof(ELEVFILEHEADER));
	if (hdr.hdrsize < (int)sizeof(ELEVFILEHEADER) || hdr.hdrsize > (int)nbytes) return;

	const int n = TILE_ELEVSTRIDE * TILE_ELEVSTRIDE;
	const BYTE *p = buf + hdr.hdrsize;
	const DWORD avail = nbytes - hdr.hdrsize;
	const float scale = (float)hdr.scale;
	const float offset = (float)hdr.offset;

	switch (hdr.dtype) {
	case 0:
		for (int i = 0; i < n; i++) data[i] = offset;
		break;
	case 8: {
		if (avail < (DWORD)n) return;
		const BYTE mask = 255;
		for (int i = 0; i < n; i++) if (p[i] != mask) data[i] = p[i] * scale + offset;
		break; }
	case -16: {
		if (avail < (DWORD)(n * 2)) return;
		const INT16 mask = SHRT_MAX;
		const INT16 *s = (const INT16*)p;
		for (int i = 0; i < n; i++) if (s[i] != mask) data[i] = (float)s[i] * scale + offset;
		break; }
	}
}

ElevationSampler::TileData *ElevationSampler::GetTile (int lvl, int ilat, int ilng)
{
	Key k(lvl, ilat, ilng);
	auto it = cache.find(k);
	if (it != cache.end()) {
		lastKey = k;
		lastTile = &it->second;
		return lastTile;
	}

	TileData td;
	BYTE *buf = NULL;
	DWORD nd = treeElev->ReadData(lvl, ilat, ilng, &buf);
	bool ok = false;
	if (nd) {
		ok = ParseElev(buf, nd, td.hdr, td.data);
		treeElev->ReleaseData(buf);
	}
	if (ok && treeMod) {
		BYTE *mbuf = NULL;
		DWORD nm = treeMod->ReadData(lvl, ilat, ilng, &mbuf);
		if (nm) {
			ApplyElevMod(mbuf, nm, td.data);
			treeMod->ReleaseData(mbuf);
		}
	}
	td.valid = ok;

	// LRU eviction
	while (cache.size() >= (size_t)cacheLimit && !lru.empty()) {
		Key old = lru.front();
		lru.pop_front();
		if (lastTile && old == lastKey) lastTile = NULL;	// don't leave a dangling fast-path pointer
		cache.erase(old);
	}
	lru.push_back(k);
	auto res = cache.insert(std::make_pair(k, std::move(td)));
	if (!res.second) return NULL;
	lastKey = k;
	lastTile = &res.first->second;
	return lastTile;
}

int ElevationSampler::FindLevel (double lat, double lng, int lvlFrom)
{
	// The elevation archives have their root nodes at level 4; levels 0-3 do
	// not exist (trying them would alias the root nodes in ZTreeMgr::Idx).
	if (lvlFrom > fineLvl) lvlFrom = fineLvl;
	if (lvlFrom < 4) lvlFrom = 4;
	for (int l = lvlFrom; l >= 4; l--) {
		int ilat, ilng;
		TileIdx(lat, lng, l, ilat, ilng);
		if (treeElev->Idx(l, ilat, ilng) != (DWORD)-1) return l;
	}
	return -1;
}

float ElevationSampler::Sample (double lat, double lng, int lvl)
{
	// Fast path: the last tile covers the requested position
	if (lastTile && lastTile->valid) {
		const ELEVFILEHEADER &h = lastTile->hdr;
		if (lng >= h.lngmin && lng <= h.lngmax && lat >= h.latmin && lat <= h.latmax) {
			double u = (lng - h.lngmin) / (h.lngmax - h.lngmin);
			double v = (lat - h.latmin) / (h.latmax - h.latmin);
			if (u >= 0.0 && u <= 1.0 && v >= 0.0 && v <= 1.0) {
				double fx = u * 256.0, fy = v * 256.0;
				int ix = (int)fx; if (ix > 255) ix = 255; if (ix < 0) ix = 0;
				int iy = (int)fy; if (iy > 255) iy = 255; if (iy < 0) iy = 0;
				float ax = (float)(fx - ix), ay = (float)(fy - iy);
				const float *d = lastTile->data.data();
				int i0 = (iy + 1) * TILE_ELEVSTRIDE + (ix + 1);
				float d00 = d[i0], d10 = d[i0 + 1], d01 = d[i0 + TILE_ELEVSTRIDE], d11 = d[i0 + TILE_ELEVSTRIDE + 1];
				return d00 * (1-ax) * (1-ay) + d10 * ax * (1-ay) + d01 * (1-ax) * ay + d11 * ax * ay;
			}
		}
	}

	int level = FindLevel(lat, lng, lvl);
	if (level < 0) return 0.0f;
	int ilat, ilng;
	TileIdx(lat, lng, level, ilat, ilng);
	TileData *t = GetTile(level, ilat, ilng);
	if (!t || !t->valid) return 0.0f;

	const ELEVFILEHEADER &h = t->hdr;
	double u = (lng - h.lngmin) / (h.lngmax - h.lngmin);
	double v = (lat - h.latmin) / (h.latmax - h.latmin);
	if (u < 0.0) u = 0.0; if (u > 1.0) u = 1.0;
	if (v < 0.0) v = 0.0; if (v > 1.0) v = 1.0;
	double fx = u * 256.0, fy = v * 256.0;
	int ix = (int)fx; if (ix > 255) ix = 255; if (ix < 0) ix = 0;
	int iy = (int)fy; if (iy > 255) iy = 255; if (iy < 0) iy = 0;
	float ax = (float)(fx - ix), ay = (float)(fy - iy);
	const float *d = t->data.data();
	int i0 = (iy + 1) * TILE_ELEVSTRIDE + (ix + 1);
	float d00 = d[i0], d10 = d[i0 + 1], d01 = d[i0 + TILE_ELEVSTRIDE], d11 = d[i0 + TILE_ELEVSTRIDE + 1];
	return d00 * (1-ax) * (1-ay) + d10 * ax * (1-ay) + d01 * (1-ax) * ay + d11 * ax * ay;
}

// ============================================================================
// Horizon field bake
// ============================================================================
struct BakeJob {
	Key    key;
	double latmin, latmax, lngmin, lngmax;
	double planetR;
	double rangeM;
	int    R, K;
	int    nearLvl, farLvl;
	int    priority;
	std::string dataRoot;
	double elevRes;
	OBJHANDLE planet;
};

struct BakeResult {
	Key    key;
	OBJHANDLE planet;
	std::vector<float> values;	// R*R*K angles [rad], row index y = k*R + i
	int    ms;
	bool   ok;
	BakeResult () : planet(NULL), ms(0), ok(false) {}
};

// Far field silhouette sample
struct FarSample {
	double d;
	float  h;
};

static void BakeTile (ElevationSampler &es, const BakeJob &job, std::vector<float> &out, int &ms)
{
	unsigned t0 = GetTickCount();

	const int R = job.R;
	const int K = job.K;
	const double rp = job.planetR;
	const double dmax = job.rangeM;

	out.assign((size_t)R * R * K, 0.0f);

	const double latc = (job.latmin + job.latmax) * 0.5;
	const double lngc = (job.lngmin + job.lngmax) * 0.5;
	const double tileSize = max(job.latmax - job.latmin, job.lngmax - job.lngmin) * rp;
	const double texelWorld = tileSize / double(R);
	const double dNear0 = min(dmax, max(8.0 * tileSize, 3000.0));

	// ---- far field silhouette profiles (per azimuth, from the tile centre) ----
	std::vector<std::vector<FarSample>> profs(K);
	const double coslatc = max(cos(latc), 0.05);
	for (int k = 0; k < K; k++) {
		double phi = PI2 * double(k) / double(K);
		double dy = cos(phi), dx = sin(phi);
		double d = dNear0;
		while (d < dmax) {
			double dlat = d * dy / rp;
			double dlng = d * dx / (rp * coslatc);
			float h = es.Sample(latc + dlat, lngc + dlng, job.farLvl);
			FarSample fs;
			fs.d = d;
			fs.h = h;
			profs[k].push_back(fs);
			d += max(400.0, d * 0.22);
		}
	}

	// ---- per field point ----
	const double latStep = (job.latmax - job.latmin) / double(R);
	const double lngStep = (job.lngmax - job.lngmin) / double(R);
	const double step0 = max(texelWorld * 0.5, 1.0);

	for (int i = 0; i < R; i++) {
		double lat = job.latmin + (i + 0.5) * latStep;
		double coslat = max(cos(lat), 0.02);
		for (int j = 0; j < R; j++) {
			double lng = job.lngmin + (j + 0.5) * lngStep;
			float h0 = es.Sample(lat, lng, job.nearLvl);

			for (int k = 0; k < K; k++) {
				double phi = PI2 * double(k) / double(K);
				double dy = cos(phi), dx = sin(phi);
				double best = 0.0;			// negative horizon = no self occlusion

				// near field: per field point ray march
				double d = step0;
				while (d < dNear0) {
					double dlat = d * dy / rp;
					double dlng = d * dx / (rp * coslat);
					float h = es.Sample(lat + dlat, lng + dlng, job.nearLvl);
					double drop = d * d / (2.0 * rp);
					double ang = atan2((double)h - (double)h0 - drop, d);
					if (ang > best) best = ang;
					d += max(step0, d * 0.15);
				}

				// far field: shared silhouette profile
				const std::vector<FarSample> &pr = profs[k];
				for (size_t q = 0; q < pr.size(); q++) {
					double dd = pr[q].d;
					double drop = dd * dd / (2.0 * rp);
					double ang = atan2((double)pr[q].h - (double)h0 - drop, dd);
					if (ang > best) best = ang;
				}

				if (best < 0.0) best = 0.0;
				if (best > PI05) best = PI05;
				out[((size_t)k * R + i) * R + j] = (float)best;
			}
		}
	}

	ms = (int)(GetTickCount() - t0);
}

// ============================================================================
// Runtime: texture cache, bake worker threads and the mask pass
// ============================================================================
static const int  MAX_JOBS = 512;
static const int  MAX_UPLOADS_PER_FRAME = 3;

// Elevation level that provides a texel size of about 'meters'
// (archive level L stores 256 cells over PI/2^(L-3) radians)
static int LevelForResolution (double rp, double meters, int maxLvl)
{
	if (meters <= 0.0) return maxLvl;
	double need = PI * rp / (256.0 * meters);	// 2^(L-3)
	int k = 0;
	while ((double)(1u << k) < need && k < 20) k++;
	int l = k + 3;
	if (l > maxLvl) l = maxLvl;
	if (l < 4) l = 4;
	return l;
}

class System
{
public:
	System ();
	~System ();

	bool Init (LPDIRECT3DDEVICE9 dev);
	void Exit ();

	void BeginFrame (TileManager2<SurfTile> *mgr, vPlanet *vp, double camAlt);
	bool EnsurePlanet (TileManager2<SurfTile> *mgr, vPlanet *vp);
	void BeginPass (LPDIRECT3DDEVICE9 dev, vPlanet *vp);
	void EndPass (LPDIRECT3DDEVICE9 dev);
	void DrawTile (SurfTile *t);

private:
	enum { N_SAVED_STATES = 11 };

	LPDIRECT3DDEVICE9 pDev;
	ShaderClass      *pMask;
	HANDLE            hMaskWorld, hHorPrm, hHorPrm1, hReady, hTex, hConstVS, hConstPS;
	LPDIRECT3DTEXTURE9 pMissing;
	bool              bDeviceValid;
	bool              bDisabled;
	bool              bFilter;

	OBJHANDLE         hPlanet;
	std::string       dataRoot;
	double            planetR;
	double            elevRes;

	struct Entry {
		LPDIRECT3DTEXTURE9 tex;
		bool     ready;
		unsigned frame;
		Entry () : tex(NULL), ready(false), frame(0) {}
	};
	std::unordered_map<Key, Entry, KeyHash> cache;
	size_t            cacheBytes;

	std::vector<std::thread> threads;
	std::mutex        mtx;
	std::condition_variable cv;
	std::deque<BakeJob>   jobs;
	std::deque<BakeResult> done;
	std::atomic<bool> quit;
	std::atomic<int>  jobCount;
	bool              bThreads;

	int               scanCountdown;
	unsigned          frameId;
	DWORD             savedStates[N_SAVED_STATES];

	int               statReady;
	int               bakeCount;
	long long         bakeMs;

	void StartWorkers ();
	void StopWorkers ();
	void Clear ();
	void WorkerLoop ();
	void UploadDone ();
	void UploadResult (BakeResult &res);
	void ScanTree (TileManager2<SurfTile> *mgr);
	void ScanNode (const QuadTreeNode<SurfTile> *node);
	void Request (SurfTile *tile);
	void Evict ();
};

static System g_sys;

System::System ()
{
	pDev = NULL;
	pMask = NULL;
	hMaskWorld = hHorPrm = hHorPrm1 = hReady = hTex = hConstVS = hConstPS = NULL;
	pMissing = NULL;
	bDeviceValid = false;
	bDisabled = false;
	bFilter = false;
	hPlanet = NULL;
	planetR = 0.0;
	elevRes = 1.0;
	cacheBytes = 0;
	quit = false;
	jobCount = 0;
	bThreads = false;
	scanCountdown = 0;
	frameId = 0;
	statReady = 0;
	bakeCount = 0;
	bakeMs = 0;
	memset(savedStates, 0, sizeof(savedStates));
}

System::~System ()
{
	Exit();
}

bool System::Init (LPDIRECT3DDEVICE9 dev)
{
	if (bDeviceValid) return true;
	if (bDisabled) return false;
	if (!dev) return false;
	oapiWriteLogV("TerrainHorizon: init begin");
	pDev = dev;

	// R32F textures are required for the horizon field / mask shader
	LPDIRECT3DTEXTURE9 probe = NULL;
	if (FAILED(dev->CreateTexture(4, 4, 1, 0, D3DFMT_R32F, D3DPOOL_MANAGED, &probe, NULL)) || !probe) {
		oapiWriteLogV("TerrainHorizon: R32F textures not supported -> disabled");
		bDisabled = true;
		return false;
	}
	probe->Release();

	D3DCAPS9 caps;
	if (FAILED(dev->GetDeviceCaps(&caps))) return false;
	{
		IDirect3D9 *pD3D = NULL;
		bFilter = false;
		if (SUCCEEDED(dev->GetDirect3D(&pD3D)) && pD3D) {
			bFilter = SUCCEEDED(pD3D->CheckDeviceFormat(caps.AdapterOrdinal, caps.DeviceType,
				D3DFMT_X8R8G8B8, D3DUSAGE_QUERY_FILTER, D3DRTYPE_TEXTURE, D3DFMT_R32F)) ? true : false;
			pD3D->Release();
		}
	}

	// 1x1 dummy texture for tiles without a baked field
	if (FAILED(dev->CreateTexture(1, 1, 1, D3DUSAGE_DYNAMIC, D3DFMT_R32F, D3DPOOL_DEFAULT, &pMissing, NULL))) {
		bDisabled = true;
		return false;
	}
	D3DLOCKED_RECT lr;
	if (SUCCEEDED(pMissing->LockRect(0, &lr, NULL, 0))) {
		*(float*)lr.pBits = 0.0f;
		pMissing->UnlockRect(0);
	}

	pMask = new ShaderClass(dev, "Modules/D3D9Client/NewPlanet.hlsl", "TerrainMaskVS", "TerrainMaskPS", "TerrainMask", "");
	if (!pMask) return false;
	hMaskWorld = pMask->GetVSHandle("HorizonWorld");
	hConstVS   = pMask->GetVSHandle("Const");
	hConstPS   = pMask->GetPSHandle("Const");
	hHorPrm    = pMask->GetPSHandle("HorizonPrm0");
	hHorPrm1   = pMask->GetPSHandle("HorizonPrm1");
	hReady     = pMask->GetPSHandle("HorizonReady");
	hTex       = pMask->GetPSHandle("tHorizon");
	if (!hMaskWorld || !hConstVS || !hConstPS || !hHorPrm || !hHorPrm1 || !hReady || !hTex) {
		oapiWriteLogV("TerrainHorizon: mask shader constants missing -> disabled");
		SAFE_DELETE(pMask);
		bDisabled = true;
		return false;
	}

	bDeviceValid = true;
	oapiWriteLogV("TerrainHorizon: overlay initialized (R32F filter=%d)", bFilter ? 1 : 0);
	return true;
}

void System::Exit ()
{
	oapiWriteLogV("TerrainHorizon: exit (bakes=%d)", bakeCount);
	StopWorkers();
	Clear();
	SAFE_RELEASE(pMissing);
	SAFE_DELETE(pMask);
	hMaskWorld = hHorPrm = hHorPrm1 = hReady = hTex = hConstVS = hConstPS = NULL;
	bDeviceValid = false;
	bDisabled = false;
	hPlanet = NULL;
}

void System::Clear ()
{
	for (auto &it : cache) SAFE_RELEASE(it.second.tex);
	cache.clear();
	cacheBytes = 0;
	statReady = 0;
}

bool System::EnsurePlanet (TileManager2<SurfTile> *mgr, vPlanet *vp)
{
	OBJHANDLE h = vp->Object();
	if (h == hPlanet && !dataRoot.empty()) return true;

	StopWorkers();
	Clear();
	hPlanet = h;
	dataRoot = mgr->DataRootDir();
	planetR = oapiGetSize(h);
	elevRes = mgr->ElevRes();
	statReady = 0;
	bakeCount = 0;
	bakeMs = 0;

	if (dataRoot.empty() || planetR <= 0.0) return false;

	// Bodies without an elevation archive cannot have a horizon field.
	ZTreeMgr *probe = ZTreeMgr::CreateFromFile(dataRoot.c_str(), ZTreeMgr::LAYER_ELEV);
	if (!probe) {
		oapiWriteLogV("TerrainHorizon: no elevation archive for %s -> overlay disabled", dataRoot.c_str());
		hPlanet = NULL;
		return false;
	}
	delete probe;

	StartWorkers();
	oapiWriteLogV("TerrainHorizon: body [%s], R=%.0f m, elevRes=%.2f m, field %dx%d, dirs %d",
		dataRoot.c_str(), planetR, elevRes, g_cfg.fieldSize, g_cfg.fieldSize, g_cfg.dirs);
	return true;
}

void System::StartWorkers ()
{
	quit = false;
	jobCount = 0;
	for (int i = 0; i < g_cfg.threads; i++) threads.emplace_back(&System::WorkerLoop, this);
	bThreads = true;
}

void System::StopWorkers ()
{
	if (!bThreads) return;
	{
		std::lock_guard<std::mutex> lk(mtx);
		quit = true;
	}
	cv.notify_all();
	for (auto &t : threads) if (t.joinable()) t.join();
	threads.clear();
	bThreads = false;
	{
		std::lock_guard<std::mutex> lk(mtx);
		jobs.clear();
		done.clear();
	}
}

void System::WorkerLoop ()
{
	ElevationSampler es;
	std::string root;
	bool samplerReady = false;

	for (;;) {
		BakeJob job;
		{
			std::unique_lock<std::mutex> lk(mtx);
			cv.wait(lk, [this] { return quit.load() || !jobs.empty(); });
			if (quit) return;
			job = jobs.front();
			jobs.pop_front();
			jobCount.fetch_sub(1);
		}
		if (!samplerReady || root != job.dataRoot) {
			es.Exit();
			samplerReady = es.Init(job.dataRoot, job.planetR, job.elevRes);
			root = job.dataRoot;
		}
		BakeResult res;
		res.key = job.key;
		res.planet = job.planet;
		if (samplerReady) {
			BakeTile(es, job, res.values, res.ms);
			res.ok = true;
		}
		else res.ok = false;
		{
			std::lock_guard<std::mutex> lk(mtx);
			done.push_back(std::move(res));
		}
	}
}

void System::UploadResult (BakeResult &res)
{
	const int R = g_cfg.fieldSize;
	const int K = g_cfg.dirs;
	const int W = R;
	const int H = R * K;
	if ((int)res.values.size() != W * H) return;

	LPDIRECT3DTEXTURE9 tex = NULL;
	if (FAILED(pDev->CreateTexture(W, H, 1, D3DUSAGE_DYNAMIC, D3DFMT_R32F, D3DPOOL_DEFAULT, &tex, NULL)) || !tex) return;
	D3DLOCKED_RECT lr;
	if (FAILED(tex->LockRect(0, &lr, NULL, 0))) { tex->Release(); return; }
	float *dst = (float*)lr.pBits;
	const int pitch = lr.Pitch / (int)sizeof(float);
	const float *src = res.values.data();
	for (int y = 0; y < H; y++) {
		memcpy(dst + (size_t)y * pitch, src + (size_t)y * W, (size_t)W * sizeof(float));
	}
	tex->UnlockRect(0);

	auto it = cache.find(res.key);
	if (it != cache.end()) {
		SAFE_RELEASE(it->second.tex);
		it->second.tex = tex;
		it->second.ready = true;
		it->second.frame = frameId;
		cacheBytes += (size_t)W * H * sizeof(float);
		if (statReady >= 0) statReady++;
	}
	else tex->Release();

	bakeCount++;
	bakeMs += res.ms;
	if (bakeCount == 1 || (bakeCount % 50) == 0) {
		oapiWriteLogV("TerrainHorizon: uploaded L%d [%d,%d] %d ms (bakes=%d, ready=%d)",
			res.key.lvl, res.key.ilat, res.key.ilng, res.ms, bakeCount, statReady);
	}
	// Diagnostics: horizon angle statistics of the field (should be > 0 deg for
	// real terrain; a constant 0 means the elevation sampling failed).
	if (bakeCount <= 3 || (bakeCount % 100) == 0) {
		float hmin = 1e9f, hmax = -1e9f;
		double hsum = 0.0;
		for (size_t i = 0; i < res.values.size(); i++) {
			float v = res.values[i];
			if (v < hmin) hmin = v;
			if (v > hmax) hmax = v;
			hsum += v;
		}
		oapiWriteLogV("TerrainHorizon: field stats L%d [%d,%d]: min=%.3f max=%.3f avg=%.3f deg",
			res.key.lvl, res.key.ilat, res.key.ilng,
			hmin * 57.29578f, hmax * 57.29578f,
			(res.values.empty() ? 0.0 : hsum / res.values.size()) * 57.29578);
	}
}

void System::UploadDone ()
{
	int n = 0;
	while (n < MAX_UPLOADS_PER_FRAME) {
		BakeResult res;
		{
			std::lock_guard<std::mutex> lk(mtx);
			if (done.empty()) break;
			res = std::move(done.front());
			done.pop_front();
		}
		if (res.planet != hPlanet) continue;		// stale (planet changed)
		auto it = cache.find(res.key);
		if (it == cache.end()) continue;			// no longer requested
		if (!res.ok) {
			cache.erase(it);						// retry on the next scan
			statReady = (statReady > 0 ? statReady - 1 : 0);
			continue;
		}
		UploadResult(res);
		n++;
	}
}

void System::BeginFrame (TileManager2<SurfTile> *mgr, vPlanet *vp, double camAlt)
{
	frameId++;
	UploadDone();

	if (camAlt <= g_cfg.maxAlt) {
		if (scanCountdown-- <= 0) {
			scanCountdown = g_cfg.updateFrames;
			ScanTree(mgr);
		}
	}

	if (cacheBytes > g_cfg.cacheBytes) Evict();

	if ((frameId % 240) == 0) {
		oapiWriteLogV("TerrainHorizon: ready=%d jobs=%d cache=%.1f MB bakes=%d avg=%.1f ms",
			statReady, jobCount.load(), double(cacheBytes) / 1048576.0,
			bakeCount, (bakeCount ? double(bakeMs) / double(bakeCount) : 0.0));
	}
}

void System::ScanTree (TileManager2<SurfTile> *mgr)
{
	for (int i = 0; i < 2; i++) ScanNode(mgr->TileTree(i));
}

void System::ScanNode (const QuadTreeNode<SurfTile> *node)
{
	if (!node) return;
	SurfTile *t = node->Entry();
	if (!t) return;
	if ((t->State() & TILE_VALID) == 0) return;
	if (t->State() == Tile::Active) {
		for (int i = 0; i < 4; i++) ScanNode(node->Child(i));
		return;
	}
	if (t->State() != Tile::ForRender) return;
	Request(t);
}

void System::Request (SurfTile *tile)
{
	const int lvl = tile->Level();
	if (lvl < g_cfg.minLevel) return;

	int ilng, ilat;
	tile->GetIndex(&ilng, &ilat);
	Key k(lvl, ilat, ilng);

	auto it = cache.find(k);
	if (it != cache.end()) {
		it->second.frame = frameId;
		return;
	}
	if (jobCount.load() >= MAX_JOBS) return;

	double latmin, latmax, lngmin, lngmax;
	tile->Extents(&latmin, &latmax, &lngmin, &lngmax);
	if (!(latmax > latmin) || !(lngmax > lngmin)) return;
	if (lngmax - lngmin > PI || latmax - latmin > PI) return;	// seam tile
	if (latmin < -PI05) latmin = -PI05;
	if (latmax > PI05) latmax = PI05;

	const double sizeLat = (latmax - latmin) * planetR;
	const double sizeLng = (lngmax - lngmin) * planetR * max(cos(0.5 * (latmin + latmax)), 0.02);
	const double tileSize = max(sizeLat, sizeLng);
	if (tileSize <= 0.0) return;

	const int R = g_cfg.fieldSize;
	const double texelWorld = tileSize / double(R);

	BakeJob job;
	job.key = k;
	job.latmin = latmin;
	job.latmax = latmax;
	job.lngmin = lngmin;
	job.lngmax = lngmax;
	job.planetR = planetR;
	job.rangeM = g_cfg.rangeM;
	job.R = R;
	job.K = g_cfg.dirs;
	job.nearLvl = LevelForResolution(planetR, texelWorld * 0.5, 14);
	job.farLvl  = LevelForResolution(planetR, max(1000.0, texelWorld * 16.0), 12);
	job.priority = -lvl;
	job.dataRoot = dataRoot;
	job.elevRes = elevRes;
	job.planet = hPlanet;

	Entry e;
	e.frame = frameId;
	cache.insert(std::make_pair(k, e));

	{
		std::lock_guard<std::mutex> lk(mtx);
		jobs.push_back(job);
		jobCount.fetch_add(1);
	}
	cv.notify_one();
}

void System::Evict ()
{
	// Drop the least recently used ready textures until the budget is met
	while (cacheBytes > g_cfg.cacheBytes) {
		unsigned bestFrame = UINT_MAX;
		auto best = cache.end();
		for (auto it = cache.begin(); it != cache.end(); ++it) {
			if (!it->second.ready) continue;
			if (it->second.frame < bestFrame) { bestFrame = it->second.frame; best = it; }
		}
		if (best == cache.end()) break;
		SAFE_RELEASE(best->second.tex);
		cacheBytes -= (size_t)g_cfg.fieldSize * g_cfg.fieldSize * g_cfg.dirs * sizeof(float);
		cache.erase(best);
		if (statReady > 0) statReady--;
	}
	// Also drop pending entries that never completed (worker restart etc.)
	int pending = 0;
	for (auto it = cache.begin(); it != cache.end(); ) {
		if (!it->second.ready && it->second.frame + 1200 < frameId && pending < 8) {
			it = cache.erase(it);
			pending++;
		}
		else ++it;
	}
}

// ----------------------------------------------------------------------------
// Mask pass
// ----------------------------------------------------------------------------
static void DrawMaskNode (const QuadTreeNode<SurfTile> *node, System &sys)
{
	if (!node) return;
	SurfTile *t = node->Entry();
	if (!t) return;
	if ((t->State() & TILE_VALID) == 0) return;
	if (t->State() == Tile::Active) {
		for (int i = 0; i < 4; i++) DrawMaskNode(node->Child(i), sys);
		return;
	}
	if (t->State() != Tile::ForRender) return;
	sys.DrawTile(t);
}

void System::BeginPass (LPDIRECT3DDEVICE9 dev, vPlanet *vp)
{
	static const D3DRENDERSTATETYPE saveIds[N_SAVED_STATES] = {
		D3DRS_ZENABLE, D3DRS_ZWRITEENABLE, D3DRS_ZFUNC, D3DRS_CULLMODE,
		D3DRS_ALPHATESTENABLE, D3DRS_ALPHABLENDENABLE, D3DRS_SRCBLEND,
		D3DRS_DESTBLEND, D3DRS_BLENDOP, D3DRS_SEPARATEALPHABLENDENABLE,
		D3DRS_COLORWRITEENABLE
	};
	for (int i = 0; i < N_SAVED_STATES; i++) {
		DWORD v = 0;
		dev->GetRenderState(saveIds[i], &v);
		savedStates[i] = v;
	}

	ConstParams *cp = vp->GetScatterConst();
	pMask->Setup(pPatchVertexDecl, true, 0);
	pMask->SetVSConstants(hConstVS, cp, sizeof(ConstParams));
	pMask->SetPSConstants(hConstPS, cp, sizeof(ConstParams));
	float prm[4] = { (float)g_cfg.fieldSize, (float)g_cfg.dirs, (float)g_cfg.strength, (float)g_cfg.debug };
	pMask->SetPSConstants(hHorPrm, prm, sizeof(prm));
	float prm1[4] = { (float)g_cfg.contourM, 0.0f, 0.0f, 0.0f };
	pMask->SetPSConstants(hHorPrm1, prm1, sizeof(prm1));

	dev->SetRenderState(D3DRS_ZENABLE, TRUE);
	dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	dev->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	dev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
	dev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
	const bool bDebugOverlay = (g_cfg.debug >= 1 && g_cfg.debug <= 2);
	if (bDebugOverlay) {
		dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
	}
	else {
		dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
		dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
	}
}

void System::EndPass (LPDIRECT3DDEVICE9 dev)
{
	static const D3DRENDERSTATETYPE saveIds[N_SAVED_STATES] = {
		D3DRS_ZENABLE, D3DRS_ZWRITEENABLE, D3DRS_ZFUNC, D3DRS_CULLMODE,
		D3DRS_ALPHATESTENABLE, D3DRS_ALPHABLENDENABLE, D3DRS_SRCBLEND,
		D3DRS_DESTBLEND, D3DRS_BLENDOP, D3DRS_SEPARATEALPHABLENDENABLE,
		D3DRS_COLORWRITEENABLE
	};
	for (int i = 0; i < N_SAVED_STATES; i++) dev->SetRenderState(saveIds[i], savedStates[i]);
}

void System::DrawTile (SurfTile *t)
{
	if (!pMask || !pDev) return;
	VBMESH *mesh = t->GetMesh();
	if (!mesh || !mesh->pVB || !mesh->pIB || mesh->nv == 0) return;

	int ilng, ilat;
	t->GetIndex(&ilng, &ilat);
	Key k(t->Level(), ilat, ilng);

	LPDIRECT3DTEXTURE9 tex = pMissing;
	float ready = 0.0f;
	auto it = cache.find(k);
	if (it != cache.end() && it->second.ready && it->second.tex) {
		tex = it->second.tex;
		ready = 1.0f;
		it->second.frame = frameId;
	}

	D3DXMATRIX mW;
	t->GetWorldMatrix(&mW);

	pDev->SetStreamSource(0, mesh->pVB, 0, sizeof(VERTEX_2TEX));
	pDev->SetIndices(mesh->pIB);
	pMask->SetVSConstants(hMaskWorld, &mW, sizeof(mW));
	pMask->SetPSConstants(hReady, &ready, sizeof(ready));
	pMask->SetTexture(hTex, tex, IPF_CLAMP | (bFilter ? IPF_LINEAR : IPF_POINT));
	pMask->UpdateTextures();
	pDev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, mesh->nv, 0, mesh->nf);
}

} // namespace THZ


// ============================================================================
// Entry points (invoked from Surfmgr2.cpp / D3D9Client.cpp)
// ============================================================================
void TerrainHorizon_RenderMask (TileManager2<SurfTile> *mgr, bool use_zbuf)
{
	if (!THZ::g_cfgValid) THZ::ReadCfg();
	static int s_logState = 0;
	if (!THZ::g_cfg.enabled) {
		if (s_logState == 0) { s_logState = 1; oapiWriteLogV("TerrainHorizon: disabled by config"); }
		return;
	}
	if (!mgr) return;
	vPlanet *vp = mgr->GetPlanet();
	if (!vp) { if (s_logState < 2) { s_logState = 2; oapiWriteLogV("TerrainHorizon: no camera planet"); } return; }
	LPDIRECT3DDEVICE9 dev = mgr->Dev();
	if (!dev) return;

	Scene *scene = mgr->GetScene();
	if (scene && scene->GetRenderPass() != RENDERPASS_MAINSCENE) return;
	// The horizon overlay is a per camera-planet feature: the tile manager is
	// also rendered for other bodies visible in the distance (e.g. the Moon from
	// the Earth surface). Ignore those: a body switch would rebuild the whole
	// bake/cache state and thrash the worker threads.
	if (scene && scene->GetCameraProxyVisual() != vp) return;

	THZ::System &sys = THZ::g_sys;
	if (!sys.Init(dev)) {
		if (s_logState < 3) { s_logState = 3; oapiWriteLogV("TerrainHorizon: init failed -> disabled"); }
		return;
	}
	double camAlt = (scene ? scene->GetCameraAltitude() : 0.0);
	if (!sys.EnsurePlanet(mgr, vp)) {
		if (s_logState < 4) { s_logState = 4; oapiWriteLogV("TerrainHorizon: planet setup failed"); }
		return;
	}
	if (s_logState < 5) {
		s_logState = 5;
		oapiWriteLogV("TerrainHorizon: mask pass active (alt=%.0f m, R=%d, K=%d, debug=%d, zbuf=%d)",
			camAlt, THZ::g_cfg.fieldSize, THZ::g_cfg.dirs, THZ::g_cfg.debug, use_zbuf ? 1 : 0);
	}

	sys.BeginFrame(mgr, vp, camAlt);

	if (!use_zbuf) return;						// overlay only in the z-buffered near surface mode
	if (camAlt > THZ::g_cfg.maxAlt) return;

	sys.BeginPass(dev, vp);
	for (int i = 0; i < 2; i++) THZ::DrawMaskNode(mgr->TileTree(i), sys);
	sys.EndPass(dev);
}

void TerrainHorizon_Exit ()
{
	THZ::g_sys.Exit();
	THZ::g_cfgValid = false;
}

#endif // TERRAINRENDER_HORIZON
#endif // D3D9CLIENT_TERRAINRENDER_INC

