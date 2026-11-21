// ==============================================================
// TerrainRenderMembers_D3D9Config.inc - project extension member declarations for class D3D9Config.
// Included inside the class body in the official header: the official
// header only grows by the include line.
// ==============================================================

public:
	int TerrainLoadWait;			///< Wait for terrain tiles to load before the simulation starts (0=off, 1=on)
	int TerrainLoadWaitTimeout;		///< Maximum wait time for the terrain load \[s\] (1...120)
	int TerrainLoadWaitMinMs;		///< Minimum duration of the terrain load wait \[ms\] (0...5000), makes the wait visible even when the tiles are cached
	int WaitTerrainLoadCamera;		///< Re-run the terrain load wait when the camera target changes (0=off, 1=on)

public:
	int PerfLog;					///< Frame profiler: 0=off, 1=CPU stage timings, 2=+GPU timings (logged to Orbiter.log every 2 s)

public:
	int TerrainPreloadLevels;		///< Terrain LOD preloading: load tiles this many levels ahead of the render requirement (0=off, 1=default, 2)
	int TerrainTransitionMs;		///< Terrain LOD cross fade transition time \[ms\] (0=off, 50...2000, default=0)
	int TerrainElevMinLevel;		///< Terrain elevation loading: above TerrainElevSkipAlt do not read elevation files for tiles below this LOD level (0=off, default=10)
	int TerrainElevSkipAlt;			///< Camera altitude \[m\] above which the elevation files are skipped for the low LOD tiles (0=always apply, default=50000)
	int TerrainLoadThreads;			///< Terrain tile loader threads (1...4, default=1)

public:
	int  RelCacheEnabled;			///< Relativity render cache: skip the cubemap/composite rendering when the input parameters are unchanged (0=off, 1=on, default=1)

public:
	// Terrain self shadowing via horizon angle field overlay (see TerrainRender.inc,
	// part TERRAINRENDER_HORIZON). The overlay is a second terrain pass that
	// multiplies the sun visibility of each terrain pixel (0=shadow, 1=lit).
	int TerrainHorizonShadow;		///< Enable terrain horizon field self shadowing overlay (0=off, 1=on, default=1)
	int TerrainHorizonDebug;		///< Debug view: 0=off, 1=sun visibility red(dark)/yellow(lit) + altitude contours, 2=horizon angle field + contours, 3=normal shading + contours, default=1
	int TerrainHorizonFieldSize;	///< Horizon field resolution per tile edge (16...128, default 32)
	int TerrainHorizonDirs;			///< Number of horizon azimuth directions (8...32, default 16)
	int TerrainHorizonRangeKm;		///< Maximum horizon search distance \[km\] (50...500, default 300)
	int TerrainHorizonStrength;		///< Shadow strength \[%\] (0...100, default 100)
	int TerrainHorizonThreads;		///< Bake worker threads (1...3, default 1)
	int TerrainHorizonCacheMB;		///< Horizon texture cache budget \[MB\] (8...512, default 64)
	int TerrainHorizonMaxAlt;		///< Camera altitude above which the overlay is disabled \[m\] (default 50000)
	int TerrainHorizonMinLevel;		///< Only bake tiles at this LOD level or finer (default 4)
	int TerrainHorizonUpdate;		///< Tile requirement scan interval \[frames\] (1...16, default 4)
	int TerrainHorizonContour;		///< Debug contour line interval \[m\] in the overlay debug views (0=off, 10...2000, default 200)
	int TerrainHorizonShadowFloor;	///< Minimum brightness kept inside the horizon shadows \[%\] (0...90, default 40): the shadowed terrain keeps this fraction of the lit brightness (sky/ambient light), so the shadow centre is never pitch black

