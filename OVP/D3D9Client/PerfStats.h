// ============================================================================
// PerfStats.h (D3D9Client)
//
// Lightweight frame profiler used to locate rendering bottlenecks. The
// implementation lives in PerfStats.inc (included by D3D9Client.cpp only);
// this header is included by the renderer sources that need the stage marks.
//
// Usage:
//   D3D9Client.cfg:  PerfLog = 0   off (default, zero overhead)
//                    PerfLog = 1   CPU stage timings, logged to Orbiter.log
//                                  every 2 seconds (safe for normal play)
//                    PerfLog = 2   additionally profiles the GPU with D3D9
//                                  event queries in short windows. The query
//                                  synchronisation stalls the pipeline while a
//                                  window runs (a few frames every 8 s), so
//                                  the frame rate of the profiled frames is
//                                  not representative.
//
// Output lines in Orbiter.log start with "PERF:" and contain the frame rate,
// the CPU time spent in each rendering stage, the GPU time per stage (level 2)
// and per frame counters (terrain tiles, shadow caster tiles, draw calls,
// vertices). All times are in milliseconds.
//
// Feature hook file, kept separate from the official sources so that the
// official renderer code only carries the one-line stage marks.
// ============================================================================
#ifndef __PERF_STATS_H
#define __PERF_STATS_H

#include <d3d9.h>

// ---------------------------------------------------------------------------
// Rendering stages. The marks are placed in the render order; the time between
// two marks is attributed to the stage of the first mark. A stage may therefore
// appear several times per frame (e.g. PLANET_PRE for each rendered planet),
// the timings are summed.
// ---------------------------------------------------------------------------
enum PerfStage {
	PERF_STAGE_SETUP = 0,		// camera/visibility bookkeeping before the scene
	PERF_STAGE_SECSCENE,		// custom camera / env map / irradiance probes
	PERF_STAGE_GBUFFER,			// normal+depth pre-pass (SSAO, visibility)
	PERF_STAGE_CELSPHERE,		// star field / celestial sphere background
	PERF_STAGE_SHADOWG0,		// near field (G0) vessel shadow map, early pass
	PERF_STAGE_PLANET_PRE,		// planet setup before the surface render
	PERF_STAGE_ATMO,			// atmosphere scatter table generation
	PERF_STAGE_TERRAIN_LOCK,	// terrain: waiting for the tile loader mutex
	PERF_STAGE_TERRAIN_PROCESS,	// terrain: tile tree update (LOD, preload)
	PERF_STAGE_TERRAIN_DRAW,	// terrain: tile drawing (+ fade tiles)
	PERF_STAGE_TERRAIN_TAIL,	// terrain: fade bookkeeping, mutex release
	PERF_STAGE_PLANET_POST,		// planet: base surfaces, clouds, remaining planets
	PERF_STAGE_MARKERS,			// planetarium art / surface markers / labels
	PERF_STAGE_SHADOWG0B,		// second G0 shadow map + vessels inside shadows
	PERF_STAGE_VESSELS,			// vessel rendering
	PERF_STAGE_SUBSYS,			// exhausts, beacons, particles, axis vectors
	PERF_STAGE_COCKPIT,			// virtual cockpit / internal parts
	PERF_STAGE_POSTFX,			// offscreen copy, relativity, bloom, glares
	PERF_STAGE_HUD,				// HUD / 2D overlay / GDI overlay
	PERF_STAGE_DEBUGOVL,		// debug visualisations and debug text
	PERF_STAGE_SCENE_OTHER,		// remaining work of clbkRenderScene
	PERF_STAGE_COUNT
};

#define PERF_MAX_SEG  192		// GPU query segments per frame (increase if needed)

// Non-zero while profiling is enabled (checked by the inline stage marks).
extern bool PerfActive;

// Frame boundaries and stage marks (all implemented in PerfStats.inc).
void Perf_BeginFrame(IDirect3DDevice9 *dev, double camera_alt);
void Perf_MarkInternal(int stage);
void Perf_EndFrame();
void Perf_AddUpdate(double us);		// clbkUpdate (scene/visual update) duration
void Perf_AddPresent(double us);	// Present duration
void Perf_AddTerrainTileInternal();

inline void Perf_Mark(int stage)			{ if (PerfActive) Perf_MarkInternal(stage); }
inline void Perf_AddTerrainTile()			{ if (PerfActive) Perf_AddTerrainTileInternal(); }

#endif // __PERF_STATS_H
