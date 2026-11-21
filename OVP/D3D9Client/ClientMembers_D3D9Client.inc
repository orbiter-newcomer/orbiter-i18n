// ==============================================================
// ClientMembers_D3D9Client.inc - project extension members of class D3D9Client.
// Included inside the class body in the official header.
// ==============================================================

private:
	// Terrain load wait: keep the simulation paused at startup until the
	// initial terrain around the camera has been loaded by the tile loaders.
	void UpdateTerrainLoadWait ();
	void DrawTerrainLoadWaitLabel ();
	void ResetTerrainLoadWait ();
	bool ShouldWaitForCameraLoad ();
	void UpdateCameraTargetRecord ();

	int    terrainWaitState;   // 0=idle, 1=warm-up, 2=waiting for terrain, 3=done
	int    terrainWaitFrames;  // frames rendered since the simulation start
	bool   terrainWaitPaused;  // true if we paused the simulation ourselves
	double terrainWaitT0;      // start time of the wait [s]
	int    terrainWaitStable;  // consecutive frames without pending tiles
	OBJHANDLE camTargetLast;   // camera target handle at the previous frame
	OBJHANDLE focusLast;       // focus object handle at the previous frame
	int    camModeLast;        // camera mode at the previous frame
	OBJHANDLE lastTargetRefBody; // central body of that target
	double lastTargetLng, lastTargetLat, lastTargetAlt; // target position (planet-relative)
	bool   lastTargetValid;    // whether the previous-target record is valid
	double terrainWaitCooldown; // earliest time for the next camera-triggered wait [s]
	double lastSimTime;         // simulation time of the previous frame (new-sim detection)

