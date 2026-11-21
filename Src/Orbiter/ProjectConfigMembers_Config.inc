// ==============================================================
// ProjectConfigMembers_Config.inc - project extension member declarations for class Config.
// Included inside the class body in the official header: the official
// header only grows by the include line.
// ==============================================================

private:
	// Wreck/debris system (implementation in WreckConfig.inc, Src/Wreck)
	void WreckLoad (std::istream &is);
	void WreckWritePhysics (std::ostream &os, bool bEchoAll) const;
	const void *GetWreckParam (DWORD paramtype) const;

	// Contact model (implementation in ContactConfig.inc)
	void ContactLoad (std::istream &is);
	void ContactWritePhysics (std::ostream &os, bool bEchoAll) const;
	const void *GetContactParam (DWORD paramtype) const;

	// Multi-core vessel update (implementation in ParallelUpdateConfig.inc)
	void ParallelUpdateLoad (std::istream &is);
	void ParallelUpdateWritePhysics (std::ostream &os, bool bEchoAll) const;


