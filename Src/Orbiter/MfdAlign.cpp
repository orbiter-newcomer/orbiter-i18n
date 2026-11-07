// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#include "MfdAlign.h"
#include "Localization.h"
#include "Pane.h"
#include "Planet.h"
#include "Psys.h"
#include "Log.h"
#include "Select.h"
#include <iomanip>

using namespace std;

extern PlanetarySystem *g_psys;
extern TimeData td;
extern InputBox *g_input;
extern Select *g_select;
extern char DBG_MSG[256];

static const Body *last_target = 0;

// =======================================================================
// class Instrument_OPlaneAlign

struct Instrument_OPlaneAlign::SavePrm Instrument_OPlaneAlign::saveprm = {0,0,0,0.0,0.0,false};

Instrument_OPlaneAlign::Instrument_OPlaneAlign (Pane *_pane, INT_PTR _id, const Spec &spec, Vessel *_vessel, bool restore)
: Instrument (_pane, _id, spec, _vessel)
{
	shpel = new Elements(); TRACENEW
	tgtel = new Elements(); TRACENEW
	mode = ORBIT;
	automode = true;
	if (restore && vessel == saveprm.usr) {
		SetRef (saveprm.elref ? saveprm.elref : _vessel->ElRef());
		SetTarget (saveprm.tgt);
		customel = saveprm.customel;
		if (customel)
			SetCustomEls (saveprm.i, saveprm.theta);
	} else {
		SetRef(_vessel->ElRef());
		tgt = 0;
		customel = false;
	}
	if (elref) {
		shpel->SetMasses(_vessel->Mass(), elref->Mass());
		shpel->Calculate(vessel->GPos() - elref->GPos(), vessel->GVel() - elref->GVel(), td.SimT1);
		shpel->Setup (_vessel->Mass(), elref->Mass(), _vessel->Els()->MJDepoch());
		if (tgt) {
			tgtel->SetMasses(tgt->Mass(), elref->Mass());
			tgtel->Calculate(tgt->GPos() - elref->GPos(), tgt->GVel() - elref->GVel(), td.SimT1);
			tgtel->Setup(tgt->Mass(), elref->Mass(), _vessel->Els()->MJDepoch());
		}
	}

	pphi = -1.0;
	preli = 0.0;
	engage = false;
}

Instrument_OPlaneAlign::~Instrument_OPlaneAlign ()
{
	// save status
	saveprm.usr      = vessel;
	saveprm.elref    = elref;
	saveprm.tgt      = tgt;
	saveprm.customel = customel;
	if (customel) {
		saveprm.i     = tgtel->i;
		saveprm.theta = tgtel->theta;
	}
	delete shpel;
	delete tgtel;
}

HELPCONTEXT *Instrument_OPlaneAlign::HelpTopic () const
{
	extern HELPCONTEXT DefHelpContext;
	DefHelpContext.topic = (char*)"/mfd_align.htm";
	return &DefHelpContext;
}

bool Instrument_OPlaneAlign::SelectRef (char *name)
{
	CelestialBody *obj = g_psys->GetGravObj (name, true);
	if (!obj) return false;
	SetRef (obj);
	return true;
}

void Instrument_OPlaneAlign::SelectAutoRef ()
{
	SetRef (vessel->ElRef());
}

void Instrument_OPlaneAlign::SetRef (const CelestialBody *cbody)
{
	if (cbody != elref) {
		elref = cbody;
		shpel->SetMasses (0.0, elref->Mass());
		tgtel->SetMasses (0.0, elref->Mass());
		refrad = elref->Size();
		Refresh();
	}
}

void Instrument_OPlaneAlign::SetTarget (const Body *target)
{
	if (target == tgt) return;
	tgt = target;
	if (target) {
		last_target = target;
		if (elref) {
			tgtel->SetMasses(tgt->Mass(), elref->Mass());
			tgtel->Calculate(tgt->GPos() - elref->GPos(), tgt->GVel() - elref->GVel(), td.SimT1);
			tgtel->Setup(tgt->Mass(), elref->Mass(), shpel->MJDepoch());
		}
		customel = false;
		pphi = -1.0;
		Refresh();
	}
}

void Instrument_OPlaneAlign::SetCustomEls (double i, double theta)
{
	tgtel->SetMasses (0.0, elref->Mass());
	tgtel->Reset (1.0, 0.0, i, theta, 0.0, 0.0, td.MJD_ref);
	tgt = 0;
	customel = true;
}

void Instrument_OPlaneAlign::UpdateDraw (oapi::Sketchpad *skp)
{
	char cbuf[128];
	int x = cw/2, x2 = x + cw*11, x3 = x2 + cw*11, y = 1+(ch*5)/2, y1;

	// update my elements
	double refrad = elref->Size();
	Vector sp = vessel->GPos() - elref->GPos();
	Vector sv = vessel->GVel() - elref->GVel();
	shpel->Calculate(sp, sv, td.SimT1);
	bool subsurf_node = (shpel->PeDist() < refrad);

	if (automode) {
		if (vessel->GroundContact())
			mode = SURFACE;
		else if (subsurf_node)
			mode = BALLISTIC;
		else
			mode = ORBIT;
	}

	DisplayTitle (skp, orbiter::Tr("Align plane"));
	static const char *modestr[3] = { "Orbit  ", "Ballist", "Surface" };
	if (elref) {
		skp->Text (cw*17, 1, orbiter::Tr(elref->Name()), min ((size_t)40, strlen(orbiter::Tr(elref->Name()))));
		strcpy (cbuf, tgt ? orbiter::Tr(tgt->Name()) : customel ? orbiter::Tr("[Custom]") : orbiter::Tr("[None]"));
		skp->Text (cw*17, 1+ch, cbuf, strlen (cbuf));
		skp->Text(cw * 5, 1 + ch, orbiter::Tr(modestr[mode]), (int)strlen(orbiter::Tr(modestr[mode])));
		skp->SetTextColor (draw[2][1].col);
		skp->Text ((cw*27)/2, 1, orbiter::Tr("Ref"), (int)strlen(orbiter::Tr("Ref")));
		skp->Text ((cw*27)/2, 1+ch, orbiter::Tr("Tgt"), (int)strlen(orbiter::Tr("Tgt")));
		skp->Text(x, 1 + ch, orbiter::Tr(automode ? "Auto" : "Mode"), (int)strlen(orbiter::Tr(automode ? "Auto" : "Mode")));
	} else {
		skp->SetTextColor (draw[0][0].col);
		skp->Text (x, y, orbiter::Tr("No reference body selected."), (int)strlen(orbiter::Tr("No reference body selected.")));
		return;
	}
	skp->SetTextColor (draw[0][0].col);

	if (!(tgt || customel)) {
		skp->Text (x, y, orbiter::Tr("No target orbit selected."), (int)strlen(orbiter::Tr("No target orbit selected."))); y += ch;
		skp->Text (x, y, orbiter::Tr("Use Shift-T to select a target"), (int)strlen(orbiter::Tr("Use Shift-T to select a target"))); y += ch;
		skp->Text (x, y, orbiter::Tr("or Shift-E to define a custom"), (int)strlen(orbiter::Tr("or Shift-E to define a custom"))); y+= ch;
		skp->Text (x, y, orbiter::Tr("orbit."), (int)strlen(orbiter::Tr("orbit.")));
		return;
	}

	// update target elements
	Vector tp, tv;
	if (tgt) {
		tp = tgt->GPos() - elref->GPos();
		tv = tgt->GVel() - elref->GVel();
		tgtel->Calculate(tp, tv, td.SimT1);
	}

	bool have_intersection = true;

	if (mode == BALLISTIC) {
		if (shpel->PeDist() < refrad) {
			double a = (shpel->ApDist() + refrad) * 0.5; // target semimajor axis
			double pd = refrad; // target periapsis distance
			double le = a - pd; // linear eccentricity
			double e = le / a; // target eccentricity
			shpel->Reset(a, e, shpel->i, shpel->theta, shpel->omegab, shpel->L, shpel->MJDepoch());
		}
	}

	// ship in planet coords
	double svmag = sv.length();
	sp.unify(), sv.unify();
	// normals of the two orbital planes
	Vector nm1 = shpel->HVec().unit();
	Vector nm2 = tgtel->HVec().unit();

	double reli = xangle(nm1, nm2);    // relative inclination
	double didt = (reli - preli) / dT; // inclination rate
	preli = reli;                      // remember for next step

	// angle between ship and ascending node
	Vector nd = crossp(nm1, nm2);
	nd.unify();
	double cosp = dotp(nd, sp);
	double Aan = acos(cosp);
	if (dotp(nm1, crossp(nd, sp)) < 0) Aan = Pi2 - Aan; // ascending node is behind us
	double Adn = posangle(Aan + Pi);
	double Aan_signed = (Aan < Pi ? Aan : Pi2 - Aan);

	// true anomalies of ascending and descending node
	double trs = shpel->TrueAnm();
	double tra = normangle(trs + Aan);
	double trd = normangle(trs + Adn);
	// mean anomalies
	double mas = shpel->MeanAnm();
	double maa = shpel->MeanAnomaly_from_TrueAnomaly(tra);
	double mad = shpel->MeanAnomaly_from_TrueAnomaly(trd);
	// mean anomaly differences
	double dmaa = posangle(maa - mas);
	double dmad = posangle(mad - mas);

	double Tan = dmaa / Pi2 * shpel->OrbitT(); // time to ascending node
	double Tdn = dmad / Pi2 * shpel->OrbitT(); // time to descending node

	double vsurf;
	if (mode == SURFACE) { // modify timings assuming we are sitting on the surface
		have_intersection = GetTimingsFromSurface(Tan, Aan, Tdn, Adn, vsurf);
		Aan_signed = (Aan < Pi ? Aan : Pi2 - Aan);
	}

	bool an_is_next = (have_intersection ? Tan < Tdn : true);
	bool an_is_closest = (fabs(Aan_signed) < PI05);
	double Tnode = (an_is_next ? Tan : Tdn); // time to next node

	double T0 = (an_is_closest ? Tan : Tdn);
	if (an_is_closest != an_is_next)
		T0 -= (mode == SURFACE ? elref->RotT() : shpel->OrbitT());

	// orbital speeds
	double van = (mode == SURFACE ? vsurf : shpel->Spd_TA(tra));
	double vdn = (mode == SURFACE ? vsurf : shpel->Spd_TA(trd));
	// dV requirements (this assumes that the thrust is applied normal to the orbital plane throughout
	// the burn - this is less efficient than pointing directly in the dV direction, where dV = 2*sin(reli/2)*V)
	double dVan = van*reli;
	double dVdn = vdn*reli;
	// burn times
	double Th_main = vessel->GetThrusterGroupMaxth(THGROUP_MAIN);
	double acc = Th_main / vessel->Mass();
	double burnTan = dVan / acc;
	double burnTdn = dVdn / acc;
	double burnT = (an_is_closest ? burnTan : burnTdn);
	if (!engage) {
		engage = (mode == SURFACE ? T0 < burnT*0.5 && T0 > 0 : (T0 < burnT*0.5 && T0 > -burnT*0.5));
		isasc = (mode == ORBIT ? an_is_next : an_is_closest);
	} 
	else {
		if (mode == ORBIT || mode == BALLISTIC) {
			if (isasc != an_is_closest)
				engage = false;
		}
		else if (have_intersection) {
			if (isasc != an_is_next)
				engage = false;
		}
		else {
			if (Tan > elref->RotT()*0.5)
				engage = false;
		}
	}

	// output relevant elements of current and target orbits, and relative inclination + rate
	{
		int i;
		char hcol[3][32], r1[3][64], r2[3][64];
		strcpy(hcol[0], orbiter::Tr("Current"));
		strcpy(hcol[1], orbiter::Tr("Target"));
		strcpy(hcol[2], orbiter::Tr("Relative"));
		sprintf(r1[0], "%s% 7.2f°", orbiter::Tr("Inc"), Deg(shpel->i));
		sprintf(r1[1], "%s% 7.2f°", orbiter::Tr("Inc"), Deg(tgtel->i));
		sprintf(r1[2], "%s%7.2f°", orbiter::Tr("RInc"), Deg(reli));
		sprintf(r2[0], "%s% 7.2f°", orbiter::Tr("LAN"), Deg(shpel->theta));
		sprintf(r2[1], "%s% 7.2f°", orbiter::Tr("LAN"), Deg(tgtel->theta));
		sprintf(r2[2], "%s %+7.3f°/s", orbiter::Tr("R"), Deg(didt));
		int colx[3], colw[3];
		for (i = 0; i < 3; i++) {
			colw[i] = (int)skp->GetTextWidth(hcol[i], (int)strlen(hcol[i]));
			int w = (int)skp->GetTextWidth(r1[i], (int)strlen(r1[i]));
			if (w > colw[i]) colw[i] = w;
			w = (int)skp->GetTextWidth(r2[i], (int)strlen(r2[i]));
			if (w > colw[i]) colw[i] = w;
			colw[i] += cw;
		}
		colx[0] = x; colx[1] = colx[0]+colw[0]; colx[2] = colx[1]+colw[1];
		skp->SetPen(draw[0][1].solidpen);
		for (i = 0; i < 3; i++) skp->Text(colx[i], y, hcol[i], (int)strlen(hcol[i]));
		y += ch+1;
		for (i = 0; i < 3; i++) skp->Line(colx[i], y, colx[i]+colw[i]-cw/2, y);
		y += 1;
		for (i = 0; i < 3; i++) skp->Text(colx[i], y, r1[i], (int)strlen(r1[i]));
		y += ch;
		for (i = 0; i < 3; i++) skp->Text(colx[i], y, r2[i], (int)strlen(r2[i]));
		y += 2 * ch;
	}

	// node encounter data
	{
		char h1[32], h2[32];
		strcpy(h1, orbiter::Tr("dA[°]"));
		strcpy(h2, orbiter::Tr("TtN[s]"));
		int lw = (int)skp->GetTextWidth("CE ", 3);
		int w1 = (int)skp->GetTextWidth(h1, (int)strlen(h1));
		int w1b = (int)skp->GetTextWidth(" 999.9", 6);
		if (w1b > w1) w1 = w1b;
		int w2 = (int)skp->GetTextWidth(h2, (int)strlen(h2));
		int w2b = (int)skp->GetTextWidth("999.9k", 6);
		if (w2b > w2) w2 = w2b;
		int c0 = x, c1 = x + lw + cw, c2 = c1 + w1 + cw;
		skp->Text(x, y, orbiter::Tr("Node encounter"), (int)strlen(orbiter::Tr("Node encounter"))); y += ch;
		skp->Text(c1, y, h1, (int)strlen(h1));
		skp->Text(c2, y, h2, (int)strlen(h2));
		y += ch + 1;
		skp->Line(c1, y, c1 + w1, y);
		skp->Line(c2, y, c2 + w2, y); y += 1;
		if (have_intersection) {
			{
				char lab[8], vda[32], vttn[32];
				strcpy(lab, "AN");
				sprintf(vda, "%6.1f", Aan*DEG);
				strcpy(vttn, FloatStr(Tan, 3));
				int yy = (an_is_next ? y : y + ch);
				skp->Text(c0, yy, lab, 2);
				skp->Text(c1, yy, vda, (int)strlen(vda));
				skp->Text(c2, yy, vttn, (int)strlen(vttn));
			}
			y += ch;
			{
				char lab[8], vda[32], vttn[32];
				strcpy(lab, "DN");
				sprintf(vda, "%6.1f", Adn*DEG);
				strcpy(vttn, FloatStr(Tdn, 3));
				int yy = (an_is_next ? y : y - ch);
				skp->Text(c0, yy, lab, 2);
				skp->Text(c1, yy, vda, (int)strlen(vda));
				skp->Text(c2, yy, vttn, (int)strlen(vttn));
			}
			y += 2 * ch;
		}
		else {
			char lab[8], vda[32], vttn[32];
			strcpy(lab, "CE");
			sprintf(vda, "%6.1f", Aan*DEG);
			strcpy(vttn, FloatStr(Tan, 3));
			skp->Text(c0, y, lab, 2);
			skp->Text(c1, y, vda, (int)strlen(vda));
			skp->Text(c2, y, vttn, (int)strlen(vttn));
			y += 3 * ch;
		}
	}

	// thrust estimates
	{
		char hdir[16], hdv[32], hbt[32], httb[32];
		strcpy(hdir, orbiter::Tr("Dir"));
		strcpy(hdv, orbiter::Tr("dV[m/s]"));
		strcpy(hbt, orbiter::Tr("BT[s]"));
		strcpy(httb, orbiter::Tr(mode == SURFACE ? "TtL[s]" : "TtB[s]"));
		int wdir = (int)skp->GetTextWidth(hdir, (int)strlen(hdir));
		{ int w = (int)skp->GetTextWidth("AN ", 3); if (w > wdir) wdir = w; }
		int wnml = (int)skp->GetTextWidth("NML+ ", 5);
		{ int w = (int)skp->GetTextWidth(orbiter::Tr("NML+"), (int)strlen(orbiter::Tr("NML+"))); if (w > wnml) wnml = w; }
		int wdv = (int)skp->GetTextWidth(hdv, (int)strlen(hdv));
		int wbt = (int)skp->GetTextWidth(hbt, (int)strlen(hbt));
		int wttb = (int)skp->GetTextWidth(httb, (int)strlen(httb));
		int cx0 = x, cx1 = cx0 + wdir + cw, cx2 = cx1 + wnml + cw,
			cx3 = cx2 + wdv + cw, cx4 = cx3 + wbt + cw;
		y = IH - (9*ch)/2;
		sprintf(cbuf, "%s [%sN]", orbiter::Tr("Thrust"), FloatStr(Th_main, 4)+1);
		skp->Text(x, y, cbuf, (int)strlen(cbuf)); y += ch;
		skp->Text(cx1, y, hdir, (int)strlen(hdir));
		skp->Text(cx2, y, hdv, (int)strlen(hdv));
		skp->Text(cx3, y, hbt, (int)strlen(hbt));
		skp->Text(cx4, y, httb, (int)strlen(httb));
		y += ch + 1;
		skp->Line(cx1, y, cx1+wnml, y);
		skp->Line(cx2, y, cx2+wdv, y);
		skp->Line(cx3, y, cx3+wbt, y);
		skp->Line(cx4, y, cx4+wttb, y);
		y += 1;
		{
			char dir[8], nml[16], vdv[32], vbt[32], vttb[32];
			if (have_intersection) { strcpy(dir, "AN"); strcpy(nml, orbiter::Tr("NML-")); }
			else { strcpy(dir, "CE"); strcpy(nml, "90°"); }
			strcpy(vdv, FloatStr(dVan));
			strcpy(vbt, FloatStr(burnTan, 3));
			int yy = (an_is_next ? y : y + ch);
			skp->Text(cx0, yy, dir, (int)strlen(dir));
			skp->Text(cx1, yy, nml, (int)strlen(nml));
			skp->Text(cx2, yy, vdv, (int)strlen(vdv));
			skp->Text(cx3, yy, vbt, (int)strlen(vbt));
			if (!engage || !an_is_closest) {
				strcpy(vttb, FloatStr(Tan - burnTan*0.5, 3));
				skp->Text(cx4, yy, vttb, (int)strlen(vttb));
			}
			else {
				skp->SetTextColor(draw[1][0].col);
				skp->Text(cx4, yy, orbiter::Tr(mode == SURFACE ? "[LAUNCH]" : "[BURN]"), (int)strlen(orbiter::Tr(mode == SURFACE ? "[LAUNCH]" : "[BURN]")));
				skp->SetTextColor(draw[0][0].col);
			}
		}
		y += ch;
		if (have_intersection) {
			char dir[8], nml[16], vdv[32], vbt[32], vttb[32];
			strcpy(dir, "DN"); strcpy(nml, orbiter::Tr("NML+"));
			strcpy(vdv, FloatStr(dVdn));
			strcpy(vbt, FloatStr(burnTdn, 3));
			int yy = (an_is_next ? y : y - ch);
			skp->Text(cx0, yy, dir, (int)strlen(dir));
			skp->Text(cx1, yy, nml, (int)strlen(nml));
			skp->Text(cx2, yy, vdv, (int)strlen(vdv));
			skp->Text(cx3, yy, vbt, (int)strlen(vbt));
			if (!engage || an_is_closest) {
				strcpy(vttb, FloatStr(Tdn - burnTdn*0.5, 3));
				skp->Text(cx4, yy, vttb, (int)strlen(vttb));
			}
			else {
				skp->SetTextColor(draw[1][0].col);
				skp->Text(cx4, yy, orbiter::Tr(mode == SURFACE ? "[LAUNCH]" : "[BURN]"), (int)strlen(orbiter::Tr(mode == SURFACE ? "[LAUNCH]" : "[BURN]")));
				skp->SetTextColor(draw[0][0].col);
			}
		}
	}
	// subsurface warning
	if (mode == ORBIT && subsurf_node || !have_intersection) {
		skp->SetTextColor(draw[1][1].col);
		y = ch * 12;
		skp->SetFont(GetDefaultFont(1));
		if (!have_intersection) {
			skp->Text(x, y, orbiter::Tr("No intersection with target"), (int)strlen(orbiter::Tr("No intersection with target"))); y += ch;
			skp->Text(x, y, orbiter::Tr("plane at launch latitude."), (int)strlen(orbiter::Tr("plane at launch latitude.")));
		} else {
			skp->Text(x, y, orbiter::Tr("Trajectory intersects surface"), (int)strlen(orbiter::Tr("Trajectory intersects surface"))); y += ch;
			skp->Text(x, y, orbiter::Tr("Data may be inaccurate."), (int)strlen(orbiter::Tr("Data may be inaccurate.")));
		}
	}

	// graphics
	int RD, CX, CY;
	RD = (IW*25)/100;
	CX = IW-RD-cw*2;
	CY = (55*IH) / 100;
	int xa, ya, xd, yd;
	Matrix irot(IRotMatrix(shpel->cost, shpel->sint, shpel->cosi, shpel->sini));
	Vector P = mul(irot, sp);
	x = (int)(RD*P.x);
	y = (int)(RD*P.z);
	if (mode == ORBIT) {
		P = mul(irot, nd);
		xa = (int)(RD*P.x);
		ya = -(int)(RD*P.z);
		xd = -xa;
		yd = -ya;
	}
	else {
		double tht = atan2(P.z, P.x);
		double thtan = tht + Aan;
		xa = (int)(cos(thtan)*RD);
		ya = -(int)(sin(thtan)*RD);
		double thtdn = tht + Adn;
		xd = (int)(cos(thtdn)*RD);
		yd = -(int)(sin(thtdn)*RD);
	}
	skp->SetPen (draw[1][0].solidpen);
	skp->SetFont(GetDefaultFont(1));
	skp->SetTextColor(draw[1][0].col);
	skp->SetTextAlign(oapi::Sketchpad::CENTER);
	if (have_intersection) {
		skp->Line(CX + xa, CY + ya, CX + xd, CY + yd); // node line
		xa = (xa * 10) / 9;  ya = (ya * 10) / 9;
		xd = (xd * 10) / 9;  yd = (yd * 10) / 9;
		skp->Text(CX + xa, CY + ya - ch / 2, orbiter::Tr("AN"), (int)strlen(orbiter::Tr("AN")));
		skp->Text(CX + xd, CY + yd - ch / 2, orbiter::Tr("DN"), (int)strlen(orbiter::Tr("DN")));
	}
	else {
		skp->Line(CX + xa - cw / 2, CY + ya, CX + xa + cw / 2, CY + ya);
		skp->Line(CX + xa, CY + ya - cw / 2, CX + xa, CY + ya + cw / 2);
		xa = (xa * 10) / 9;  ya = (ya * 10) / 9;
		skp->Text(CX + xa, CY + ya - ch / 2, "CE", 2);
	}
	skp->SetPen (draw[0][0].solidpen);
	skp->Ellipse (CX-RD, CY-RD, CX+RD+1, CY+RD+1);
	skp->Line (CX, CY, CX+x, CY-y); // radius vector
	x = (x*10)/9;  y = (y*10)/9;
	skp->SetTextColor(draw[0][0].col);
	skp->Text (CX+x, CY-y-ch/2, "P", 1);
	pphi = Aan;
	pT = td.SimT1;
}

bool Instrument_OPlaneAlign::GetTimingsFromSurface(double &Tan, double &Aan, double &Tdn, double &Adn, double &VSurf)
{
	// target plane normal
	Vector nm2 = tgtel->HVec().unit();
	nm2 = tmul(elref->GRot(), nm2); // rotate into frame of reference planet
	double nx = nm2.x, ny = nm2.y, nz = nm2.z;

	// ship position in reference body frame
	Vector p = elref->GlobalToLocal(vessel->GPos());
	double y0 = p.y;
	double r0 = hypot(p.x, p.z);
	double phi0 = atan2(p.x, p.z);

	// velocity of surface point
	double rotT = elref->RotT();
	VSurf = r0 * Pi2 / rotT;

	// Find intersection of trajectory of surface point with target orbital plane
	// Assume that vessel remains fixed in reference body frame (e.g. parked on surface)
	// Then the trajectory is a circle in the plane of current y0 = p.y with radius r0 = hypot(p.x, p.z)
	// So equations of trajectory in space:
	//    sqrt(x^2 + z^2) = r0
	//    y = y0
	// Equation of target orbital plane with normal N = (nx, ny, nz):
	//    nx*x + ny*y + nz*z = 0
	// Solving this leads to:
	double a = 1.0 + (nx*nx) / (nz*nz);
	double b = 2.0*nx*ny*y0 / (nz*nz);
	double c = ny*ny*y0*y0 / (nz*nz) - r0*r0;

	double arg = b*b - 4.0*a*c;
	if (arg >= 0.0) {
		double x, z, phi1, phi2, s = sqrt(b*b - 4.0*a*c);
		x = (-b + s) / (2.0*a);
		z = (-nx*x - ny*y0) / nz;
		phi1 = atan2(x, z);
		if (phi1 > phi0) phi1 -= PI2;
		Aan = phi0 - phi1;
		Tan = Aan / PI2 * rotT;

		x = (-b - s) / (2.0*a);
		z = (-nx*x - ny*y0) / nz;
		phi2 = atan2(x, z);
		if (phi2 > phi0) phi2 -= PI2;
		Adn = phi0 - phi2;
		Tdn = Adn / PI2 * rotT;

		double dT = (Tan > Tdn ? Tan - Tdn : Tan + rotT - Tdn);
		if (dT > 0.5*rotT) {
			double A = Aan; Aan = Adn; Adn = A;
			double T = Tan; Tan = Tdn; Tdn = T;
		}
		return true;
	}
	else { // no itersections of vessel trajectory with target plane
		// find points of closest approach instead
		double phi = atan2(-nm2.x, -nm2.z);
		if (phi > phi0) phi -= PI2;
		Aan = phi0 - phi;
		Tan = Aan / PI2 * rotT;
		Adn = 0.0;
		Tdn = 0.0;
		return false;
	}
}

bool Instrument_OPlaneAlign::KeyBuffered (DWORD key)
{
	switch (key) {
	case OAPI_KEY_A:  // auto reference
		SelectAutoRef ();
		return true;
	case OAPI_KEY_E:  // custom elements
		g_input->Open ("Ecliptic inclination and longitude of asc. node [deg.]:", 0, 30, Instrument_OPlaneAlign::CallbackElements, (void*)this);
		return true;
	case OAPI_KEY_R:  // select reference
		OpenSelect_CelBody ("Align MFD: Reference", ClbkEnter_Ref);
		return true;
	case OAPI_KEY_T:  // select target
		OpenSelect_Tgt ("Align MFD: Target", ClbkEnter_Tgt, elref, 0);
		return true;
	case OAPI_KEY_M:  // mode selection
		CycleModes();
		return true;
	}
	return false;
}

bool Instrument_OPlaneAlign::ProcessButton (int bt, int event)
{
	static const DWORD btkey[5] = { OAPI_KEY_R, OAPI_KEY_A, OAPI_KEY_T, OAPI_KEY_E, OAPI_KEY_M };
	if (event & PANEL_MOUSE_LBDOWN) {
		if (bt < 5) return KeyBuffered (btkey[bt]);
	}
	return false;
}

const char *Instrument_OPlaneAlign::BtnLabel (int bt) const
{
	static const char *label[5] = { "REF", "AR", "TGT", "ELS", "MOD" };
	return (bt < 5 ? label[bt] : 0);
}

int Instrument_OPlaneAlign::BtnMenu (const MFDBUTTONMENU **menu) const
{
	static const MFDBUTTONMENU mnu[5] = {
		{orbiter::Tr("Orbit reference"), 0, 'R'},
		{orbiter::Tr("Auto reference"), 0, 'A'},
		{orbiter::Tr("Select target"), 0, 'T'},
		{orbiter::Tr("Select elements"), 0, 'E'},
		{orbiter::Tr("Cycle modes"), 0, 'M'}
	};
	if (menu) *menu = mnu;
	return 5;
}

bool Instrument_OPlaneAlign::ClbkEnter_Ref (Select *menu, int item, char *str, void *data)
{
	Instrument_OPlaneAlign *mfd = (Instrument_OPlaneAlign*)data;
	return mfd->SelectRef (str);
}

bool Instrument_OPlaneAlign::ClbkEnter_Tgt (Select *menu, int item, char *str, void *data)
{
	Instrument_OPlaneAlign* mfd = (Instrument_OPlaneAlign*)data;
	return mfd->SelectTarget (str);
}

bool Instrument_OPlaneAlign::CallbackElements (InputBox*, char *str, void *data)
{
	Instrument_OPlaneAlign* instr = (Instrument_OPlaneAlign*)data;
	double i, theta;
	if (sscanf (str, "%lf%lf", &i, &theta) == 2) {
		instr->SetCustomEls (i*RAD, theta*RAD);
		return true;
	}
	return false;
}

bool Instrument_OPlaneAlign::SelectTarget (char *name)
{
	const Body *obj = g_psys->GetObj (name, true);
	if (obj && obj != elref) {
		SetTarget (obj);
		return true;
	}
	return false;
}

void Instrument_OPlaneAlign::CycleModes()
{
	if (automode) {
		automode = false;
		mode = ORBIT;
	}
	else if (mode == SURFACE) {
		automode = true;
	}
	else {
		mode = (Mode)(mode + 1);
	}
	Refresh();
}

int Instrument_OPlaneAlign::ProcessMessage (int msg, void *data)
{
	switch (msg) {
	case MSG_KILLVESSEL:
		if ((Body*)data == tgt) {
			tgt = 0; // current target object was destroyed
		}
		return 1;
	}
	return 0;
}

bool Instrument_OPlaneAlign::ReadParams (ifstream &ifs)
{
	if (!FindScnHeader (ifs)) return false;
	char cbuf[256], cref[128] = "", ctgt[128] = "", *pc;
	double i, theta;
	bool customels = false;
	for (;;) {
		if (!ifs.getline (cbuf, 256)) return false;
		pc = trim_string (cbuf);
		if (!_strnicmp (pc, "END_MFD", 7)) break;
		if (!_strnicmp (pc, "TARGET", 6)) {
			strcpy (ctgt, trim_string (pc+6));
		}
		else if (!_strnicmp(pc, "REF", 3)) {
			strcpy(cref, trim_string(pc + 3));
		} else if (!_strnicmp(pc, "MODE", 4)) {
			int m;
			sscanf(pc + 4, "%d", &m);
			mode = (Mode)m;
		} else if (!_strnicmp (pc, "TGTELS", 6)) {
			sscanf (pc+6, "%lf%lf", &i, &theta);
			customels = true;
		}
	}
	if (cref[0]) SelectRef (cref);
	if (ctgt[0]) SelectTarget (ctgt);
	if (customels) SetCustomEls (RAD*i, RAD*theta);
	return true;
}

void Instrument_OPlaneAlign::WriteParams (ostream &ofs) const
{
	ofs << "  TYPE OAlign" << endl;
	if (elref)
		ofs << "  REF " << elref->Name() << endl;
	if (mode != ORBIT)
		ofs << "  MODE " << (int)mode << endl;
	if (tgt)
		ofs << "  TARGET " << tgt->Name() << endl;
	else if (customel)
		ofs << "  TGTELS " << setprecision(4) << DEG*tgtel->i << ' ' << DEG*tgtel->theta << endl;
}
