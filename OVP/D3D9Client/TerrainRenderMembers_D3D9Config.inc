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

