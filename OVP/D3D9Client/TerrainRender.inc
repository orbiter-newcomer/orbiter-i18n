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

