// VesselUpdate.inc - vessel update split for the multi-core update (project extension).
// Included by the official Vessel.cpp at the end of the file.

bool Vessel::RunUpdateGuards ()
{
	{
		// diagnostic: relativity protection guards (PERFDIAG)
		PerfDiag::Scope _perfGuard(PerfDiag::g.updGuard);
		// Relativity extension: the player vessel kept as a central wreck piece
		// is frozen in place so the camera stays on the debris cloud.
		// Relativistic numeric protection (NaN/inf fallback; implementation in RelativityPhysics.inc)
		if (RelativityNumericalGuard (this)) return false;

		// Relativistic strong-field protection (implementation in RelativityPhysics.inc)
		if (RelativityStrongFieldGuard (this)) return false;

		// Relativistic FTL velocity protection (implementation in RelativityPhysics.inc)
		if (RelativityFTLGuard (this, *s0, s1)) return false;
	}

	// Relativity extension: atmospheric dynamic-pressure burn-up check
	{
		PerfDiag::Scope _perfBurn(PerfDiag::g.updBurn);
		WreckAtmosphericBurnup (this);
	}
	return true;
}

void Vessel::UpdateCore (bool force)
{
	// if the vessel is part of a composite structure or passively attached
	// to a parent vessel, its state is updated by the composite or parent
	if (attach)
		return;

	UpdatePropagate (force);
	UpdateFinish ();
}

void Vessel::UpdatePropagate (bool force)
{
	// State propagation part of the vessel update. This part only depends on
	// the vessel's own state and on read-only celestial body data (no module
	// callbacks, no terrain/atmosphere queries for the vessels that are updated
	// on worker threads), so it can run concurrently. See ParallelUpdate.inc.

	int i;

	double perfStateT = PerfDiag::Now();
	if (fstatus == FLIGHTSTATUS_FREEFLIGHT) {

		if (!supervessel) {
			if (bFRplayback) {
				FRecorder_Play();          // update from playback stream
			} else {
				bool savedCanStabilise = bCanUpdateStabilised;
				if (collisionCooldownT > td.SimT0)
					bCanUpdateStabilised = false;
				RigidBody::Update (force); // standard dynamic update
				bCanUpdateStabilised = savedCanStabilise;
			}
		}

	} else if (fstatus == FLIGHTSTATUS_LANDED) {

		// simplified state update for idle vessels
		proxyplanet->LocalToGlobal_t1 (sp.ploc, s1->pos);
		double vground = Pi2 * proxyplanet->Size() * sp.clat / proxyplanet->RotT();
		s1->vel.Set (-vground*sp.slng, 0.0, vground*sp.clng);
		s1->vel.Set (mul (proxyplanet->s1->R, s1->vel) + proxyplanet->s1->vel);
		s1->R.Set (land_rot);
		s1->R.premul (proxyplanet->s1->R);
		s1->Q.Set (s1->R);
		acc = proxyplanet->Acceleration() + (proxyplanet->s1->pos-s1->pos) * (vground * Pi2/(proxyplanet->Size()*proxyplanet->RotT()));
		el_valid = false;

		if (bForceActive) {
			// switch to freeflight whenever the user engages engines
			fstatus = FLIGHTSTATUS_FREEFLIGHT;
			LandingTest.testing = false;

			// this needs thinking
			if (proxybase) proxybase->ReportTakeoff (this);
			if (lstatus == 3 || lstatus == 5) lstatus = 1;

			// need to synchronise state vectors - DO WE NEED THIS?
			rpos_base = s1->pos; rpos_add.Set (0,0,0);
			s1->vel += rvel_add; // MS-071005: rvel_add included to fix "stuck at latitude 90" bug
			rvel_base = s1->vel; rvel_add.Set (0,0,0);
			s1->omega.Set (0,0,0);
		}

		//if (bFRplayback)
		//	FRecorder_Play();

	} else { // should not get here
	}
	PerfDiag::Add(PerfDiag::g.updState, perfStateT);
}

void Vessel::UpdateFinish ()
{
	// Remainder of the vessel update, executed on the main thread after the
	// propagation phase (surface/atmosphere queries, base collisions, touchdown
	// handling, comms and module-independent bookkeeping).

	int i;

	// update surface parameters
	double perfSurfT = PerfDiag::Now();
	if (proxybody && fstatus != FLIGHTSTATUS_LANDED)
		UpdateSurfParams();
	PerfDiag::Add(PerfDiag::g.updSurfPrm, perfSurfT);

	nforcevec_col = 0;
	if (td.SimT1 >= m_colDebug.showTime)
		m_colDebug.active = false;

	double perfBaseT = PerfDiag::Now();
	if (proxyplanet && fstatus != FLIGHTSTATUS_LANDED && !bFRplayback) {

		// base-to-vessel mesh collision
		if (g_pOrbiter->Cfg()->CfgPhysicsPrm.bBaseCollision && sp.alt < 5.0 * size) {
			CheckBaseCollisions((Planet*)proxyplanet);
		}
	}
	PerfDiag::Add(PerfDiag::g.updBaseCol, perfBaseT);

	double perfRestT = PerfDiag::Now();

	if (proxyplanet)
		m_hullCachePValid = false; // reset cache for this frame

	if (proxyplanet && fstatus != FLIGHTSTATUS_LANDED && !bFRplayback) {

		// handle planetary surface touchdown events
		if (sp.alt < 2.0*size) {

			// check for touchdown
			Matrix T (sp.L2H); // T: vessel->horizon rotation matrix
			T.tpostmul (proxyplanet->s1->R);
			T.postmul (s1->R);

			double py, pymin;
			int imin;
			for (i = 0; i < 3; i++) {
				Vector p (mul (T, touchdown_vtx[i].pos));
				py = p.y + sp.alt;                 // vertical position
				if (!i || py < pymin) pymin = py, imin = i;
			}

			if (pymin < 0.0 && !bFRplayback && !bDynamicGroundContact) { // we have (at least partial) touchdown

				// Step 1: rotate vessel towards sitting flat on the ground

				// Calculate compressed touchdown points at equilibrium
				double mg = GetWeight();
				Vector tp_comp[3];
				for (i = 0; i < 3; i++)
					tp_comp[i] = touchdown_vtx[i].pos + touchdown_nm*(touchdown_vtx[i].compression*mg);

				// equilibrium touchdown plane normal
				Vector tnm_comp = crossp (tp_comp[0]-(tp_comp[1]+tp_comp[2])*0.5, tp_comp[2]-tp_comp[1]).unit();

				// horizon normal in vessel frame
				Vector horizon_nm (T.m21, T.m22, T.m23);
				double tilt = acos (dotp (tnm_comp, horizon_nm));

				if (tilt > 1e-10) {
					// rotate rotation axis into vessel x-axis
					Vector raxis (crossp (tnm_comp, horizon_nm).unit());
					double theta = asin (raxis.y);
					double phi   = atan2 (raxis.z, raxis.x);
					double sint  = sin (theta), cost = cos (theta);
					double sinp  = sin (phi), cosp = cos (phi);
					Matrix R (cost*cosp, sint, cost*sinp,  -sint*cosp, cost, -sint*sinp,  -sinp, 0, cosp);
					// rotate around x-axis to compensate tilt
					double arm = touchdown_vtx[imin].pos.dist(touchdown_cg)/size;
					// rotate around x-axis to compensate tilt
					double dtilt = min (tilt, -pymin * arm * 0.03 /*0.005e-1*/);
					double sina = sin(dtilt), cosa = cos(dtilt);
					R.premul (Matrix (1,0,0,  0,cosa,-sina,  0,sina,cosa));
					// rotate rotation axis back
					R.premul (Matrix (cost*cosp, -sint*cosp, -sinp,  sint, cost, 0,  cost*sinp, -sint*sinp, cosp));

					// modify angular velocity
					Vector dv = raxis * (dtilt*td.iSimDT);
					s1->omega -= dv;

					// limit angular velocity when we are flat on the ground
					if (tilt < 5.0*RAD) {
						double maxv = tilt*tilt*50 + 0.1*RAD;
						for (i = 0; i < 3; i++)
							s1->omega.data[i] = min (maxv, max (-maxv, s1->omega.data[i]));
					}

					// update global rotation matrix
					//s1->R.postmul (R);
					//s1->Q.Set (s1->R);

					//T.postmul (R);
				}

				// Update touchdown points for compression
				for (i = 0; i < 3; i++) {
					Vector p (mul (T, tp_comp[i]));
					py = p.y + sp.alt;
					if (!i || py < pymin) pymin = py, imin = i;
				}

				// Step 2: lift vessel altitude so that lowest touchdown point touches the ground
				if (pymin < 0.0) {
					sp.rad -= pymin;
					sp.alt -= pymin;
					Vector loc;
					proxyplanet->EquatorialToLocal (sp.slng, sp.clng, sp.slat, sp.clat, sp.rad, loc);
					Vector pgpos(s1->pos);
					SetRPos (mul (proxyplanet->s1->R, loc) + proxyplanet->s1->pos);
					Vector dv ((s1->pos-pgpos)*td.iSimDT);
					double ay = mul (sp.L2H, tmul (proxyplanet->s1->R, acc)).y;
					if (ay < 0) dv -= mul (proxyplanet->s1->R, tmul(sp.L2H, Vector (0, ay, 0)))*0.5*td.SimDT;
					AddRVel (dv);
				}

				// update surface parameters
				UpdateSurfParams();
			}
		}
	}

	if (td.SimT1 > commsT) {

		// update NAV receivers
		if (nnav) UpdateReceiverStatus ();

		// update landing target comms setup
		if (landtgt && proxyplanet) {
			double blng, blat, adist, bdir, dist;
			landtgt->EquPos (blng, blat);
			Orthodome (sp.lng, sp.lat, blng, blat, adist, bdir);
			dist = adist * landtgt->RefPlanet()->Size();
			if (dist > 1e5) lstatus = 0;
			else if (lstatus == 0) {
				lstatus = 1;
			} else if (lstatus == 2) {
				lstatus = landtgt->RequestLanding (this, nport);
			} else if (lstatus == 3) {
				lstatus = landtgt->RequestTakeoff ();
			}
		} else {
			lstatus = 0;
		}
#ifdef UNDEF
		// update docking target comms setup
		if (docktgt) {
			double dist2;
			dist2 = gpos->dist2 (docktgt->GPos());
			if (dist2 > 1e10) dstatus = 0;
			else if (dstatus == 0) {
				dstatus = 1;
			} else if (dstatus == 2) {
				dstatus = docktgt->RequestDocking (this, nport);
			} else if (dstatus == 3) {
				dstatus = docktgt->RequestUndock ();
			}
		} else {
			dstatus = 0;
		}
#endif
		commsT = td.SimT1 + 2.0 + rand1()*4;
	}

	// state vectors w.r.t. reference body
	cpos = s1->pos - cbody->s1->pos;
 	cvel = s1->vel - cbody->s1->vel;

	// module interface calls (animation callbacks are executed by
	// UpdateAnimate() on the main thread, see ParallelUpdate.inc)

	Flin.Set (Flin_add);    // store current linear force
	Amom.Set (Amom_add);    // store current torque
	Flin_add.Set (0,0,0);   // reset linear force
	Amom_add.Set (0,0,0);   // reset angular moments
	E0_comp = E_comp;       // store compression energy
	//for (i = 0; i < 2; i++) wbrake_override[i] = 0;
	weight_valid = torque_valid = false;
	bForceActive = false;
	UpdateMass(); // register fuel consumption

	// Now update all passively attached child vessels
	UpdateAttachments();

	PerfDiag::Add(PerfDiag::g.updRest, perfRestT);
}

void Vessel::UpdateAnimate ()
{
	// module interface calls (visual animation of the vessel: plugin code runs
	// on the main thread only, see Src/Orbiter/ParallelUpdate.inc)
	if (hVis && animcount) {
		if (modIntf.v->Version() >= 1)
			((VESSEL2*)modIntf.v)->clbkAnimate (td.SimT1);
	}
}

bool Vessel::IsParallelUpdateCandidate () const
{
	// Multi-core vessel update classification (Src/Orbiter/ParallelUpdate.inc).
	//
	// Only vessels that cannot interact with anything else during the
	// propagation phase are eligible: no attachments or composite structures,
	// free flight, no playback, above the terrain elevation query range (so no
	// elevation/contact queries are executed during the integration) and no
	// complex (Pines) gravity solver, whose internal state is mutable. The
	// distance test against all other vessels is done by the caller.
	//
	// The state update itself only requires read-only celestial body data; the
	// atmosphere/surface parameter queries, touchdown handling and comms setup
	// are deferred to the main thread (Vessel::UpdateFinish).
	if (!proxybody) return false;
	if (attach || supervessel || ncattach) return false;
	if (fstatus != FLIGHTSTATUS_FREEFLIGHT) return false;
	if (bFRplayback) return false;
	if (sp.alt < 1.1e5) return false;	// terrain elevation queries below ~100 km
	if (g_pOrbiter && g_pOrbiter->Cfg()->CfgPhysicsPrm.bNonsphericalGrav)
		return false;					// Pines gravity solver is not re-entrant
	return true;
}

bool Vessel::IsParallelForcesCandidate () const
{
	// The body-force update calls the vessel module navigation callback
	// (clbkNavProcess) for vessels with an active nav mode, and the atmosphere
	// parameter query may call into the atmosphere model module; both must stay
	// on the main thread.
	if (!IsParallelUpdateCandidate()) return false;
	if (navmode != 0) return false;
	if (sp.is_in_atm) return false;
	return true;
}

