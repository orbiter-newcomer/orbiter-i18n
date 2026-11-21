// ==============================================================
// VesselMembers_Vessel.inc - project extension member declarations for class Vessel.
// Included inside the class body in the official header: the official
// header only grows by the include line.
// ==============================================================

public:

	// Internal-damage interface: the player can no longer control this vessel
	// (camera/view switching is not affected). See WreckDisableControl().
	inline void SetControlDisabled (bool b)
	{ bControlDisabled = b; }
	inline bool IsControlDisabled() const
	{ return bControlDisabled; }

public:
	// Multi-core vessel update (implementation in Vessel.cpp, driven by
	// Src/Orbiter/ParallelUpdate.inc): the protection guards, the main-thread
	// part of the state update and the module animation callbacks run on the
	// main thread; UpdatePropagate() (state integration) of independent vessels
	// runs on worker threads.
	void UpdateCore (bool force);
	void UpdatePropagate (bool force);
	void UpdateFinish ();
	bool RunUpdateGuards ();
	void UpdateAnimate ();
	bool IsParallelUpdateCandidate () const;
	bool IsParallelForcesCandidate () const;


public:

	inline void SetShadowMode (int mode) { shadowMode = mode; }
	inline int  GetShadowMode () const { return shadowMode; }
	// shadow rendering mode (SHADOWMODE_*), used by graphics clients

private:
	int shadowMode;         // shadow rendering mode (SHADOWMODE_*)


private:

protected:
	bool bControlDisabled;   // player control disabled (internal damage, see WreckDamageVessel)

