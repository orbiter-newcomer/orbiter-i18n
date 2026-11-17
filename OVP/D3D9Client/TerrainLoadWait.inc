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
#ifndef D3D9CLIENT_TERRAINLOADWAIT_INC
#define D3D9CLIENT_TERRAINLOADWAIT_INC

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
		if (oapiGetPause()) { terrainWaitState = 3; break; }	// user requested a paused start
		oapiSetPause (true);
		terrainWaitPaused = true;
		terrainWaitState  = 2;
		terrainWaitStable = 0;
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
			if (terrainWaitPaused) oapiSetPause (false);
			terrainWaitPaused   = false;
			terrainWaitCooldown = oapiGetSysTime();
			terrainWaitState    = 3;
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
	lastTarget      = oapiCameraTarget ();
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

#endif // D3D9CLIENT_TERRAINLOADWAIT_INC
