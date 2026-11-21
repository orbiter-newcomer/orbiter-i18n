// ============================================================================
// ParallelUpdate.inc (Orbiter core, performance extension)
//
// Multi-core vessel state update.
//
// The vessel state update (rigid body propagation + surface parameter update)
// is independent for most vessels: gravity and the celestial body states are
// read-only during the update phase, and all vessel interactions (collisions,
// docking, attachments, composite structures, module callbacks) are either
// excluded from the parallel set or solved in a serial phase.
//
// Feature hook: included by the official Psys.cpp. The official file changes
// are limited to the include line and the replacement of two loops in
// PlanetarySystem::Update(). The guard/animation split lives in Vessel.cpp.
//
// Safety rules for the parallel set (conservative):
//   - no attachments, no composite (super) vessel, no attached children
//   - free flight only (landed vessels use the simple landed update)
//   - not in the atmosphere and above the terrain elevation query range,
//     so no terrain/elevation/base collision/contact queries are executed
//   - no navigation mode active (avoids a module callback from a worker thread)
//   - far enough away from every other vessel (possible physical interaction)
//   - no complex (Pines) gravity solver, whose internal state is mutable
//   - the relativity protection guards and the module animation callbacks are
//     executed on the main thread (they may create/destroy objects)
//
// Everything else is updated in sequence on the main thread, so behaviour is
// identical to the single-threaded update for interacting vessels.
// ============================================================================
#ifndef ORBITER_PARALLEL_UPDATE_INC
#define ORBITER_PARALLEL_UPDATE_INC

#include <vector>
#include <algorithm>
#include <ppl.h>

#include "Orbiter.h"	// Orbiter class (configuration access)

extern Orbiter *g_pOrbiter;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static bool PerfParallelVesselSetsEnabled ()
{
	if (!g_pOrbiter || !g_pOrbiter->Cfg()) return false;
	return (g_pOrbiter->Cfg()->CfgPhysicsPrm.bParallelVesselUpdate != 0);
}

static void PerfParallelVesselSets (PlanetarySystem *psys,
	std::vector<Vessel*> &par, std::vector<Vessel*> &ser)
{
	const DWORD n = psys->nVessel();
	par.clear();
	ser.clear();

	const bool bEnabled = PerfParallelVesselSetsEnabled();

	for (DWORD i = 0; i < n; i++) {
		Vessel *v = psys->GetVessel(i);
		if (!v) continue;
		if (bEnabled && v->IsParallelUpdateCandidate()) par.push_back(v);
		else ser.push_back(v);
	}
	if (!bEnabled) return;

	// Any vessel that is within the interaction range of another vessel is
	// updated in sequence together with the rest of the main thread work.
	double rng = g_pOrbiter->Cfg()->CfgPhysicsPrm.ParallelVesselRange;
	if (rng < 1e3) rng = 1e3;
	const double rng2 = rng * rng;

	std::vector<bool> indep(par.size(), true);
	for (size_t a = 0; a < par.size(); a++) {
		for (DWORD i = 0; i < n && indep[a]; i++) {
			Vessel *w = psys->GetVessel(i);
			if (!w || w == par[a]) continue;
			Vector d = par[a]->GPos() - w->GPos();
			if (d.length2() < rng2) indep[a] = false;
		}
	}
	size_t k = 0;
	for (size_t a = 0; a < par.size(); a++) {
		if (indep[a]) par[k++] = par[a];
		else         ser.push_back(par[a]);
	}
	par.resize(k);
}

// ---------------------------------------------------------------------------
// Vessel body forces (thrust/aerodynamics/radiation) for the parallel set
// ---------------------------------------------------------------------------
static void PerfParallelUpdateBodyForces (std::vector<Vessel*> &par, std::vector<Vessel*> &ser)
{
	// interacting / near-planet vessels, and vessels with an active navigation
	// mode (module callback): serial force update on the main thread
	for (size_t i = 0; i < ser.size(); i++) ser[i]->UpdateBodyForces();

	std::vector<Vessel*> fpar;
	fpar.reserve(par.size());
	for (size_t i = 0; i < par.size(); i++) {
		if (par[i]->IsParallelForcesCandidate()) fpar.push_back(par[i]);
		else par[i]->UpdateBodyForces();
	}

	if (fpar.size() >= 2) {
		concurrency::parallel_for(size_t(0), fpar.size(), [&](size_t i) {
			fpar[i]->UpdateBodyForces();
		});
	}
	else {
		for (size_t i = 0; i < fpar.size(); i++) fpar[i]->UpdateBodyForces();
	}
}

// ---------------------------------------------------------------------------
// Vessel state update: guards and module callbacks on the main thread, state
// propagation for the independent vessels on the worker threads.
// ---------------------------------------------------------------------------
static void PerfParallelUpdateVessels (PlanetarySystem *psys, bool force,
	std::vector<Vessel*> &par, std::vector<Vessel*> &ser)
{
	// interacting / near-planet vessels: full serial update
	for (size_t i = 0; i < ser.size(); i++) ser[i]->Update (force);

	// independent vessels: protection guards first (main thread)
	std::vector<Vessel*> cand;
	cand.reserve(par.size());
	for (size_t i = 0; i < par.size(); i++) {
		if (par[i]->RunUpdateGuards()) cand.push_back(par[i]);
	}

	// state propagation on the worker threads
	if (cand.size() >= 2) {
		concurrency::parallel_for(size_t(0), cand.size(), [&](size_t i) {
			cand[i]->UpdatePropagate (force);
		});
	}
	else {
		for (size_t i = 0; i < cand.size(); i++) cand[i]->UpdatePropagate (force);
	}

	// main-thread part of the update (surface parameters, touchdown, comms, ...)
	for (size_t i = 0; i < cand.size(); i++) cand[i]->UpdateFinish ();

	// module animation callbacks (main thread, plugin code must not be
	// executed concurrently)
	for (DWORD i = 0; i < psys->nVessel(); i++) {
		Vessel *v = psys->GetVessel(i);
		if (v) v->UpdateAnimate();
	}
}

#endif // ORBITER_PARALLEL_UPDATE_INC
