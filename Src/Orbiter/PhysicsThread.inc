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
// seen by the main thread between two physics frames and logs discontinuities
// that are physically impossible (a velocity/position jump larger than the
// applied acceleration/time step can explain). Also verifies that the stable
// state does not change while the physics thread is running next to a render.
// ============================================================================

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
static int    s_sbeFrames = 0;

void Orbiter::RunDualStageB()
{
	LARGE_INTEGER freq, c0, c1;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&c0);


	// camera/state part of WreckDebrisUpdate (originally after g_psys->Update)
	WreckDebrisUpdate();
	// dialog updates (main thread only)
	if (pDlgMgr) pDlgMgr->UpdateDialogs();
	// module post-step callbacks: plugin/vessel code may create or delete
	// vessels, touch panels or the graphics client, so keep them here
	if (bRunning) ModulePostStep();
	// close the state update window (original position: end of UpdateWorld)
	g_bStateUpdate = false;
	// queued wreck vessel creation (needs the graphics client/main thread)
	WreckProcessPendingSafe();
	// kill vessels marked for deletion (also notifies the graphics client)
	if (!KillVessels()) if (hRenderWnd) DestroyWindow(hRenderWnd);
	// finalise update, advance time, camera/panel/visual update, session limits
	EndTimeStep(bRunning);

	// physics diagnostics: the accumulators are only read while the physics
	// thread is parked, so no locking is required

	QueryPerformanceCounter(&c1);
	m_dualSyncBWait += double(c1.QuadPart - c0.QuadPart) * 1e3 / double(freq.QuadPart);
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

		// stable state seen by the renderer does not change during the render
		auto RenderFinishedFrame = [this]() {
			if (!bVisible) return;
			LARGE_INTEGER freq, c0, c1;
			QueryPerformanceFrequency(&freq);
			QueryPerformanceCounter(&c0);
			if (FAILED(Render3DEnvironment()))
				if (hRenderWnd) DestroyWindow(hRenderWnd);
			QueryPerformanceCounter(&c1);
			m_dualRenderAcc += double(c1.QuadPart - c0.QuadPart) * 1e3 / double(freq.QuadPart);
		};

		// prepare the next physics frame (main thread, physics parked)
		RunDualStageA();
		// let the physics thread run the next frame ...
		ReleasePhysicsFrame();
		// ... and render the previous frame in parallel
		RenderFinishedFrame();
	}

	StopPhysicsThread();
	oapiWriteLogV("[PT] dual-thread main loop end");
	return msg.wParam;
}
