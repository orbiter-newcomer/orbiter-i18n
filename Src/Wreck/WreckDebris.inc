// ==============================================================
// WreckDebris.inc
// Wreck/debris system: spawn, Newtonian propagation, ground contact.
// Included by Orbiter.cpp (independent wreck module, Src/Wreck; the wreck
// system does not depend on the relativity extension).
// ==============================================================

#include <stdlib.h>
#include "WreckDebris.h"
#include "Localization.h"

struct REL_DEBRIS {
	Vector pos, vel;        // global position/velocity
	Vector axis;            // rotation axis
	double angle, angVel;   // rotation state
	double size;            // characteristic size [m]
	int    shape;           // shape index for the renderer
	double age;             // time since spawn [s]
	double still;           // time spent resting on the surface [s]
	bool   active;
};

static const int    REL_DEBRIS_MAX       = 200;
static const int    REL_DEBRIS_PER_BURST = 12;
static const double REL_DEBRIS_LIFE      = 30.0;   // max lifetime [s]
static const double REL_DEBRIS_STILL_T   = 15.0;    // rest time before removal [s]
static const double REL_DEBRIS_STILL_V   = 0.5;    // rest speed threshold [m/s]
static const double REL_DEBRIS_RESTITUTION = 0.2;  // normal bounce factor
static const double REL_DEBRIS_FRICTION  = 0.5;    // tangential factor

// ---- wreck message (shown on screen by the graphics client) ----------------
static char   g_relWreckMsg[1024] = "";
static bool   g_relWreckMsgIsFocus = false;   // message belongs to the focus vessel
static char   g_relWreckMsgShip[64] = "";     // name of the destroyed vessel
static double g_relWreckTick = -1e18;   // GetTickCount64() at message time
static char g_relWreckVesselName[64] = "";   // focus vessel kept as large wreck

static bool g_relWreckCamActive = false;   // our wreck camera is currently active
static bool g_relWreckGroundCamActive = false;   // ground camera control active

void WreckOnFocusChanged (Vessel *newFocus)
{
	if (!newFocus || !g_camera) return;
	// wreck vessels have no cockpit: always use the external view, otherwise the
	// internal camera would be attached to a vessel without a cockpit and crash.
	if (WreckIsObserver (newFocus)) {
		g_relWreckCamActive = false;
		g_camera->SetTrackMode (CAMERA_GLOBALFRAME, 0);   // global reference frame
		g_camera->Attach (newFocus, 1);
		g_bForceUpdate = true;
		return;
	}
	if (!g_relWreckCamActive) return;
	g_relWreckCamActive = false;
	if (g_relWreckGroundCamActive) {
		g_relWreckGroundCamActive = false;
		g_camera->UnregisterExternalControl ();
	}
	// the player switched vessels: restore a valid external view of the new vessel
	g_camera->SetTrackMode (CAMERA_TARGETRELATIVE, 0);
	g_camera->Attach (newFocus, 1);
	g_bForceUpdate = true;
}



void WreckMessage (int reason, const char *bodyName, const char *otherName, bool otherDestroyed, bool isFocus, const char *shipName)
{
	const char *bn = bodyName ? bodyName : "";
	const char *on = otherName ? otherName : "";
	// the BODY of the message (without the subject); the subject prefix is added below:
	//   focus vessel:            "你的飞船..." / "Your vessel ..."
	//   any other vessel:        "<ship name> ..." / "<ship name> ..."
	// The English templates are the translation keys (see i18n/core/WreckMsgZh.inc);
	// orbiter::Tr() returns the English template unchanged in English mode.
	char body[768];
	switch (reason) {
	case REL_WRECK_COLLISION:
		sprintf_s (body, 768, orbiter::Tr ("was destroyed by a collision with %s."), on);
		if (otherDestroyed) strcat_s (body, 768, orbiter::Tr (" The other vessel was destroyed as well."));
		break;
	case REL_WRECK_SURFACE:
		sprintf_s (body, 768, orbiter::Tr ("crashed into the surface of %s."), bn);
		break;
	case REL_WRECK_STRONGFIELD:
		sprintf_s (body, 768, orbiter::Tr ("was torn apart by the gravity field of %s."), bn);
		break;
	case REL_WRECK_FTL:
		strcpy_s (body, 768, orbiter::Tr ("disintegrated after hitting the light-speed wall."));
		break;
	case REL_WRECK_NUMERIC:
		strcpy_s (body, 768, orbiter::Tr ("was destroyed by the hand of God."));
		break;
	case REL_WRECK_STAR:
		sprintf_s (body, 768, orbiter::Tr ("melted inside %s."), bn);
		break;
	case REL_WRECK_GASGIANT:
		sprintf_s (body, 768, orbiter::Tr ("was crushed by the atmosphere of %s."), bn);
		break;
	case REL_WRECK_BURNUP:
		strcpy_s (body, 768, orbiter::Tr ("became a meteor."));
		break;
	default:
		strcpy_s (body, 768, orbiter::Tr ("was destroyed."));
		break;
	}
	char msg[1024];
	if (isFocus) {
		sprintf_s (msg, 1024, "%s%s", orbiter::Tr ("Your vessel "), body);
	} else {
		const char *sn = (shipName && shipName[0]) ? shipName : orbiter::Tr ("A vessel");
		sprintf_s (msg, 1024, "%s %s", sn, body);
	}
	strcpy_s (g_relWreckMsg, 1024, msg);
	g_relWreckMsgIsFocus = isFocus;
	if (shipName) strncpy_s (g_relWreckMsgShip, 64, shipName, 63); else g_relWreckMsgShip[0] = 0;
	g_relWreckTick = (double)GetTickCount64 ();
}

// Convenience wrapper: decide big (focus) vs corner (other vessel) automatically
void WreckMessageVessel (int reason, Vessel *vessel, const char *bodyName, const char *otherName, bool otherDestroyed)
{
	bool isFocus = (vessel && oapiGetFocusObject () == (OBJHANDLE)vessel);
	WreckMessage (reason, bodyName, otherName, otherDestroyed, isFocus, vessel ? vessel->Name () : 0);
}


OBJHANDLE WreckGetVessel ()
{
	if (!g_relWreckVesselName[0] || !g_psys) return 0;
	return (OBJHANDLE)g_psys->GetVessel (g_relWreckVesselName);
}

bool WreckGetMessage (char *buf, int n, double *age_ms)
{
	if (!buf || n <= 0) return false;
	if (!g_relWreckMsg[0]) return false;
	strncpy_s (buf, n, g_relWreckMsg, n-1);
	buf[n-1] = 0;
	if (age_ms) *age_ms = (double)GetTickCount64 () - g_relWreckTick;
	return true;
}

bool WreckGetMessageEx (char *buf, int n, double *age_ms, int *isFocus, char *ship, int shipN)
{
	if (!WreckGetMessage (buf, n, age_ms)) return false;
	if (isFocus) *isFocus = g_relWreckMsgIsFocus ? 1 : 0;
	if (ship && shipN > 0) {
		strncpy_s (ship, shipN, g_relWreckMsgShip, shipN-1);
		ship[shipN-1] = 0;
	}
	return true;
}



bool WreckIsVessel (Vessel *vessel)
{
	return (vessel && g_relWreckVesselName[0] && strcmp (vessel->Name (), g_relWreckVesselName) == 0);
}

// Third-person camera helpers for the wreck situation.



void WreckDestroyVessel (Vessel *vessel, int reason, const char *otherName)
{
	// DISABLED (wreck/camera system removed for ship-parameter tuning):
	// no vessel deletion, no wreck anchor, no camera switching.
}


bool WreckIsObserver (Vessel *vessel)
{
	if (!vessel) return false;
	const char *n = vessel->Name ();
	if (strncmp (n, "WRECK-", 6) == 0) return true;
	// the invisible wreck anchor is the player vessel kept in the simulation
	return (g_relWreckVesselName[0] && strcmp (n, g_relWreckVesselName) == 0);
}


static REL_DEBRIS g_relDebris[REL_DEBRIS_MAX];
static int g_relDebrisNext = 0;

static double RelDebrisRand ()
{
	return (double)rand() / (double)RAND_MAX;
}

static Vector RelDebrisRandomDir ()
{
	double z = 2.0*RelDebrisRand() - 1.0;
	double phi = 2.0*PI*RelDebrisRand();
	double r = sqrt (fabs (1.0 - z*z));
	return Vector (r*cos(phi), r*sin(phi), z);
}

static REL_DEBRIS *RelDebrisAlloc ()
{
	for (int i = 0; i < REL_DEBRIS_MAX; i++) {
		int idx = (g_relDebrisNext + i) % REL_DEBRIS_MAX;
		if (!g_relDebris[idx].active) {
			g_relDebrisNext = (idx + 1) % REL_DEBRIS_MAX;
			return &g_relDebris[idx];
		}
	}
	REL_DEBRIS *d = &g_relDebris[g_relDebrisNext];
	g_relDebrisNext = (g_relDebrisNext + 1) % REL_DEBRIS_MAX;
	return d;
}

// ---- wreck vessels (real Orbiter vessels with generated meshes) --------------
static int g_wreckVesselCounter = 0;
struct REL_WRECK_VESSEL { char name[64]; double expire; bool reported; VESSELSTATUS2 vs; Vector impactPos; double impactLng, impactLat; };
static REL_WRECK_VESSEL g_wreckVessels[64];
static int g_wreckVesselCount = 0;
static Vector g_pendingWreckPos, g_pendingWreckVel;
static CelestialBody *g_pendingWreckRef = 0;   // impact reference body
static Vector g_pendingWreckOffsets[16];       // per-piece global offsets (hull decomposition)
static int    g_pendingWreckMeshSeq[16];       // slice mesh sequence number (0 = none)
static double g_pendingWreckSize[16];          // slice radius
static int    g_wreckSliceCounter = 0;         // global slice mesh counter
static int g_pendingWreckOffsetCount = 0;
static char g_pendingDeleteVessel[64] = {0};   // vessel to delete once wrecks exist
static char g_pendingFocusWreck[64] = {0};     // wreck vessel to focus on
static double g_pendingWreckLng = 0.0, g_pendingWreckLat = 0.0, g_pendingWreckRad = 0.0;
static Vector g_pendingWreckGroundVel;         // ground-relative velocity at impact
static char g_pendingWreckParent[64] = {0};
static double g_wreckSpeedLimit = 100.0;   // max wreck speed (clamped)
static int g_pendingWreckCount = 0;
static double g_pendingWreckTick = -1e18;
static double g_pendingWreckReleaseT = -1e18;   // delay release until the parent bounces up

// ---- wreck scatter velocity model (separated so it is easy to change) ----------
static const double REL_WRECK_SCATTER_MAX   = 10.0;  // max random scatter speed [m/s]
static const double REL_WRECK_SPLASH_FACTOR = 0.8;   // lateral splash = factor * |normal impact speed|

// Scatter velocity model for one wreck piece: PURE scatter velocity (the inherited
// parent velocity is added separately).
//  - lateral splash: random horizontal direction (ground tangent plane) with speed
//    proportional to the normal impact speed
//  - random scatter: random direction, 0..REL_WRECK_SCATTER_MAX
static Vector RelWreckScatterVelocity (const Vector &nrm, double vnImpact)
{
	Vector v (0, 0, 0);
	// random scatter
	Vector dir ( (double)rand()/RAND_MAX - 0.5,
	             (double)rand()/RAND_MAX - 0.5,
	             (double)rand()/RAND_MAX - 0.5 );
	if (dir.length () < 1e-6) dir = Vector (0, 1, 0);
	v += dir.unit () * (REL_WRECK_SCATTER_MAX * ((double)rand ()/RAND_MAX));
	// lateral splash (ground tangent plane)
	if (vnImpact < 0.0) {
		Vector tanDir ( (double)rand()/RAND_MAX - 0.5,
		                (double)rand()/RAND_MAX - 0.5,
		                (double)rand()/RAND_MAX - 0.5 );
		tanDir -= nrm * dotp (tanDir, nrm);
		if (tanDir.length () > 1e-6) tanDir = tanDir.unit ();
		else tanDir = Vector (1, 0, 0);
		v += tanDir * (fabs (vnImpact) * REL_WRECK_SPLASH_FACTOR);
	}
	return v;
}

// Queue the creation of wreck vessels (processed at the end of the frame).
void WreckSpawnVessels (const Vector &pos, const Vector &vel, int count);   // forward declaration

// Spawn wrecks at explicit global offsets from the impact point (hull decomposition)
void WreckSpawnVesselsEx (const Vector &pos, const Vector &vel, const Vector *offsets, int count)
{
	if (count <= 0) return;
	g_pendingWreckPos = pos;   // body-relative impact position (already converted by caller)
	g_pendingWreckVel = vel;
	g_pendingWreckOffsetCount = min (count, 16);
	for (int k = 0; k < g_pendingWreckOffsetCount; k++) {
		g_pendingWreckOffsets[k] = offsets ? offsets[k] : Vector (0, 0, 0);
	}
	g_pendingWreckCount = min (count, 16);
	g_pendingWreckTick = 0.0;
	g_pendingWreckReleaseT = td.SimT1 + 0.3;
}

// Write one convex-hull slice as a MSHX1 mesh file (Meshes\Wreck\slice_<seq>.msh).
static bool RelWriteSliceMeshFile (const Vessel::HullSliceMesh &sl, int seq)
{
	CreateDirectoryA ("Meshes\\Wreck", NULL);
	char fname[512];
	sprintf_s (fname, 512, "Meshes\\Wreck\\slice_%d.msh", seq);
	FILE *fp = 0;
	if (fopen_s (&fp, fname, "w") != 0 || !fp) return false;
	size_t nv = sl.verts.size ();
	size_t nf = sl.faces.size () / 3;
	fprintf (fp, "MSHX1\n");
	fprintf (fp, "GROUPS 1\n");
	fprintf (fp, "MATERIAL 1\n");
	fprintf (fp, "TEXTURE %d\n", sl.texName[0] ? 1 : 0);   // texture count in the header
	fprintf (fp, "GEOM %d %d\n", (int)nv, (int)nf);
	bool haveN = (sl.norms.size () >= nv);
	bool haveUV = (sl.uvs.size () >= nv * 2);
	for (size_t i = 0; i < nv; i++) {
		double l = sqrt (sl.verts[i].x*sl.verts[i].x + sl.verts[i].y*sl.verts[i].y + sl.verts[i].z*sl.verts[i].z);
		double nx = 0.0, ny = 1.0, nz = 0.0;
		if (haveN) { nx = sl.norms[i].x; ny = sl.norms[i].y; nz = sl.norms[i].z; }
		else if (l > 1e-9) { nx = sl.verts[i].x/l; ny = sl.verts[i].y/l; nz = sl.verts[i].z/l; }
		double tu = haveUV ? sl.uvs[i*2]   : 0.0;
		double tv = haveUV ? sl.uvs[i*2+1] : 0.0;
		fprintf (fp, "%.4f %.4f %.4f %.4f %.4f %.4f %.6f %.6f\n",
		         sl.verts[i].x, sl.verts[i].y, sl.verts[i].z, nx, ny, nz, tu, tv);
	}
	for (size_t i = 0; i < nf; i++) {
		fprintf (fp, "%d %d %d\n", (int)sl.faces[i*3], (int)sl.faces[i*3+1], (int)sl.faces[i*3+2]);
	}
	// material line (diffuse colour) followed by the texture list (MSHX1 layout)
	fprintf (fp, "0.8 0.8 0.8 1\n");
	if (sl.texName[0]) fprintf (fp, "TEXTURES 1\n%s\n", sl.texName);
	else                fprintf (fp, "TEXTURES 0\n");
	fclose (fp);
	return true;
}

// Hull decomposition: one wreck piece per hull group (cabin/wing/truss), each piece
// using its OWN convex-hull mesh extracted from the vessel mesh (real ship slices).
void WreckSpawnHull (Vessel *vessel, const Vector &pos, const Vector &vel, int maxCount)
{
	Vector offs[16];
	int seqs[16];
	double sizes[16];
	int n = 0;
	if (vessel) {
		// convex-hull meshes from the largest SPATIAL clusters of the vessel hull
		std::vector<Vessel::HullSliceMesh> slices;
		int ns = vessel->BuildVoronoiSliceMeshes (slices, maxCount);
		for (int i = 0; i < ns && n < maxCount && n < 16; i++) {
			int seq = ++g_wreckSliceCounter;
			if (!RelWriteSliceMeshFile (slices[i], seq)) continue;
			offs[n] = mul (vessel->GRot (), slices[i].center);   // vessel-local -> global offset
			seqs[n] = seq;
			sizes[n] = slices[i].radius;
			n++;
		}
	}
	if (n <= 0) { WreckSpawnVessels (pos, vel, 8); return; }
	WreckSpawnVesselsEx (pos, vel, offs, n);
	for (int k = 0; k < n; k++) { g_pendingWreckMeshSeq[k] = seqs[k]; g_pendingWreckSize[k] = sizes[k]; }
}

void WreckSpawnVessels (const Vector &pos, const Vector &vel, int count)
{
	if (count <= 0) return;
	g_pendingWreckOffsetCount = 0;   // no hull offsets: random scatter
	// always store the latest impact position/velocity (a conditional update could
	// keep a STALE position from a previous impact, ~9 km away)
	g_pendingWreckPos = pos;
	g_pendingWreckVel = vel;
	g_pendingWreckCount = min (g_pendingWreckCount + count, 16);
	g_pendingWreckTick = 0.0;
	// release the wrecks slightly after the impact, when the ship is already
	// bouncing upwards: the wrecks then inherit an upward velocity and never
	// collide with the ground at spawn time
	g_pendingWreckReleaseT = td.SimT1 + 0.3;
}

// Nearest gravitational body for an arbitrary global position. There is no
// public API for position-based lookups; for vessels use VESSEL::GetGravityRef().
static CelestialBody *RelNearestBody (const Vector &pos)
{
	if (!g_psys) return 0;
	CelestialBody *best = 0;
	double bestD = 1e300;
	for (size_t i = 0; i < g_psys->nGrav(); i++) {
		CelestialBody *c = g_psys->GetGravObj ((int)i);
		if (!c) continue;
		double d = c->GPos().dist (pos);
		if (d < bestD) { bestD = d; best = c; }
	}
	return best;
}

static void WreckProcessPending ()
{
	if (g_pendingWreckTick != 0.0 || g_pendingWreckCount <= 0 || !g_psys) return;
	if (td.SimT1 < g_pendingWreckReleaseT) return;   // wait until the parent bounces up
	g_pendingWreckTick = -1e18;
	// inherit the reference body from the parent vessel (simple and always correct):
	// no nearest-body search, which was picking the Sun for body-relative positions.
	CelestialBody *ref = 0;
	{
		Vessel *pv = g_pendingWreckParent[0] ? g_psys->GetVessel (g_pendingWreckParent) : 0;
		if (pv) ref = pv->ProxyBody();   // official proxy body (nearest massive object)
	}
	if (!ref) ref = g_pendingWreckRef;   // fallback: body recorded at impact time
	if (!ref) ref = RelNearestBody (g_pendingWreckPos);
	if (!ref) { LOGOUT_ERR ("Wreck: no reference body found"); g_pendingWreckCount = 0; return; }
	for (int i = 0; i < g_pendingWreckCount; i++) {
		char name[80];
		int sliceSeq = (i < g_pendingWreckOffsetCount) ? g_pendingWreckMeshSeq[i] : 0;
		if (sliceSeq > 0) sprintf_s (name, 80, "WRECK-S%d", sliceSeq);
		else             sprintf_s (name, 80, "WRECK-V-%d", g_wreckVesselCounter++);
		VESSELSTATUS2 vs;
		memset (&vs, 0, sizeof(vs));
		vs.version = 2;
		Vessel *parent = g_pendingWreckParent[0] ? g_psys->GetVessel (g_pendingWreckParent) : 0;
		if (parent) {
			parent->GetState2 (&vs);
			// official ground-relative velocity (SurfParam is up to date in the safe phase)
			VECTOR3 gvP = {0, 0, 0};
			if (oapiGetGroundspeedVector ((OBJHANDLE)parent, FRAME_GLOBAL, &gvP)) {
				g_pendingWreckGroundVel = Vector (gvP.x, gvP.y, gvP.z);
			}
		}
		if (!vs.rbody) {
			char refname2[128];
			strcpy_s (refname2, 128, ref->Name());
			vs.rbody = oapiGetGbodyByName (refname2);
		}
		if (!vs.rbody) vs.rbody = oapiGetGbodyByIndex (0);
		vs.status = 0;
		vs.base = 0;
		vs.port = (DWORD)-1;
		vs.flag = 0;
		vs.nfuel = 0; vs.fuel = 0;
		vs.nthruster = 0; vs.thruster = 0;
		vs.ndockinfo = 0; vs.dockinfo = 0;
		// force the reference body to the nearest body at the impact point (the
		// parent->GetState2 may return a stale/zero rbody, e.g. after the parent was
		// marked as landed)
		{
			char refname3[128];
			strcpy_s (refname3, 128, ref->Name());
			vs.rbody = oapiGetGbodyByName (refname3);
		}
		CelestialBody *rb = (CelestialBody*)vs.rbody;
		if (!rb) rb = ref;
		double ang = 6.2831853 * ((double)rand () / RAND_MAX);
		double rad = 2.0 + 8.0 * ((double)rand () / RAND_MAX);
		// release exactly at the impact point; if that point is below the ground,
		// clamp it to 1 m above the surface (collisions happen only at the control
		// point, which is the wreck centre)
		// POSITION: from longitude/latitude/altitude via the official conversion
		// (no hand-composed global position). Lift to 1 m above the surface if needed.
		double terrW = oapiSurfaceElevation ((OBJHANDLE)ref, g_pendingWreckLng, g_pendingWreckLat);   // terrain height
		double radAdj = g_pendingWreckRad;
		if (radAdj < ref->Size () + terrW + 1.0) radAdj = ref->Size () + terrW + 1.0;
		// each wreck is scattered around the impact point: small longitude/latitude
		// jitter (1e-5 rad ~ 64 m on Earth)
		double jLng = ((double)rand ()/RAND_MAX - 0.5) * 2.0e-5;
		double jLat = ((double)rand ()/RAND_MAX - 0.5) * 2.0e-5;
		Vector pos;
		ref->EquatorialToGlobal (g_pendingWreckLng + jLng, g_pendingWreckLat + jLat, radAdj, pos);
		// hull decomposition: offset this piece to its hull group position (global)
		if (g_pendingWreckOffsetCount > 0) {
			pos += g_pendingWreckOffsets[i % g_pendingWreckOffsetCount];
		}
		Vector radial = (pos - ref->GPos ()).unit ();
		// the wreck gets the BOUNCE velocity (not the parent's downward impact
		// velocity): normal component reflected with restitution, tangential kept.
		// This prevents the wreck from being pushed into the ground at high speed,
		// which made the contact solver diverge (NaN).
		// VELOCITY: inherit the parent's official relative velocity (VESSELSTATUS2
		// gives the velocity relative to the reference body, including the body spin
		// exactly as the core expects) plus a small random scatter.
		// inherited parent velocity + PURE scatter velocity (model in
		// RelWreckScatterVelocity)
		Vector nrm = (pos - ref->GPos ()).unit ();
		double vnImpact = dotp (Vector (vs.rvel.x, vs.rvel.y, vs.rvel.z), nrm);
		Vector rvel = Vector (vs.rvel.x, vs.rvel.y, vs.rvel.z)
		            + RelWreckScatterVelocity (nrm, vnImpact);
		// initial position: use the global impact position DIRECTLY (the
		// GlobalToEquatorial -> EquatorialToGlobal round trip did not close exactly
		// and introduced a ~9 km offset). After this frame the wreck is left
		// entirely to the collision/physics solver.
		Vector rposOnce = pos - rb->GPos ();
		vs.status = 0;                     // free flight (NOT landed)
		vs.rpos = _V (rposOnce.x, rposOnce.y, rposOnce.z);
		vs.rvel = _V (rvel.x, rvel.y, rvel.z);
		vs.arot = _V (0, 0, 0);
		vs.vrot = _V (0, 0, 0);
		// speed limit for the wreck: never faster than the parent vessel at impact
		g_wreckSpeedLimit = 100.0;
		if (parent) {
			double pv = (parent->GVel() - rb->GVel()).length ();
			if (pv > g_wreckSpeedLimit) g_wreckSpeedLimit = pv;
		}
		OBJHANDLE h = oapiCreateVesselEx (name, "Wreck", &vs);
		if (!h) { LOGOUT_ERR ("Wreck vessel creation failed: %s", name); continue; }
		// feature tag: wreck debris is rendered into the shadow map (the terrain samples
		// it), so the legacy projected ground shadow must be skipped for it.
		oapiSetVesselShadowMode (h, SHADOWMODE_SHADOWMAP);
		// initialise and commit the wreck state vectors once (the vessel is frozen
		// afterwards, so without this GPos() would stay at zero)
		{
			Vessel *wv = g_psys->GetVessel (name);
			if (wv) {
				wv->Update (true);       // compute s1 from the initial state
				wv->EndStateUpdate ();   // commit s1 -> s0
				// sanity: the wreck must start near the impact point; if the initial
				// state was not applied correctly, force the intended position
				double d = (wv->GPos() - pos).length ();
				if (!std::isfinite (d) || d > 1000.0) {
					wv->SetState2 (&vs);
					wv->Update (true);
					wv->EndStateUpdate ();
				}
			}
		}
		{
			double slng = 0.0, slat = 0.0, srad = 0.0;
			rb->GlobalToEquatorial (pos, slng, slat, srad);
		}
		if (g_wreckVesselCount < 64) {
			strcpy_s (g_wreckVessels[g_wreckVesselCount].name, 64, name);
			g_wreckVessels[g_wreckVesselCount].impactPos = pos;
			{
				double ilng = 0.0, ilat = 0.0, irad = 0.0;
				rb->GlobalToEquatorial (pos, ilng, ilat, irad);
				g_wreckVessels[g_wreckVesselCount].impactLng = ilng;
				g_wreckVessels[g_wreckVesselCount].impactLat = ilat;
			}
			g_wreckVessels[g_wreckVesselCount].expire = (double)GetTickCount64 () + 30000.0;
			g_wreckVessels[g_wreckVesselCount].reported = false;
			g_wreckVessels[g_wreckVesselCount].vs = vs;
			g_wreckVesselCount++;
		}
	}
	g_pendingWreckCount = 0;

	// deferred: focus the first new wreck (global object handle), then delete the
	// impacted vessel
	if (g_pendingDeleteVessel[0]) {
		Vessel *focusWreck = 0;
		for (int k = g_wreckVesselCount - 1; k >= 0; k--) {
			Vessel *wv2 = g_psys->GetVessel (g_wreckVessels[k].name);
			if (wv2) { focusWreck = wv2; break; }
		}
		if (focusWreck) {
			oapiSetFocusObject ((OBJHANDLE)focusWreck);   // global object handle
		}
		Vessel *dv = g_psys->GetVessel (g_pendingDeleteVessel);
		if (dv) {
			// TEST ONLY: ship deletion is disabled on request so that one impact shows
			// both the impacted vessel and the wreck debris (shadow verification).
			// TODO: restore: dv->RequestDestruct ();
		}
		g_pendingDeleteVessel[0] = 0;
	}
}


// Called after the state-update phase: creating wreck vessels is only valid
// outside the update loop.
// Terrain normal (global frame) from the official surface-elevation API: the
// returned normal is in the local horizon frame (x=east, y=up, z=north). It is
// converted to the planet-fixed frame and then to the global frame.
static Vector RelTerrainNormal (CelestialBody *cb, double lng, double lat)
{
	VECTOR3 nml;
	nml.x = nml.y = nml.z = 0.0;
	oapiSurfaceElevationEx ((OBJHANDLE)cb, lng, lat, 0, 0, &nml);
	double slng = sin (lng), clng = cos (lng);
	double slat = sin (lat), clat = cos (lat);
	Vector n_pf (
		-slng*nml.x + clat*clng*nml.y - slat*clng*nml.z,
		           + slat*nml.y        + clat*nml.z,
		 clng*nml.x + clat*slng*nml.y - slat*slng*nml.z);
	return mul (cb->GRot (), n_pf);              // to global frame
}


// Official equatorial coordinates of a vessel (longitude/latitude/radius about its
// reference body): oapiGetEquPos.
static bool RelGetEquPos (Vessel *v, double &lng, double &lat, double &rad)
{
	lng = lat = rad = 0.0;
	return (oapiGetEquPos ((OBJHANDLE)v, &lng, &lat, &rad) != FALSE);
}

// Ground-relative velocity via the official Orbiter API (the body spin is already
// subtracted by the core).
static Vector RelGroundVelocity (Vessel *v)
{
	VECTOR3 gv = { 0.0, 0.0, 0.0 };
	if (!oapiGetGroundspeedVector ((OBJHANDLE)v, FRAME_GLOBAL, &gv)) return Vector (0.0, 0.0, 0.0);
	return Vector (gv.x, gv.y, gv.z);
}


static void WreckCleanup ()
{
	if (!g_psys) return;
	// The wreck state is written ONLY once, at creation. After that the wreck is
	// fully controlled by the collision/physics solver: no normal-velocity removal,
	// no static lock, no forced position/state writes here.
	//
	// Only housekeeping is done: remove wrecks whose state went non-finite and
	// diagnostic logging of the current state.
	for (int i = 0; i < g_wreckVesselCount; i++) {
		Vessel *wv = g_psys->GetVessel (g_wreckVessels[i].name);
		if (!wv) continue;
		// once the wreck has settled on the ground (small linear AND angular velocity),
		// the PROGRAM marks it as landed to stop the physics integration (performance).
		// While it is still moving no state write is done at all.
		{
			const SurfParam *wsp0 = wv->GetSurfParam ();
			if (wsp0 && wsp0->alt < wv->Size () + 2.0) {
				VESSELSTATUS2 st0;
				memset (&st0, 0, sizeof(st0));
				st0.version = 2;
				wv->GetState2 (&st0);
				if (st0.status == 0) {   // still free flight
					Vector vg0 = RelGroundVelocity (wv);
					double av2 = st0.vrot.x*st0.vrot.x + st0.vrot.y*st0.vrot.y + st0.vrot.z*st0.vrot.z;
					// RELAXED ANGULAR dead zone for wrecks: complex shapes are less stable than the
					//
					if (vg0.length () < 1.5 && av2 < 4.0) {   // linear 1.5 m/s, angular 2.0 rad/s
						double lng0 = 0.0, lat0 = 0.0, rad0 = 0.0;
						if (RelGetEquPos (wv, lng0, lat0, rad0)) {
							VESSELSTATUS2 lst0;
							memset (&lst0, 0, sizeof(lst0));
							lst0.version = 2;
							lst0.rbody = st0.rbody;
							lst0.status = 1;
							lst0.surf_lng = lng0;
							lst0.surf_lat = lat0;
							lst0.surf_hdg = 0.0;
							lst0.arot.x = 5.0;
							wv->SetState2 (&lst0);
							continue;
						}
					}
				}
			}
		}
	}
	// AUTO-REMOVAL DISABLED (re-enable after the visual test)
	if (false)
	for (int i = g_wreckVesselCount - 1; i >= 0; i--) {
		Vessel *wv = g_psys->GetVessel (g_wreckVessels[i].name);
		if (!wv) { g_wreckVessels[i] = g_wreckVessels[g_wreckVesselCount - 1]; g_wreckVesselCount--; continue; }
		{
			bool remove = false;
			if ((double)GetTickCount64 () > g_wreckVessels[i].expire) remove = true;
			VESSELSTATUS2 stR;
			memset (&stR, 0, sizeof(stR));
			stR.version = 2;
			wv->GetState2 (&stR);
			if (stR.status == 1) {   // settled: keep it a bit, then remove
				static double s_settleT[64] = {0};
				if (s_settleT[i] <= 0.0) s_settleT[i] = td.SimT1;
				if (td.SimT1 - s_settleT[i] > 10.0) remove = true;
			}
			if (remove) {
				LOGOUT_ERR ("Wreck %s removed (cleanup).", g_wreckVessels[i].name);
				wv->RequestDestruct ();
				g_wreckVessels[i] = g_wreckVessels[g_wreckVesselCount - 1];
				g_wreckVesselCount--;
				continue;
			}
		}
		const Vector &wp = wv->GPos ();
		const Vector &wv2 = wv->GVel ();
		if (!std::isfinite (wp.x) || !std::isfinite (wp.y) || !std::isfinite (wp.z) ||
			!std::isfinite (wv2.x) || !std::isfinite (wv2.y) || !std::isfinite (wv2.z)) {
			LOGOUT_ERR ("Wreck %s removed: non-finite state.", g_wreckVessels[i].name);
			wv->RequestDestruct ();
			g_wreckVessels[i] = g_wreckVessels[g_wreckVesselCount - 1];
			g_wreckVesselCount--;
		}
	}
}


void WreckProcessPendingSafe ()
{
	WreckProcessPending ();   // create queued wrecks (safe phase only)
	WreckCleanup ();          // housekeeping only (no state writes)
}

void WreckDebrisSpawn (const Vector &pos, const Vector &vel, double scale)
{
	if (scale <= 0.0) scale = 1.0;
	for (int k = 0; k < REL_DEBRIS_PER_BURST; k++) {
		REL_DEBRIS *d = RelDebrisAlloc();
		d->active = true;
		d->pos = pos;
		d->vel = vel * 0.1 + RelDebrisRandomDir() * (scale * (0.5 + 1.5*RelDebrisRand()));
		d->axis = RelDebrisRandomDir();
		d->angle = 2.0*PI*RelDebrisRand();
		d->angVel = (0.5 + 2.5*RelDebrisRand()) * (RelDebrisRand() < 0.5 ? -1.0 : 1.0);
		d->size = 100.0;   // TEST: giant 100 m debris cubes to verify rendering
		d->shape = (int)(RelDebrisRand()*4.0) % 4;
		d->age = 0.0;
		d->still = 0.0;
	}
}

void WreckDebrisUpdate ()
{
	if (!g_pOrbiter || !g_psys) return;
	double dt = td.SimDT;
	if (dt <= 0.0 || dt > 0.1) return;   // only during real-time stepping

	for (int k = 0; k < REL_DEBRIS_MAX; k++) {
		REL_DEBRIS &d = g_relDebris[k];
		if (!d.active) continue;

		// dominant body = nearest gravitational body
		CelestialBody *cb = RelNearestBody (d.pos);
		double best = cb ? cb->GPos().dist (d.pos) : 0.0;

		if (cb) {
			double R = cb->Size();
			Vector n = (d.pos - cb->GPos()).unit();
			double r = best;
			double gm = Ggrav * cb->Mass();
			if (r > 1.0) d.vel -= n * (gm/(r*r));
			d.pos += d.vel * dt;
			r = d.pos.dist (cb->GPos());
			if (r <= R + 1.0) {
				// ground contact: bounce with damping, heavy friction
				Vector n2 = (d.pos - cb->GPos()).unit();
				double vn = dotp (d.vel, n2);
				Vector vt = d.vel - n2*vn;
				if (vn < 0.0)
					d.vel = vt * REL_DEBRIS_FRICTION - n2 * (vn * REL_DEBRIS_RESTITUTION);
				d.pos = cb->GPos() + n2 * (R + 1.0);
				if (d.vel.length() < REL_DEBRIS_STILL_V) d.still += dt;
				else d.still = 0.0;
			} else {
				d.still = 0.0;
			}
		}

		d.angle += d.angVel * dt;
		d.age += dt;
		if (d.age > REL_DEBRIS_LIFE || d.still > REL_DEBRIS_STILL_T) d.active = false;
	}

	// keep the wreck frozen (zero angular velocity) so the camera is stable
	if (g_relWreckVesselName[0] && g_psys) {
		Vessel *wreck = g_psys->GetVessel (g_relWreckVesselName);
		if (!wreck) {
			g_relWreckVesselName[0] = 0;
		} else {
			wreck->SetAngVel (Vector (0,0,0));
		}
	}
	// Once the wreck camera is no longer active, make sure the camera is not
	// still attached to the invisible wreck anchor (e.g. after switching
	// vessels and pressing F1 for the external view of the new vessel).
	if (g_relWreckVesselName[0] && !g_relWreckCamActive && g_focusobj && g_camera) {
		Vessel *anchor = g_psys ? g_psys->GetVessel (g_relWreckVesselName) : 0;
		if (anchor && g_camera->Target() == (Body*)anchor && g_focusobj != anchor) {
			g_camera->Attach (g_focusobj, 2);
			g_bForceUpdate = true;
		}
	}

}

void WreckCollisionImpact (Vessel *vessel, const Vector &deltaV, const char *otherName, bool otherDestroyed)
{
	if (!vessel || !g_pOrbiter || !g_pOrbiter->Cfg()->CfgPhysicsPrm.bWrecks) return;
	if (WreckIsObserver (vessel)) return;
	if (vessel->KillPending()) return;
	double dv = deltaV.length();
	if (dv <= g_pOrbiter->Cfg()->CfgPhysicsPrm.RelCollisionDVThreshold) return;
		WreckDebrisSpawn (vessel->GPos(), vessel->GVel(), vessel->Size());
	WreckDestroyVessel (vessel, REL_WRECK_COLLISION, otherName);
	WreckMessageVessel (REL_WRECK_COLLISION, vessel, 0, otherName, otherDestroyed);
}

void WreckSurfaceImpact (Vessel *vessel, const StateVectors &state)
{
	if (!vessel || !g_psys) return;
	if (WreckIsObserver (vessel)) return;
	if (vessel->KillPending()) return;
	static double s_lastImpactT = -1e18;
	if (td.SimT1 - s_lastImpactT < 1.0) return;   // one burst per second max
	// nearest celestial body: the vessel's proxy body (official data, nearest massive object)
	CelestialBody *cb = vessel->ProxyBody();
	if (!cb) return;
	double best = cb->GPos().dist (state.pos);
	double alt = best - cb->Size ();
	if (alt > 50.0) return;                       // not touching the ground
	// downward (relative) velocity check: only a real crash spawns wrecks
	Vector vrel = state.vel - cb->GVel ();
	Vector n = (state.pos - cb->GPos()).unit ();
	double vn = dotp (vrel, n);                    // negative = into the ground
	if (vn > -10.0) return;                        // gentle contact, no wreck (normal speed > 10 m/s required)
	s_lastImpactT = td.SimT1;
	// queue the wreck vessels (no ship deletion, no camera switching)
	strcpy_s (g_pendingWreckParent, 64, vessel->Name());
	g_pendingWreckRef = cb;   // remember the impact body (the position below is body-relative)
	// all positions inherit longitude/latitude/altitude; all velocities inherit the
	// GROUND-relative velocity (the body spin must not be added on top)
	{
		double ilng = 0.0, ilat = 0.0, irad = 0.0;
		if (!RelGetEquPos (vessel, ilng, ilat, irad)) cb->GlobalToEquatorial (state.pos, ilng, ilat, irad);
		g_pendingWreckLng = ilng;
		g_pendingWreckLat = ilat;
		g_pendingWreckRad = irad;
		// ground velocity is queried later in the SAFE phase (see
		// WreckProcessPending) via the official oapiGetGroundspeedVector:
		// inside the physics loop SurfParam is stale.
		g_pendingWreckGroundVel = Vector (0, 0, 0);   // placeholder
	}
	WreckSpawnHull (vessel, state.pos - cb->GPos (), state.vel - cb->GVel (), 8);
	// screen message: big centred text if this is the focus vessel, small corner text
	// (with the ship name) for any other vessel
	{
		OBJHANDLE hF = oapiGetFocusObject ();
		bool isFocus = (hF == (OBJHANDLE)vessel);
		WreckMessage (REL_WRECK_SURFACE, cb->Name (), 0, false, isFocus, vessel->Name ());
	}
	// deferred: once the wreck vessels exist, move the focus to the first wreck and
	// then delete the impacted vessel (recorded here, executed in the safe phase)
	g_pendingDeleteVessel[0] = 0;
	strcpy_s (g_pendingDeleteVessel, 64, vessel->Name ());   // position RELATIVE to the body (it moves during the release delay)
	// mark the impacted vessel itself (kept as the wreck anchor) as LANDED
	{
		double alng = 0.0, alat = 0.0, arad = 0.0;
		if (!RelGetEquPos (vessel, alng, alat, arad)) {
			cb->GlobalToEquatorial (state.pos, alng, alat, arad);   // fallback
		}
		VESSELSTATUS2 als;
		memset (&als, 0, sizeof(als));
		als.version = 2;
		als.rbody = (OBJHANDLE)cb;
		als.status = 1;              // landed
		als.surf_lng = alng;
		als.surf_lat = alat;
		als.surf_hdg = 0.0;
		als.arot.x = 5.0;            // simple landed form
		vessel->SetState2 (&als);
	}
	{
		double ilng = 0.0, ilat = 0.0, irad = 0.0;
		cb->GlobalToEquatorial (state.pos, ilng, ilat, irad);
		// actual position of the focus vessel (the player ship, typically GL-01)
		double flng = 0.0, flat = 0.0, frad = 0.0;
		const char *fname = "-";
		OBJHANDLE hF = oapiGetFocusObject ();
		if (hF) {
			for (DWORD iv = 0; iv < g_psys->nVessel(); iv++) {
				Vessel *fv = g_psys->GetVessel (iv);
				if (fv && (OBJHANDLE)fv == hF) {
					fname = fv->Name ();
					cb->GlobalToEquatorial (fv->GPos(), flng, flat, frad);
					break;
				}
			}
		}
	}
}

// Atmospheric dynamic-pressure check: a vessel hitting the atmosphere at a
// high dynamic pressure (re-entry) burns up like a meteor.
void WreckAtmosphericBurnup (Vessel *vessel)
{
	if (!vessel || !g_pOrbiter || !g_pOrbiter->Cfg()->CfgPhysicsPrm.bWrecks) return;
	if (WreckIsObserver (vessel)) return;
	if (vessel->KillPending()) return;
	// GROUND-relative velocity (the atmosphere co-rotates with the body).
	//  - surface/atmosphere damage uses the GROUND-relative velocity (the atmosphere
	//    co-rotates with the body): official groundspeed vector.
	//  - orbital damage (FTL wall, strong field, collisions) uses the inertial/global
	//    frame elsewhere.
	VESSELSTATUS vs;
	vessel->GetState (vs);
	if (vs.status != 0) return;                       // landed: skip
	// ground-relative velocity via the official API (falls back to the body-relative
	// velocity if unavailable)
	VECTOR3 gvA = { 0.0, 0.0, 0.0 };
	double v2;
	if (oapiGetGroundspeedVector ((OBJHANDLE)vessel, FRAME_GLOBAL, &gvA)) {
		v2 = gvA.x*gvA.x + gvA.y*gvA.y + gvA.z*gvA.z;
	} else {
		v2 = vs.rvel.x*vs.rvel.x + vs.rvel.y*vs.rvel.y + vs.rvel.z*vs.rvel.z;
	}
	double speed = sqrt (v2);
	if (speed < 1000.0) return;                       // launch/landing regime
	double pressure = 0.0, density = 0.0;
	if (!vessel->AtmPressureAndDensity (pressure, density)) return;
	if (density <= 0.0) return;
	double q = 0.5 * density * v2;                    // dynamic pressure [Pa]
	const double REL_BURNUP_Q = 1.0e5;                // threshold (10 kPa)
	if (q <= REL_BURNUP_Q) return;
		WreckDebrisSpawn (vessel->GPos(), vessel->GVel(), vessel->Size());
	WreckDestroyVessel (vessel, REL_WRECK_BURNUP, 0);
	WreckMessageVessel (REL_WRECK_BURNUP, vessel, 0, 0, false);
}


int WreckDebrisCount ()
{
	int n = 0;
	for (int i = 0; i < REL_DEBRIS_MAX; i++)
		if (g_relDebris[i].active) n++;
	return n;
}

bool WreckDebrisGet (int idx, VECTOR3 &pos, VECTOR3 &axis, double &angle, double &size, int &shape)
{
	if (idx < 0 || idx >= REL_DEBRIS_MAX) return false;
	REL_DEBRIS &d = g_relDebris[idx];
	if (!d.active) return false;
	pos = _V (d.pos.x, d.pos.y, d.pos.z);
	axis = _V (d.axis.x, d.axis.y, d.axis.z);
	angle = d.angle;
	size = d.size;
	shape = d.shape;
	return true;
}
