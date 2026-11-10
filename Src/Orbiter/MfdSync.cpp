// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#include "MfdSync.h"
#include "Localization.h"
#include "Pane.h"
#include "Celbody.h"
#include "Psys.h"
#include "Select.h"
#include <stdio.h>

using namespace std;

extern PlanetarySystem *g_psys;
extern InputBox *g_input;

// =======================================================================
// class Instrument_OSync

static const char *modestr[7] = {
	"Intersect 1",
	"Intersect 2",
	"Sh periapsis",
	"Sh apoapsis",
	"Tg periapsis",
	"Tg apoapsis",
	"Manual axis"
};

struct Instrument_OSync::SavePrm Instrument_OSync::saveprm = {0, 0, Instrument_OSync::MODE_INTERSECT1, 0.0, 5};

Instrument_OSync::Instrument_OSync (Pane *_pane, INT_PTR _id, const Spec &spec, Vessel *_vessel, bool restore)
: Instrument (_pane, _id, spec, _vessel)
{
	mode = MODE_INTERSECT1;
	norbit = 5;
	SetSize (spec);
	tgt = 0;
	man_rlng = 0.0;
	if (restore && vessel == saveprm.usr) {
		SetTarget (saveprm.tgt);
		mode     = saveprm.mode;
		man_rlng = saveprm.man_rlng;
		norbit   = saveprm.norbit;
	}
	man_sinr = sin(man_rlng), man_cosr = cos(man_rlng);
}

Instrument_OSync::~Instrument_OSync ()
{
	// save status
	saveprm.usr      = vessel;
	saveprm.tgt      = tgt;
	saveprm.mode     = mode;
	saveprm.man_rlng = man_rlng;
	saveprm.norbit   = norbit;
}

HELPCONTEXT *Instrument_OSync::HelpTopic () const
{
	extern HELPCONTEXT DefHelpContext;
	DefHelpContext.topic = (char*)"/mfd_sync.htm";
	return &DefHelpContext;
}

void Instrument_OSync::UpdateDraw (oapi::Sketchpad *skp)
{
	char cbuf[128];
	int i, j, x = cw/2, x2 = IW/2, y = 1+(ch*3)/2, x0, x1;
	double scale; // scaling factor for orbit display
	oapi::IVECTOR2 pt[ELN+5];

	sprintf (cbuf, "%s%s", orbiter::Tr("Sync Orbit: "), tgt ? orbiter::Tr(tgt->Name()) : orbiter::Tr("no target"));
	DisplayTitle (skp, cbuf);
	skp->SetTextColor (draw[0][0].col);

	const Body *ref = vessel->ElRef();
	const Elements *myel = vessel->Els();
	if (!ref || !myel) {
		skp->Text (x, y, orbiter::Tr("Cannot determine orbit"), (int)strlen(orbiter::Tr("Cannot determine orbit")));
		return;
	}
	if (!tgt) {
		skp->Text (x, y, orbiter::Tr("Select target (Shift-T)"), (int)strlen(orbiter::Tr("Select target (Shift-T)")));
		return;
	}
	if (tgt->ElRef() != ref) {
		skp->Text (x, y, orbiter::Tr("Invalid target"), (int)strlen(orbiter::Tr("Invalid target")));
		return;
	}
	const Elements *tgtel = tgt->Els();
	if (!tgtel) {
		skp->Text (x, y, orbiter::Tr("Cannot determine target orbit"), (int)strlen(orbiter::Tr("Cannot determine target orbit")));
		return;
	}
	// at this point we know that vessel and target, and their elements,
	// are defined and that they have the same reference body

	// draw orbit ellipses and radius vectors
	scale = pixrad / (myel->ApDist() > tgtel->ApDist() ? myel->ApDist() : tgtel->ApDist());
	Matrix irot (IRotMatrix (myel->cost, myel->sint, myel->cosi, myel->sini));
	Matrix rot1 (RotMatrix (myel->coso, myel->sino, myel->cost, myel->sint, myel->cosi, myel->sini));
	rot1.premul (irot);
	UpdateEllipse (ICNTX, ICNTY, scale, myel, rot1, irot, pt);
	skp->SetPen (draw[0][1].solidpen);
	skp->Polygon (pt, ELN);
	skp->Line (ICNTX, ICNTY, pt[ELN].x, pt[ELN].y);
	Matrix rot2 (RotMatrix (tgtel->coso, tgtel->sino, tgtel->cost, tgtel->sint, tgtel->cosi, tgtel->sini));
	rot2.premul (irot);
	UpdateEllipse (ICNTX, ICNTY, scale, tgtel, rot2, irot, pt);
	skp->SetPen (draw[1][1].solidpen);
	skp->Polygon (pt, ELN);
	skp->Line (ICNTX, ICNTY, pt[ELN].x, pt[ELN].y);

	double p1, p2, A, B, C, d, p, q, arg;
	double ea, rma, cma, dt0, dt1, T0, T1;
	double rlng, sinr, cosr; // direction of reference axis
	double domega = tgtel->omegab - myel->omegab; // angle between periapses

	// reference axis mode
	strcpy (cbuf, orbiter::Tr("Ref: ")); strcat (cbuf, orbiter::Tr(modestr[mode]));
	skp->Text (x, y, cbuf, strlen(cbuf)); y += ch;

	switch (mode) {
	case MODE_INTERSECT1:
	case MODE_INTERSECT2:
		// find the intersection of the ellipses
		p1 = myel->a * (1.0-myel->e*myel->e);
		p2 = tgtel->a * (1.0-tgtel->e*tgtel->e);
		A = myel->e/p1 - tgtel->e/p2 * cos(domega);
		B = -tgtel->e/p2 * sin(domega);
		C = 1.0/p1 - 1.0/p2;
		d = A*A + B*B;
		p = 2.0*B*C/d;
		q = (C*C-A*A)/d;
		arg = p*p*0.25-q;
		if (arg >= 0) { // intersections exist
			double s = sqrt (arg);
			sinr = (mode == MODE_INTERSECT1 ? -0.5*p+s : -0.5*p-s);
			rlng = asin(sinr);
			cosr = cos(rlng);
			// resolve ambiguity in sine
			if (fabs (A*cosr + B*sinr + C) > fabs (-A*cosr + B*sinr + C)) rlng = Pi-rlng, cosr = -cosr;
		} else {
			skp->Text (x, y, orbiter::Tr("No intersection"), (int)strlen(orbiter::Tr("No intersection"))); y += ch;
			return;
		}
		break;
	case MODE_SHIP_PA:
		sinr = 0.0, cosr = 1.0, rlng = 0.0;
		break;
	case MODE_SHIP_AA:
		sinr = 0.0, cosr = -1.0, rlng = Pi;
		break;
	case MODE_TGT_PA:
		rlng = posangle (domega), sinr = sin(rlng), cosr = cos(rlng);
		break;
	case MODE_TGT_AA:
		rlng = posangle (domega+Pi), sinr = sin(rlng), cosr = cos(rlng);
		break;
	case MODE_MANUAL:
		rlng = man_rlng, sinr = man_sinr, cosr = man_cosr;
		break;
	}
	Vector v(mul (rot1, Vector(cosr, 0.0, sinr)));
	skp->SetPen (draw[0][0].solidpen);
	skp->Line (ICNTX, ICNTY, ICNTX+(int)(v.x*pixrad), ICNTY-(int)(v.z*pixrad));

	// relative anomaly
	sprintf (cbuf, "%s%7.2fº", orbiter::Tr("RAnm"), Deg(rlng));
	skp->Text (x, y, cbuf, strlen(cbuf)); y += (3*ch)/2;

	// ship in planet coords
	Vector sp = vessel->GPos()-ref->GPos();
	Vector sv = vessel->GVel()-ref->GVel();
	// normals of the two orbital planes
	Vector nm1 = crossp (sv, sp);
	Vector nm2 = crossp (tgt->GVel()-ref->GVel(), tgt->GPos()-ref->GPos());
	nm1.unify();
	nm2.unify();
	// relative inclination between ship's and target's orbital planes
	double reli = xangle (nm1, nm2);

	// calculate ship time to reference point
	double myta = myel->TrueAnm();
	arg = sqrt ((1.0-myel->e)/(1.0+myel->e));
	ea  = 2.0 * atan (arg * tan (0.5*rlng));          // ship's eccentric anomaly at ref. axis
	rma = ea - myel->e * sin(ea);                     // ship's mean anomaly at ref. axis
	ea  = 2.0 * atan (arg * tan (0.5*myta));          // ship's current eccentric anomaly
	cma = ea - myel->e * sin(ea);                     // ship's current mean anomaly
	T0  = myel->OrbitT();                             // ship's orbit period
	dt0 = T0/Pi2 * (rma-cma);                         // ship's time to ref. axis
	if (dt0 < 0.0) dt0 += T0; else if (dt0 >= T0) dt0 -= T0;

	// calculate target time to reference point
	double tgta = tgtel->TrueAnm();
	arg = sqrt ((1.0-tgtel->e)/(1.0+tgtel->e));
	ea  = 2.0 * atan (arg * tan (0.5*(rlng-domega))); // target eccentric anomaly at ref. axis
	rma = ea - tgtel->e * sin(ea);                    // target mean anomaly at ref. axis
	ea  = 2.0 * atan (arg * tan (0.5*tgta));          // target current eccentric anomaly
	cma = ea - tgtel->e * sin(ea);                    // target current mean anomaly
	T1  = tgtel->OrbitT();                            // target orbit period
	dt1 = T1/Pi2 * (rma-cma);                         // target time to ref. axis
	if (dt1 < 0.0) dt1 += T1; else if (dt1 >= T1) dt1 += T1;

	// longitude difference
	double dlng = normangle(tgta+domega-myta);
	sprintf (cbuf, "%s%7.2fº", orbiter::Tr("DLng"), Deg(dlng));
	skp->Text (x, y, cbuf, strlen(cbuf)); y += ch;
	// target distance
	Vector rp = tgt->GPos()-vessel->GPos();
	sprintf (cbuf, "%s%s", orbiter::Tr("Dist"), DistStr (rp.length()));
	skp->Text (x, y, cbuf, strlen(cbuf)); y += ch;
	// target velocity
	Vector rv = tgt->GVel()-vessel->GVel();
	sprintf (cbuf, "%s%s", orbiter::Tr("RVel"), DistStr (rv.length()));
	skp->Text (x, y, cbuf, strlen(cbuf)); y += ch;

	// find best match
	double mindt = 1e10;
	int mini = 0, minj = 0;
	for (i = 0; i < norbit; i++) {
		double t0 = dt0 + i*T0;
		for (j = 0; j < norbit; j++) {
			double t1 = dt1 + j*T1;
			double dt = fabs(t0-t1);
			if (dt < mindt) mini = i, minj = j, mindt = dt;
		}
	}
	sprintf (cbuf, "%s%s", orbiter::Tr("DTmin"), DistStr (mindt));
	skp->Text (x, y, cbuf, strlen(cbuf)); y += ch;

	sprintf (cbuf, "%s%7.2fº", orbiter::Tr("RInc"), Deg(reli));
	if (reli > RAD*1.0) skp->SetTextColor (draw[3][0].col);
	skp->Text (x, IH-(3*ch)/2, cbuf, strlen(cbuf));
	skp->SetTextColor (draw[0][0].col);

	y = 1 + (ch*3)/2;
	// ---- 中文布局偏置（官方坐标为基准，仅中文模式追加）----
	// 中文模式：三列按表头实测宽度紧凑排列，并整体右对齐到 MFD 右边缘；
	// 英文模式：保持官方固定列布局。
	const bool zh = orbiter::IsChinese();
	if (zh) {
		const char *h0 = orbiter::Tr("Ob");
		const char *h1 = orbiter::Tr("Sh-ToR");
		const char *h2 = orbiter::Tr("Tg-ToR");
		int w0 = (int)skp->GetTextWidth(h0, (int)strlen(h0));
		int w1 = (int)skp->GetTextWidth(h1, (int)strlen(h1));
		int w2 = (int)skp->GetTextWidth(h2, (int)strlen(h2));
		int gap = cw;            // 列间距（紧凑）
		int rightMargin = cw/2;  // 右边距
		x  = IW - rightMargin - (w0 + gap + w1 + gap + w2);
		x0 = x + w0 + gap;
		x1 = x0 + w1 + gap;
	} else {
		x  = IW - (33*cw)/2;          // original layout
		x0 = x + 2*cw;
		x1 = x + 9*cw;
	}
	if (zh) {
		char hl0[16], hl1[32], hl2[32];
		strcpy(hl0, orbiter::Tr("Ob"));
		strcpy(hl1, orbiter::Tr("Sh-ToR"));
		strcpy(hl2, orbiter::Tr("Tg-ToR"));
		skp->Text(x,  y, hl0, (int)strlen(hl0));
		skp->Text(x0, y, hl1, (int)strlen(hl1));
		skp->Text(x1, y, hl2, (int)strlen(hl2));
	} else {
		skp->Text(x, y, "Ob Sh-ToR Tg-ToR", 16);
	}
	y += ch;
	for (i = 0; i < norbit; i++) {
		sprintf (cbuf, "%2d", i); skp->Text (x, y, cbuf, strlen(cbuf));
		if (i == mini) skp->SetTextColor (draw[1][0].col);
		strcpy (cbuf, DistStr(dt0)); skp->Text (x0, y, cbuf, strlen(cbuf));
		if (i == mini) skp->SetTextColor (draw[0][0].col);
		if (i == minj) skp->SetTextColor (draw[1][0].col);
		strcpy (cbuf, DistStr(dt1)); skp->Text (x1, y, cbuf, strlen(cbuf));
		if (i == minj) skp->SetTextColor (draw[0][0].col);
		dt0 += T0, dt1 += T1; y += ch;
	}
}

bool Instrument_OSync::KeyImmediate (char *kstate)
{
	if (KEYDOWN (kstate, OAPI_KEY_COMMA)) { // rotate ref axis
		if (BufKey (OAPI_KEY_COMMA, 0.1) && mode == MODE_MANUAL) {
			man_rlng = posangle (man_rlng-RAD);
			man_sinr = sin(man_rlng), man_cosr = cos(man_rlng);
			Refresh();
		}
		return true;
	}
	if (KEYDOWN (kstate, OAPI_KEY_PERIOD)) { // rotate ref axis
		if (BufKey (OAPI_KEY_PERIOD, 0.1) && mode == MODE_MANUAL) {
			man_rlng = posangle (man_rlng+RAD);
			man_sinr = sin(man_rlng), man_cosr = cos(man_rlng);
			Refresh();
		}
		return true;
	}
	return false;
}

bool Instrument_OSync::KeyBuffered (DWORD key)
{
	switch (key) {
	case OAPI_KEY_M:  // switch mode
		mode = (Mode)(((int)mode+1)%7);
		Refresh();
		return true;
	case OAPI_KEY_N:  // number of transit times
		g_input->Open ("Transit list length:", 0, 20, Instrument_OSync::CallbackNorbit, (void*)this);
		return true;
	case OAPI_KEY_T:  // select target
		OpenSelect_Tgt ("Sync MFD: Target", ClbkEnter_Tgt, vessel->ElRef(), 2);
		return true;
	}
	return false;
}

bool Instrument_OSync::ProcessButton (int bt, int event)
{
	static const DWORD btkey[5] = { OAPI_KEY_T, OAPI_KEY_M, OAPI_KEY_N, OAPI_KEY_COMMA, OAPI_KEY_PERIOD };
	if (event & PANEL_MOUSE_LBDOWN) {
		if (bt < 3) return KeyBuffered (btkey[bt]);
	} else if (event & PANEL_MOUSE_LBPRESSED) {
		if (bt >= 3 && bt < 5) return KeyImmediate (KstateSet (btkey[bt]));
	}
	return false;
}

const char *Instrument_OSync::BtnLabel (int bt) const
{
	static const char *label[5] = { "TGT", "MOD", "LEN", "R-", "R+" };
	return (bt < 5 ? label[bt] : 0);
}

int Instrument_OSync::BtnMenu (const MFDBUTTONMENU **menu) const
{
	static const MFDBUTTONMENU mnu[5] = {
		{orbiter::Tr("Select target"), 0, 'T'},
		{orbiter::Tr("Toggle intersect"), 0, 'M'},
		{orbiter::Tr("Select # transit"), 0, 'N'},
		{orbiter::Tr("Rot. manual axis"), 0, ','},
		{orbiter::Tr("Rot. manual axis"), 0, '.'}
	};
	if (menu) *menu = mnu;
	return 5;
}

void Instrument_OSync::SetTarget (const RigidBody *target)
{
	if (tgt != target) {
		tgt = target;
	}
}

bool Instrument_OSync::ClbkEnter_Tgt (Select *menu, int item, char *str, void *data)
{
	Body *obj = g_psys->GetObj (str, true);
	if (obj) {
		Instrument_OSync *instr = (Instrument_OSync*)data;
		instr->SetTarget ((RigidBody*)obj);
		instr->Refresh();
	}
	return (obj != 0);
}

bool Instrument_OSync::CallbackNorbit (InputBox*, char *str, void *data)
{
	int no;
	if (sscanf (str, "%d", &no)) {
		Instrument_OSync *instr = (Instrument_OSync*)data;
		int maxno = instr->IH/instr->ch - 3;
		if (no <= 0) no = 1;
		else if (no > maxno) no = maxno;
		instr->norbit = no;
		instr->Refresh();
		return true;
	}
	return false;
}

void Instrument_OSync::SetSize (const Spec &spec)
{
	pixrad = (spec.w*4)/9;
	ICNTX  = spec.w/2;
	ICNTY  = spec.h/2;
}

int Instrument_OSync::ProcessMessage (int msg, void *data)
{
	switch (msg) {
	case MSG_KILLVESSEL:
		if ((Body*)data == tgt) tgt = 0; // current target object was destroyed
		return 1;
	}
	return 0;
}

bool Instrument_OSync::ReadParams (ifstream &ifs)
{
	char cbuf[256], ctgt[128] = "", cmode[128] = "", *pc;
	int i, llen = 0;
	double rlng = 0.0;
	if (!FindScnHeader (ifs)) return false;
	for (;;) {
		if (!ifs.getline (cbuf, 256)) return false;
		pc = trim_string (cbuf);
		if (!_strnicmp (pc, "END_MFD", 7)) break;
		if (!_strnicmp (pc, "TARGET", 6)) {
			strcpy (ctgt, trim_string (pc+6));
		} else if (!_strnicmp (pc, "MODE", 4)) {
			strcpy (cmode, trim_string (pc+4));
		} else if (!_strnicmp (pc, "MANUALREF", 9)) {
			sscanf (pc+9, "%lf", &rlng);
			rlng *= RAD;
		} else if (!_strnicmp (pc, "LISTLEN", 7)) {
			sscanf (pc+7, "%d", &llen);
		}
	}
	if (ctgt[0]) {
		Body *obj = g_psys->GetObj (ctgt, true);
		if (obj) SetTarget ((RigidBody*)obj);
	}
	if (cmode[0]) {
		for (i = 0; i < 7; i++)
			if (!_stricmp (cmode, modestr[i])) {
				mode = (Mode)i;
				break;
			}
	}
	if (mode == MODE_MANUAL) {
		man_rlng = rlng;
		man_cosr = cos(rlng), man_sinr = sin(rlng);
	}
	if (llen) norbit = llen;
	return true;
}

void Instrument_OSync::WriteParams (ostream &ofs) const
{
	ofs << "  TYPE OSync" << endl;
	if (tgt) ofs << "  TARGET " << tgt->Name() << endl;
	ofs << "  MODE " << modestr[mode] << endl;
	if (mode == MODE_MANUAL)
		ofs << "  MANUALREF " << DEG*man_rlng << endl;
	ofs << "  LISTLEN " << norbit << endl;
}

