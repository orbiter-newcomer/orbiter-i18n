// ==============================================================
// OrbiterMembers_Orbiter.inc - project extension member declarations for class Orbiter.
// Included inside the class body in the official header: the official
// header only grows by the include line.
// ==============================================================

protected:
	// ---- dual thread route A: physics thread + main-thread rendering ----
	// (enabled by DUAL_THREAD_EXPERIMENT in Orbiter.cpp; 0 = single-threaded fallback)
	// Physics thread: UpdatePhysicsFrameDual() (PlanetarySystem::Update only).
	// Main thread: StageA (pre-step), StageB (post-step/EndTimeStep) and the
	// D3D9Client rendering. See PhysicsThread.inc and
	// docs/handover/dual-thread-route-a.md.
	void UpdatePhysicsFrameDual();
	void PhysicsThreadProc();
	void EnsurePhysicsThread();
	void StopPhysicsThread();
	bool WaitPhysicsFrame();
	void ReleasePhysicsFrame();
	void RunDualStageA();
	void RunDualStageB();
	INT  RunDualThread();
	std::thread m_physicsThread;
	std::mutex  m_dualMutex;
	std::condition_variable m_dualCv;
	bool m_dualThreadQuit = false;      // guarded by m_dualMutex
	bool m_dualFrameReady = false;      // guarded by m_dualMutex
	bool m_dualGateOpen = false;        // guarded by m_dualMutex
	bool m_dualSessionActive = false;   // guarded by m_dualMutex
	std::atomic<bool> m_dualPhysRunning{false};      // bRunning mirror
	std::atomic<bool> m_dualPhysForceUpdate{false};  // g_bForceUpdate mirror
	std::atomic<bool> m_dualPhysPlayback{false};     // bPlayback mirror
	// per-frame performance counters (physics time is written by the physics
	// thread and read by the main thread while the physics thread is parked)
	double m_dualPhysTimeAcc = 0.0;
	double m_dualSyncAWait = 0.0, m_dualSyncBWait = 0.0;
	double m_dualWaitAcc = 0.0;
	double m_dualRenderAcc = 0.0;
	double m_dualPerfStart = 0.0, m_dualPerfLast = 0.0;
	double m_dualPhysLogT = 0.0;
	int    m_dualPerfFrames = 0;



