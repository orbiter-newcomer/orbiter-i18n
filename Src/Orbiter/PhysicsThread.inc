// ============================================================================
// PhysicsThread.inc (Orbiter core, dual-thread route A)
//
// Route A: the simulation update (PlanetarySystem::Update: relativistic
// propagation / rigid-body dynamics / gravity / subsystems) runs on a
// dedicated physics thread, while the main thread keeps rendering the
// 3D/2D scene (D3D9Client) and the UI.
//
// Thread boundary:
//   physics thread : g_bStateUpdate=true, g_psys->Update()  (state window)
//   main thread    : everything that touches the UI, the graphics client or
//                    creates/destroys simulation objects:
//                    pre/post-step module callbacks, dialog update, wreck
//                    creation, vessel deletion, EndTimeStep (FinaliseUpdate,
//                    camera/panels/graphics client update), snapshot publish.
//
// Per-frame pipeline (1 frame latency):
//
//   main  : Wait(physics N-1) -> StageB(N-1) -> StageA(N) -> Release(physics N)
//                                                                  |
//           Render(N-1)  <---------- in parallel with -----------+
//
// StageB (main thread, physics thread parked):
//   WreckDebrisUpdate, pDlgMgr->UpdateDialogs, ModulePostStep,
//   g_bStateUpdate=false, WreckProcessPendingSafe, KillVessels,
//   EndTimeStep (FinaliseUpdate, td.EndStep,
//   camera/panel/graphics-client update, session limit check)
//
// StageA (main thread, physics thread parked):
//   UpdateRelativityWarp, BeginTimeStep, UserInput, CaptureVideoFrame,
//   console command processing, ModulePreStep, publish flags to the
//   physics thread
//
// Everything here is compiled only when DUAL_THREAD_EXPERIMENT is 1 (the
// official Orbiter.cpp contains the include + a Run() dispatch line).
// ============================================================================

// ---- global thread state (definitions in Orbiter.cpp, also used by
//      Rigidbody.cpp for the Els() time base) ----
extern std::thread::id g_mainThreadId;   // id of the Orbiter main thread
extern bool g_bDualThreadMode;           // true while the physics thread runs

bool OrbiterIsMainThread();

// declarations from WreckDebris.h (included later in Orbiter.cpp)
void WreckDebrisUpdate ();
void WreckProcessPendingSafe ();

// ============================================================================
// Temporary diagnostics for the physics/render jitter investigation
// (DUAL_THREAD_DIAG, see Orbiter.cpp). Records the stable (s0) vessel states
// seen by the main thread between two physics frames and logs discontinuities
// that are physically impossible (a velocity/position jump larger than the
// applied acceleration/time step can explain). Also verifies that the stable
// state does not change while the physics thread is running next to a render.
// ============================================================================
#if DUAL_THREAD_DIAG
#include <vector>
#include <string>

namespace {

	struct DualDiagRec {
		OBJHANDLE h;
		Vector pos, vel;
		std::string name;
	};

	std::vector<DualDiagRec> g_diagPrev;    // stable state of the previous physics frame
	std::vector<DualDiagRec> g_diagRender;  // stable state sampled when the render started
	unsigned long long g_diagFrames = 0;
	unsigned long long g_diagJumps = 0;
	unsigned long long g_diagRenderChanges = 0;
	double g_diagLogT = 0.0;

	void DualDiagCapture (std::vector<DualDiagRec> &out)
	{
		out.clear();
		if (!g_psys) return;
		DWORD n = g_psys->nVessel();
		out.reserve(n);
		for (DWORD i = 0; i < n; i++) {
			Vessel *v = g_psys->GetVessel(i);
			if (!v) continue;
			DualDiagRec r;
			r.h = (OBJHANDLE)v;
			r.pos = v->GPos();
			r.vel = v->GVel();
			// full class name (e.g. "ISS") for the log
			const char *nm = v->Name();
			r.name = (nm ? nm : "?");
			out.push_back(r);
		}
	}

	const DualDiagRec *DualDiagFind (const std::vector<DualDiagRec> &v, OBJHANDLE h)
	{
		for (size_t i = 0; i < v.size(); i++)
			if (v[i].h == h) return &v[i];
		return 0;
	}

	// called at the end of StageB (main thread, physics thread parked)
	void DualDiagStageSample ()
	{
		std::vector<DualDiagRec> cur;
		DualDiagCapture(cur);
		g_diagFrames++;

		if (!g_diagPrev.empty()) {
			for (size_t i = 0; i < cur.size(); i++) {
				const DualDiagRec &c = cur[i];
				const DualDiagRec *p = DualDiagFind(g_diagPrev, c.h);
				if (!p) continue;   // new vessel
				Vector dv = c.vel - p->vel;
				double dvmag = dv.length();
				double dpmag = (c.pos - p->pos).length();
				double vmean = (c.vel.length() + p->vel.length()) * 0.5;
				bool bad = false;
				double expv = 0.0;
				if (!g_pOrbiter->IsRunning()) {
					// paused: the state must not change at all
					bad = (dpmag > 1.0 || dvmag > 1.0);
				} else if (td.SimDT0 > 0.0) {
					double dt = td.SimDT0;
					expv = vmean * dt;
					// A physically explainable change: |dV| <= a_max*dt and
					// |dP| roughly v*dt. The linear estimate is only
					// approximate for relativistic/rotating frames and for
					// variable step sizes, hence the generous position margin;
					// the velocity jump is the reliable indicator.
					double dev = fabs(dpmag - expv);
					bad = (dvmag > 50.0) || (dev > 0.30*expv + 500.0);
				}
				if (bad) {
					g_diagJumps++;
					oapiWriteLogV("[DIAG] STAGEB-JUMP frame=%llu vessel=%s dt=%.4f dP=%.1f expP=%.1f dV=%.2f | pos=(%.1f %.1f %.1f) vel=(%.2f %.2f %.2f)",
						g_diagFrames, c.name.c_str(), td.SimDT0, dpmag, expv, dvmag,
						c.pos.x, c.pos.y, c.pos.z, c.vel.x, c.vel.y, c.vel.z);
				}
			}
		}
		g_diagPrev.swap(cur);
	}

	// called immediately before / after the render call: the stable state must
	// not change while the physics thread computes the next frame
	bool DualDiagRenderWindowBegin ()
	{
		DualDiagCapture(g_diagRender);
		return !g_diagRender.empty();
	}

	void DualDiagRenderWindowEnd ()
	{
		if (g_diagRender.empty()) return;
		std::vector<DualDiagRec> cur;
		DualDiagCapture(cur);
		for (size_t i = 0; i < cur.size(); i++) {
			const DualDiagRec &c = cur[i];
			const DualDiagRec *p = DualDiagFind(g_diagRender, c.h);
			if (!p) continue;
			double dpmag = (c.pos - p->pos).length();
			double dvmag = (c.vel - p->vel).length();
			if (dpmag > 1.0 || dvmag > 0.01) {
				g_diagRenderChanges++;
				// rate-limited: at most ~1 log line per 20 occurrences
				if (g_diagRenderChanges < 20 || (g_diagRenderChanges % 1000) == 0)
					oapiWriteLogV("[DIAG] RENDER-WINDOW CHANGE vessel=%s dP=%.2f dV=%.3f | pos=(%.1f %.1f %.1f)",
						c.name.c_str(), dpmag, dvmag, c.pos.x, c.pos.y, c.pos.z);
			}
		}
		g_diagRender.clear();
	}

	// periodic summary so that the absence of jumps is distinguishable from
	// the diagnostics not running at all
	void DualDiagLog ()
	{
		LARGE_INTEGER freq, c;
		QueryPerformanceFrequency(&freq);
		QueryPerformanceCounter(&c);
		double now = double(c.QuadPart) / double(freq.QuadPart);
		if (g_diagLogT == 0.0) { g_diagLogT = now; return; }
		if (now - g_diagLogT < 2.0) return;
		oapiWriteLogV("[DIAG] frames=%llu jumps=%llu renderChanges=%llu",
			g_diagFrames, g_diagJumps, g_diagRenderChanges);
		g_diagLogT = now;
	}

} // namespace
#endif // DUAL_THREAD_DIAG

// ---- physics thread: one simulation state update ----
void Orbiter::UpdatePhysicsFrameDual()
{
	g_bStateUpdate = true;
	if (m_dualPhysRunning.load() && td.SimDT) {
		if (m_dualPhysPlayback.load()) FRecorder_Play();
		g_psys->Update(m_dualPhysForceUpdate.load());
	}
	// g_bStateUpdate is cleared by the main thread at the end of StageB, so
	// that the update window spans the same phases as in the single-threaded
	// loop (update -> post-step callbacks).
}

// ---- physics thread main loop ----
void Orbiter::PhysicsThreadProc()
{
	oapiWriteLogV("[PT] physics thread started");
	LARGE_INTEGER freq, c0, c1;
	QueryPerformanceFrequency(&freq);
	for (;;) {
		std::unique_lock<std::mutex> lk(m_dualMutex);
		m_dualCv.wait(lk, [this] { return m_dualThreadQuit || m_dualGateOpen; });
		if (m_dualThreadQuit) break;
		m_dualGateOpen = false;
		bool active = m_dualSessionActive;
		lk.unlock();

		if (active) {
			QueryPerformanceCounter(&c0);
			UpdatePhysicsFrameDual();
			QueryPerformanceCounter(&c1);
			double dt = double(c1.QuadPart - c0.QuadPart) * 1e3 / double(freq.QuadPart);
			m_dualPhysTimeAcc += dt;
		}

		lk.lock();
		m_dualFrameReady = true;
		lk.unlock();
		m_dualCv.notify_all();
	}
	oapiWriteLogV("[PT] physics thread exited");
}

// ---- main thread: start the physics thread for a new session ----
void Orbiter::EnsurePhysicsThread()
{
	if (m_physicsThread.joinable()) return;

	g_mainThreadId = std::this_thread::get_id();
	{
		std::lock_guard<std::mutex> lk(m_dualMutex);
		m_dualThreadQuit = false;
		m_dualFrameReady = false;
		m_dualGateOpen = false;
		m_dualSessionActive = true;
		m_dualPhysRunning.store(bRunning);
		m_dualPhysForceUpdate.store(g_bForceUpdate);
		m_dualPhysPlayback.store(bPlayback);
		m_dualPhysTimeAcc = 0.0;
		m_dualSyncAWait = m_dualSyncBWait = 0.0;
		m_dualWaitAcc = 0.0;
		m_dualRenderAcc = 0.0;
		m_dualPerfFrames = 0;
		m_dualPerfStart = 0.0;
		m_dualPerfLast = 0.0;
		m_dualPhysLogT = 0.0;
	}
	g_bDualThreadMode = true;
	m_physicsThread = std::thread(&Orbiter::PhysicsThreadProc, this);
	oapiWriteLogV("[PT] physics thread launched");
}

// ---- main thread: stop the physics thread (idempotent) ----
void Orbiter::StopPhysicsThread()
{
	if (!m_physicsThread.joinable()) {
		g_bDualThreadMode = false;
		return;
	}
	{
		std::lock_guard<std::mutex> lk(m_dualMutex);
		m_dualThreadQuit = true;
		m_dualSessionActive = false;
		m_dualGateOpen = true;
	}
	m_dualCv.notify_all();
	m_physicsThread.join();
	g_bDualThreadMode = false;
	oapiWriteLogV("[PT] physics thread stopped");
}

// ---- main thread: wait until the physics thread finished its frame ----
// Pumps the Windows message queue while waiting (the render window may need
// message processing, and a WM_CLOSE/WM_QUIT must be able to abort the wait).
bool Orbiter::WaitPhysicsFrame()
{
	LARGE_INTEGER freq, c0, c1;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&c0);

	std::unique_lock<std::mutex> lk(m_dualMutex);
	while (!m_dualFrameReady) {
		if (m_dualThreadQuit) return false;
		m_dualCv.wait_for(lk, std::chrono::milliseconds(2));
		if (m_dualFrameReady) break;
		lk.unlock();
		MSG msg;
		while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				oapiWriteLogV("[PT] WM_QUIT while waiting for physics frame");
				return false;
			}
			if (!m_pLaunchpad || !m_pLaunchpad->ConsumeMessage(&msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		lk.lock();
	}
	lk.unlock();
	QueryPerformanceCounter(&c1);
	m_dualWaitAcc += double(c1.QuadPart - c0.QuadPart) * 1e3 / double(freq.QuadPart);
	return true;
}

// ---- main thread: let the physics thread run the next frame ----
void Orbiter::ReleasePhysicsFrame()
{
	{
		std::lock_guard<std::mutex> lk(m_dualMutex);
		m_dualFrameReady = false;
		m_dualGateOpen = true;
	}
	m_dualCv.notify_all();
}

// ---- StageB: finish the physics frame (main thread, physics parked) ----
// TEMPORARY per-phase timers for the main-thread frame time diagnostics
static double s_sbeWreck = 0.0, s_sbeDlg = 0.0, s_sbeModule = 0.0;
static double s_sbeKill = 0.0, s_sbePublish = 0.0, s_sbeEnd = 0.0;
static int    s_sbeFrames = 0;

void Orbiter::RunDualStageB()
{
	LARGE_INTEGER freq, c0, c1;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&c0);

	LARGE_INTEGER p0, p1;
#define SBE_BEGIN() QueryPerformanceCounter(&p0)
#define SBE_END(acc) do { QueryPerformanceCounter(&p1); acc += double(p1.QuadPart - p0.QuadPart) * 1e3 / double(freq.QuadPart); } while (0)

	// camera/state part of WreckDebrisUpdate (originally after g_psys->Update)
	SBE_BEGIN(); WreckDebrisUpdate(); SBE_END(s_sbeWreck);
	// dialog updates (main thread only)
	SBE_BEGIN(); if (pDlgMgr) pDlgMgr->UpdateDialogs(); SBE_END(s_sbeDlg);
	// module post-step callbacks: plugin/vessel code may create or delete
	// vessels, touch panels or the graphics client, so keep them here
	SBE_BEGIN(); if (bRunning) ModulePostStep(); SBE_END(s_sbeModule);
	// close the state update window (original position: end of UpdateWorld)
	g_bStateUpdate = false;
	// queued wreck vessel creation (needs the graphics client/main thread)
	WreckProcessPendingSafe();
	// kill vessels marked for deletion (also notifies the graphics client)
	SBE_BEGIN(); if (!KillVessels()) if (hRenderWnd) DestroyWindow(hRenderWnd); SBE_END(s_sbeKill);
	// finalise update, advance time, camera/panel/visual update, session limits
	SBE_BEGIN(); EndTimeStep(bRunning); SBE_END(s_sbeEnd);
	s_sbeFrames++;
#undef SBE_BEGIN
#undef SBE_END

	// physics diagnostics: the accumulators are only read while the physics
	// thread is parked, so no locking is required
	PerfDiag::g.frames++;
	UpdateDualPhysicsLog();
#if DUAL_THREAD_DIAG
	DualDiagStageSample();
#endif

	QueryPerformanceCounter(&c1);
	m_dualSyncBWait += double(c1.QuadPart - c0.QuadPart) * 1e3 / double(freq.QuadPart);
}

// ---- physics breakdown log (main thread, physics parked) ----
void Orbiter::UpdateDualPhysicsLog()
{
	LARGE_INTEGER freq, c;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&c);
	double now = double(c.QuadPart) / double(freq.QuadPart);
	if (m_dualPhysLogT == 0.0) {
		m_dualPhysLogT = now;
		return;
	}
	if (now - m_dualPhysLogT < 2.0) return;

	const int nf = PerfDiag::g.frames;
	if (nf > 0) {
		char buf[640];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE,
			"[PT] UPD: frames=%d | pre=%.2f psys=%.2f post=%.2f fin=%.2f clbk=%.2f tail=%.2f"
			" | psyVes=%.2f rbGrav=%.2f rbProp=%.2f bfor=%.2f"
			" | subs=%.0f dyn=%.1f",
			nf,
			PerfDiag::g.preStep / nf * 1e-3, PerfDiag::g.psysUpdate / nf * 1e-3,
			PerfDiag::g.postStep / nf * 1e-3, PerfDiag::g.finalise / nf * 1e-3,
			PerfDiag::g.clbkUpdate / nf * 1e-3, PerfDiag::g.tail / nf * 1e-3,
			PerfDiag::g.psyVessels / nf * 1e-3, PerfDiag::g.rbGrav / nf * 1e-3,
			PerfDiag::g.rbProp / nf * 1e-3, PerfDiag::g.psyBodyForces / nf * 1e-3,
			PerfDiag::g.substeps / nf, PerfDiag::g.dynVessels / nf);
		oapiWriteLogV("%s", buf);

		// TEMPORARY diagnostics: Vessel::Update / PostUpdate breakdown [ms/frame]
		oapiWriteLogV("[PT] UPD2: guard=%.2f burn=%.2f state=%.2f surfPrm=%.2f baseCol=%.2f rest=%.2f"
			" | vesSurf=%.2f surfCalls=%.1f bodyFor=%.2f simT=%.4f"
			" | postBase=%.2f postColl=%.2f postDock=%.2f postRec=%.2f postRest=%.2f finVes=%.2f"
			" | modPost=%.2f modPre=%.2f plugPost=%.2f plugPre=%.2f",
			PerfDiag::g.updGuard / nf * 1e-3, PerfDiag::g.updBurn / nf * 1e-3,
			PerfDiag::g.updState / nf * 1e-3, PerfDiag::g.updSurfPrm / nf * 1e-3,
			PerfDiag::g.updBaseCol / nf * 1e-3, PerfDiag::g.updRest / nf * 1e-3,
			PerfDiag::g.vesSurf / nf * 1e-3, PerfDiag::g.surfCalls / nf,
			PerfDiag::g.vesBodyFor / nf * 1e-3, td.SimDT0,
			PerfDiag::g.postBase / nf * 1e-3, PerfDiag::g.postColl / nf * 1e-3,
			PerfDiag::g.postDock / nf * 1e-3, PerfDiag::g.postRec / nf * 1e-3,
			PerfDiag::g.postRest / nf * 1e-3, PerfDiag::g.psyFinalVessels / nf * 1e-3,
			PerfDiag::g.modulePost / nf * 1e-3, PerfDiag::g.modulePre / nf * 1e-3,
			PerfDiag::g.pluginPost / nf * 1e-3, PerfDiag::g.pluginPre / nf * 1e-3);
	}
	PerfDiag::Reset();
	m_dualPhysLogT = now;
}

// ---- StageA: prepare the next physics frame (main thread, physics parked) ----
void Orbiter::RunDualStageA()
{
	LARGE_INTEGER freq, c0, c1;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&c0);

	// relativistic time acceleration (updates td warp; original call site is
	// the main loop, just before BeginTimeStep)
	UpdateRelativityWarp();

	if (BeginTimeStep(bRunning)) {
		// process user input (keyboard/joystick/attitude controls). In the
		// single-threaded loop this runs after EndTimeStep and before Render.
		if (bVisible) {
			if (bAllowInput) bActive = true, bAllowInput = false;
			if (bActive) UserInput();
		}
		if (bRunning && bCapture) CaptureVideoFrame();
	}
	if (m_pConsole) m_pConsole->ParseCmd();
	// module pre-step callbacks (same reason as ModulePostStep in StageB)
	if (bRunning) ModulePreStep();

	// publish the flags the physics thread needs for the next frame
	m_dualPhysRunning.store(bRunning);
	m_dualPhysForceUpdate.store(g_bForceUpdate);
	m_dualPhysPlayback.store(bPlayback);

	QueryPerformanceCounter(&c1);
	m_dualSyncAWait += double(c1.QuadPart - c0.QuadPart) * 1e3 / double(freq.QuadPart);
}

// ---- per-frame performance log (main thread, physics parked) ----
void Orbiter::UpdateDualPerfLog()
{
	LARGE_INTEGER freq, c;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&c);
	double now = double(c.QuadPart) / double(freq.QuadPart);

	if (m_dualPerfStart == 0.0) {
		m_dualPerfStart = now;
		m_dualPerfLast = now;
		return;
	}
	m_dualPerfFrames++;
	if (now - m_dualPerfLast < 2.0) return;

	double span = now - m_dualPerfStart;
	int nf = m_dualPerfFrames;
	double fifo = nf / span;
	double phys = m_dualPhysTimeAcc / nf;
	double syncA = m_dualSyncAWait / nf;
	double syncB = m_dualSyncBWait / nf;
	double wait = m_dualWaitAcc / nf;
	double rnd = m_dualRenderAcc / nf;
	oapiWriteLogV("[PT] PERF: fps=%.1f phys=%.2f sA=%.2f sB=%.2f wait=%.2f rnd=%.2f frames=%d",
		fifo, phys, syncA, syncB, wait, rnd, nf);
	if (s_sbeFrames > 0) {
		double inv = 1.0 / double(s_sbeFrames);
		oapiWriteLogV("[PT] SBE: wreck=%.2f dlg=%.2f mod=%.2f kill=%.2f pub=%.2f end=%.2f frames=%d",
			s_sbeWreck * inv, s_sbeDlg * inv, s_sbeModule * inv,
			s_sbeKill * inv, s_sbePublish * inv, s_sbeEnd * inv, s_sbeFrames);
		s_sbeWreck = s_sbeDlg = s_sbeModule = s_sbeKill = s_sbePublish = s_sbeEnd = 0.0;
		s_sbeFrames = 0;
	}
	m_dualPhysTimeAcc = m_dualSyncAWait = m_dualSyncBWait = 0.0;
	m_dualWaitAcc = m_dualRenderAcc = 0.0;
	m_dualPerfFrames = 0;
	m_dualPerfStart = now;
	m_dualPerfLast = now;
}

// ---- main-thread frame loop of the dual-thread mode ----
INT Orbiter::RunDualThread()
{
	oapiWriteLogV("[PT] dual-thread main loop start (route A: physics thread + main-thread rendering)");

	if (!pConfig->CfgCmdlinePrm.LaunchScenario.empty())
		Launch(pConfig->CfgCmdlinePrm.LaunchScenario.c_str());

	MSG msg;
	PeekMessage(&msg, NULL, 0U, 0U, PM_NOREMOVE);

	while (WM_QUIT != msg.message) {
		BOOL bGotMsg;
		if (bSession) {
			bGotMsg = PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE);
		} else {
			bGotMsg = GetMessage(&msg, NULL, 0U, 0U);
		}
		if (bGotMsg) {
			if (!m_pLaunchpad || !m_pLaunchpad->ConsumeMessage(&msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			continue;
		}

		if (!bSession) {
			// back on the launchpad: the physics thread must be gone
			StopPhysicsThread();
			continue;
		}

		// ---- session active ----
		bool firstFrame = !m_physicsThread.joinable();
		EnsurePhysicsThread();
		if (firstFrame) {
			// first frame: prepare time step / pre-step callbacks, then let
			// the physics thread compute the first state
			RunDualStageA();
			ReleasePhysicsFrame();
		}

		// wait for the physics frame (physics thread parks afterwards)
		if (!WaitPhysicsFrame()) break;
		if (!bSession) continue;

		// post-physics phase (main thread, physics parked)
		RunDualStageB();
		if (!bSession) continue;

		// render the finished frame (timed); DUAL_THREAD_DIAG checks that the
		// stable state seen by the renderer does not change during the render
		auto RenderFinishedFrame = [this]() {
			if (!bVisible) return;
			LARGE_INTEGER freq, c0, c1;
			QueryPerformanceFrequency(&freq);
			QueryPerformanceCounter(&c0);
#if DUAL_THREAD_DIAG
			DualDiagRenderWindowBegin();
#endif
			if (FAILED(Render3DEnvironment()))
				if (hRenderWnd) DestroyWindow(hRenderWnd);
			QueryPerformanceCounter(&c1);
			m_dualRenderAcc += double(c1.QuadPart - c0.QuadPart) * 1e3 / double(freq.QuadPart);
#if DUAL_THREAD_DIAG
			DualDiagRenderWindowEnd();
#endif
		};

#if DUAL_THREAD_SERIAL_EXPERIMENT
		// diagnosis: render while the physics thread is parked (no overlap).
		// If the jitter/phantom-momentum disappears here, it is caused by the
		// main thread racing the physics thread during the render.
		RenderFinishedFrame();
		// prepare the next physics frame, then let the physics thread run
		RunDualStageA();
		UpdateDualPerfLog();
		ReleasePhysicsFrame();
#else
		// prepare the next physics frame (main thread, physics parked)
		RunDualStageA();
		// let the physics thread run the next frame ...
		ReleasePhysicsFrame();
		// ... and render the previous frame in parallel
		RenderFinishedFrame();
		UpdateDualPerfLog();
#endif
#if DUAL_THREAD_DIAG
		DualDiagLog();
#endif
	}

	StopPhysicsThread();
	oapiWriteLogV("[PT] dual-thread main loop end");
	return msg.wParam;
}
