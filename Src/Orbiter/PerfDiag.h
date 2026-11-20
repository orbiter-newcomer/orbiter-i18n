// ============================================================================
// PerfDiag.h (Orbiter core)
//
// TEMPORARY diagnostic instrumentation for the performance work: breaks the
// per-frame simulation update down into its phases (module callbacks, particle
// system update, rigid body propagation, relativistic gravity, contact solver,
// post-update collisions, ...) and counts propagator substeps.
//
// The accumulators are read and logged in Orbiter::Run() ("PERF UPD:" line in
// Orbiter.log, every 2 seconds). Remove this file and all PERFDIAG_* marked
// blocks when the performance work is finished.
//
// Included by Orbiter.cpp, Psys.cpp, RigidBody.cpp and Vessel.cpp (all part of
// the Orbiter.exe target); C++17 inline variables keep a single instance.
// ============================================================================
#ifndef __PERF_DIAG_H
#define __PERF_DIAG_H

#include <windows.h>
#include <atomic>

namespace PerfDiag {

// ---------------------------------------------------------------------------
// Accumulators (all in microseconds, accumulated per logging window)
// ---------------------------------------------------------------------------
struct Accum {
	// Orbiter::UpdateWorld / EndTimeStep phases
	double preStep;			// module clbkPreStep
	double psysUpdate;		// PlanetarySystem::Update
	double wreck;			// WreckDebrisUpdate
	double dialogs;			// dialogs update
	double postStep;		// module clbkPostStep (vessel + plugin)
	double finalise;		// PlanetarySystem::FinaliseUpdate
	double clbkUpdate;		// graphics client clbkUpdate (visual update)
	double tail;			// remaining-update / EndTimeStep bookkeeping

	// inside PlanetarySystem::Update / FinaliseUpdate
	double psyBodies;		// celestial body state update
	double psyCelestials;	// celestials update
	double psySuper;		// supervision vessel update
	double psyVessels;		// vessel state update (all vessels)
	double psyBodyForces;	// vessel force update (gravity etc.)
	double psyFinalBodies;	// body EndStateUpdate / FinaliseUpdate
	double psyFinalSuper;
	double psyFinalVessels;	// vessel PostUpdate (collisions, docking, ...)

	// inside RigidBody::Update (dynamic propagation)
	double rbUpdate;		// whole dynamic part
	double rbGrav;			// gravity acceleration evaluations (GetIntermediateMoments)
	double rbProp;			// propagator steps (incl. nested gravity)
	double rbContact;		// surface contact / touchdown handling

	// vessel module callbacks
	double modulePost;		// per-vessel module clbkPostStep (DG, Shuttle, ...)
	double modulePre;		// per-vessel module clbkPreStep
	double pluginPost;		// plugin module clbkPostStep
	double pluginPre;		// plugin module clbkPreStep

	// vessel surface/contact forces (Vessel::AddSurfaceForces)
	double vesSurf;			// total AddSurfaceForces time
	double surfCalls;		// number of AddSurfaceForces calls
	double vesBodyFor;		// Vessel::UpdateBodyForces total

	// Vessel::Update breakdown
	double updGuard;		// relativity numeric/strong-field/FTL guards
	double updBurn;			// atmospheric burn-up check
	double updState;		// freeflight/landed state update (incl. RigidBody::Update)
	double updSurfPrm;		// UpdateSurfParams
	double updBaseCol;		// base mesh collision test
	double updRest;			// remaining Vessel::Update work

	// Vessel::PostUpdate breakdown
	double postBase;		// VesselBase::PostUpdate
	double postColl;		// vessel-vessel collision resolution
	double postDock;		// docking scan
	double postRec;			// flight recorder
	double postRest;		// remaining PostUpdate work

	// counters
	double dynVessels;		// number of dynamically propagated vessels
	double substeps;		// total propagator substeps
	int    maxSubsteps;		// max substeps of a single vessel this window

	int    frames;			// frames in this logging window
};

// top-N tracking of the most expensive vessels
struct Burst {
	double cost[8];
	int    idx[8];
	int    n;
};

inline Accum g;
inline Burst worstUpdate;
inline Burst worstPost;
inline Burst worstModule;

// While the vessel update runs on worker threads the accumulators above must
// not be written concurrently: the parallel sections suspend the diagnostics
// and the wall-clock time of the parallel section is measured by the caller.
// atomic: in dual-thread mode the physics thread toggles it while the main
// thread may execute timing scopes.
inline std::atomic<bool> g_suspended = false;
inline void Suspend() { g_suspended.store(true); }
inline void Resume() { g_suspended.store(false); }

// ---------------------------------------------------------------------------
inline double Now()
{
	static LONGLONG freq = 0;
	LARGE_INTEGER c;
	if (!freq) QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	QueryPerformanceCounter(&c);
	return double(c.QuadPart) * 1e6 / double(freq);
}

inline void Add(double &acc, double t0) { if (!g_suspended) acc += Now() - t0; }

// RAII scope timer: accumulates the lifetime of the scope into acc.
struct Scope {
	double t0;
	double &acc;
	Scope(double &a) : acc(a), t0(Now()) {}
	~Scope() { if (!g_suspended) acc += Now() - t0; }
};

inline void Track(Burst &b, int idx, double us)
{
	if (g_suspended) return;
	if (b.n < 8) {
		b.cost[b.n] = us;
		b.idx[b.n] = idx;
		b.n++;
		return;
	}
	// replace the cheapest entry if this one is more expensive
	int w = 0;
	for (int i = 1; i < b.n; i++) if (b.cost[i] < b.cost[w]) w = i;
	if (us > b.cost[w]) { b.cost[w] = us; b.idx[w] = idx; }
}

inline void Reset()
{
	memset(&g, 0, sizeof(g));
	memset(&worstUpdate, 0, sizeof(worstUpdate));
	memset(&worstPost, 0, sizeof(worstPost));
	memset(&worstModule, 0, sizeof(worstModule));
}

} // namespace PerfDiag

#endif // __PERF_DIAG_H
