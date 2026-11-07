// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#include "MfdOrbit.h"
#include "Localization.h"
#include "Pane.h"
#include "Celbody.h"
#include "Psys.h"
#include "Select.h"
#include "Log.h"

using namespace std;

extern PlanetarySystem *g_psys;
extern TimeData td;
extern Pane *g_pane;
extern InputBox *g_input;
extern Select *g_select;
extern char DBG_MSG[256];

// =======================================================================
// class Instrument_Orbit

struct Instrument_Orbit::SavePrm Instrument_Orbit::saveprm = {
	0, 0, 0,
	Instrument_Orbit::PRJ_FRM,
	Instrument_Orbit::FRM_ECL,
	Instrument_Orbit::DIST_RAD,
	Instrument_Orbit::DISP_BOTH
};

Instrument_Orbit::Instrument_Orbit (Pane *_pane, INT_PTR _id, const Spec &spec, Vessel *_vessel)
: Instrument (_pane, _id, spec, _vessel)
{
	projmode = saveprm.projmode;
	frmmode  = saveprm.frmmode;
	dstmode  = saveprm.dstmode;
	dispmode = saveprm.dispmode;
	if (_vessel == saveprm.usr) {
		elref = (saveprm.elref ? saveprm.elref : _vessel->ElRef());
		tgt = saveprm.tgt;
	} else {
		elref = _vessel->ElRef();
		tgt = 0;
	}
	shpel = new Elements(); TRACENEW
	tgtel = new Elements(); TRACENEW
	if (elref) {
		shpel->Setup (_vessel->Mass(), elref->Mass(), _vessel->Els()->MJDepoch());
		if (tgt)
			tgtel->Setup (tgt->Mass(), elref->Mass(), _vessel->Els()->MJDepoch());
	}

	SetSize (spec);
	strcpy (title_str, "Orbit: ");
	strcpy (proj_str, "Proj: ");

	// init drawing resources
	if (gc) {
		brush[0]    = gc->clbkCreateBrush (col_green2);
		brush[1]    = gc->clbkCreateBrush (col_yellow2);
	} else {
		int i;
		for (i = 0; i < 2; i++) brush[i] = 0;
	}
}

Instrument_Orbit::~Instrument_Orbit ()
{
	int i;
	if (gc) {
		for (i = 0; i < 2; i++)
			if (brush[i]) gc->clbkReleaseBrush (brush[i]);
	}
	delete shpel;
	delete tgtel;

	// save status
	saveprm.usr      = vessel;
	saveprm.elref    = elref;
	saveprm.tgt      = tgt;
	saveprm.projmode = projmode;
	saveprm.frmmode  = frmmode;
	saveprm.dstmode  = dstmode;
	saveprm.dispmode = dispmode;
}

HELPCONTEXT *Instrument_Orbit::HelpTopic () const
{
	extern HELPCONTEXT DefHelpContext;
	DefHelpContext.topic = (char*)"/mfd_orbit.htm";
	return &DefHelpContext;
}

bool Instrument_Orbit::KeyBuffered (DWORD key)
{
	switch (key) {
	case OAPI_KEY_A:  // auto reference
		SelectAutoRef ();
		return true;
	case OAPI_KEY_D:  // toggle distance display mode
		dstmode = (dstmode == DIST_RAD ? DIST_ALT : DIST_RAD);
		Refresh();
		return true;
	case OAPI_KEY_F:  // reference frame
		frmmode = (frmmode == FRM_ECL ? FRM_EQU : FRM_ECL);
		Refresh();
		return true;
	case OAPI_KEY_H:  // copy reference to HUD
		CopyToHUD ();
		return true;
	case OAPI_KEY_M:  // display mode
		dispmode = (DisplayMode)((((int)dispmode)+1) % 3);
		Refresh();
		return true;
	case OAPI_KEY_N:  // deselect target
		UnselectTarget ();
		return true;
	case OAPI_KEY_P:  // projection plane
		projmode = (ProjectionMode)((((int)projmode)+1) % 3);
		if (projmode == PRJ_TGT && !tgt) projmode = PRJ_FRM;
		Refresh();
		return true;
	case OAPI_KEY_R:  // select reference
		OpenSelect_CelBody ("Orbit MFD: Reference", ClbkEnter_Ref);
		return true;
	case OAPI_KEY_T:  // select target
		OpenSelect_Tgt ("Orbit MFD: Target", ClbkEnter_Tgt, elref, 0);
		return true;
	}
	return false;
}

bool Instrument_Orbit::ProcessButton (int bt, int event)
{
	static const DWORD btkey[9] = { OAPI_KEY_R, OAPI_KEY_A, OAPI_KEY_T, OAPI_KEY_N, OAPI_KEY_M, OAPI_KEY_F, OAPI_KEY_P, OAPI_KEY_D, OAPI_KEY_H };
	if (event & PANEL_MOUSE_LBDOWN) {
		if (bt < 9) return KeyBuffered (btkey[bt]);
	}
	return false;
}

const char *Instrument_Orbit::BtnLabel (int bt) const
{
	static const char *label[9] = { "REF", "AR", "TGT", "NT", "MOD", "FRM", "PRJ", "DST", "HUD" };
	return (bt < 9 ? label[bt] : 0);
}

int Instrument_Orbit::BtnMenu (const MFDBUTTONMENU **menu) const
{
	static const MFDBUTTONMENU mnu[9] = {
		{orbiter::Tr("Orbit reference"), 0, 'R'},
		{orbiter::Tr("Auto reference"), 0, 'A'},
		{orbiter::Tr("Select target"), 0, 'T'},
		{orbiter::Tr("Unselect target"), 0, 'N'},
		{orbiter::Tr("Display mode"), 0, 'M'},
		{orbiter::Tr("Reference frame"), 0, 'F'},
		{orbiter::Tr("Projection plane"), 0, 'P'},
		{orbiter::Tr("Distance display"), 0, 'D'},
		{orbiter::Tr("Copy data to HUD"), 0, 'H'}
	};
	if (menu) *menu = mnu;
	return 9;
}

bool Instrument_Orbit::ClbkEnter_Tgt (Select *menu, int item, char *str, void *data)
{
	Instrument_Orbit *instr = (Instrument_Orbit*)data;
	return instr->SelectTarget (str);
}

bool Instrument_Orbit::ClbkEnter_Ref (Select *menu, int item, char *str, void *data)
{
	Instrument_Orbit* instr = (Instrument_Orbit*)data;
	return instr->SelectRef (str);
}

void Instrument_Orbit::SetRef (const CelestialBody *ref)
{
	if (ref && ref != elref) {
		elref = ref;
		shpel->SetMasses (0.0, elref->Mass());
		tgtel->SetMasses (0.0, elref->Mass());
		if (tgt == (const Body*)ref) UnselectTarget();
		Refresh();
	}
}

bool Instrument_Orbit::SelectRef (char *str)
{
	CelestialBody *obj = g_psys->GetGravObj (str, true);
	if (!obj) return false;
	SetRef (obj);
	return true;
}

void Instrument_Orbit::SelectAutoRef ()
{
	SetRef (vessel->ElRef());
}

bool Instrument_Orbit::SelectTarget (char *str)
{
	Body *body = g_psys->GetObj (str, true);
	if (body == (Body*)elref) return false;
	if (body) {
		tgt = body;
		if (elref)
			tgtel->Setup (tgt->Mass(), elref->Mass(), shpel->MJDepoch());
		Refresh();
		return true;
	} else return false;
}

void Instrument_Orbit::UnselectTarget ()
{
	tgt = 0;
	if (projmode == PRJ_TGT) projmode = PRJ_FRM;
	Refresh();
}

void Instrument_Orbit::CopyToHUD () const
{
	g_pane->SetHUDMode (HUD_ORBIT);
	HUD *hud = g_pane->GetHUD();
	if (hud && hud->Mode() == HUD_ORBIT) {
		((HUD_Orbit*)hud)->SetReference (elref);
	}
}

void Instrument_Orbit::DisplayOrbit (oapi::Sketchpad *skp, int which, oapi::IVECTOR2 *p)
{
	skp->SetPen (GetDefaultPen (which));
	if (p[ELN+2].x != -1) { // closed orbit
		// ellipse
		skp->Polygon (p, ELN);
		// apoapsis
		oapi::IVECTOR2 &pa = p[ELN+2];
		skp->Ellipse (pa.x-3, pa.y-3, pa.x+4, pa.y+4);
	} else {                // open orbit
		// hyperbola
		skp->Polyline (p, ELN-1);
	}
	// radius vector
	skp->Line (ICNTX, ICNTY, p[ELN].x, p[ELN].y);
	// periapsis
	oapi::IVECTOR2 &pe = p[ELN+1];
	skp->SetBrush (brush[which]);
	skp->Ellipse (pe.x-3, pe.y-3, pe.x+4, pe.y+4);
	// ascending node
	oapi::IVECTOR2 &asc = p[ELN+3];
	skp->Rectangle (asc.x-3, asc.y-3, asc.x+4, asc.y+4);
	skp->SetBrush (0);
	// descending node
	oapi::IVECTOR2 &desc = p[ELN+4];
	skp->Rectangle (desc.x-3, desc.y-3, desc.x+4, desc.y+4);
	// line of nodes
	skp->SetPen (GetDefaultPen (which, 0, 2));
	skp->Line (asc.x, asc.y, desc.x, desc.y);
}

void Instrument_Orbit::UpdateDraw (oapi::Sketchpad *skp)
{
	static const char *projstr[3] = {"Ecliptic", "Ship plane", "Target plane"};
	Matrix rot1, rot2, irot;

	bool bValidShpEl = (elref != 0);
	bool bValidTgtEl = (elref && tgt);

	if (bValidShpEl) {
		Vector pos = vessel->GPos()-elref->GPos();
		Vector vel = vessel->GVel()-elref->GVel();
		if (frmmode == FRM_EQU) { // convert to equatorial frame
			pos = tmul (elref->RotObliq(), pos);
			vel = tmul (elref->RotObliq(), vel);
		}
		shpel->Calculate (pos, vel, td.SimT1);
		sprintf (title_str, "%s%s", orbiter::Tr("Orbit: "), orbiter::Tr(elref->Name()));
		sprintf (proj_str, "%s%s", orbiter::Tr("Proj: "), orbiter::Tr(projstr[projmode]));
		instable = (shpel->PeDist() < elref->Size());
		scale = pixrad / (shpel->e < 1.0 ? shpel->ApDist() :
						  max (2.0*shpel->PeDist(), shpel->Radius()));
	}
	if (bValidTgtEl) {
		Vector pos = tgt->GPos()-elref->GPos();
		Vector vel = tgt->GVel()-elref->GVel();
		if (frmmode == FRM_EQU) { // convert to equatorial frame
			pos = tmul (elref->RotObliq(), pos);
			vel = tmul (elref->RotObliq(), vel);
		}
		tgtel->Calculate (pos, vel, td.SimT1);
		double scale2 = (tgtel->e < 1.0 ? pixrad / tgtel->ApDist() : scale);
		if (scale2 < scale) scale = scale2;
	} else {
		if (projmode == PRJ_TGT) projmode = PRJ_SHIP;
	}

	if (elref)
		elref_rad = (int)(elref->Size() * scale + 0.5);

	if (dispmode != DISP_LIST) { // draw orbit graphs
		bool eclproj;

			skp->SetTextColor (draw[2][0].col);
			skp->Text (hx, 1, lbl, (int)strlen(lbl)); hx += lw + cw;
			skp->Text (hx, 1, frmStr, (int)strlen(frmStr)); hx += fw + cw*3;
			skp->SetTextColor (draw[2][1].col);
			skp->Text (hx, 1, plbl, (int)strlen(plbl)); hx += plw + cw;
			skp->Text (hx, 1, prjStr, (int)strlen(prjStr));
		}
		if (bValidShpEl && projmode == PRJ_SHIP) {
			irot = IRotMatrix (shpel->cost, shpel->sint, shpel->cosi, shpel->sini);
			eclproj = false;
		} else if (bValidTgtEl && projmode == PRJ_TGT) {
			irot = IRotMatrix (tgtel->cost, tgtel->sint, tgtel->cosi, tgtel->sini);
			eclproj = false;
		} else {
			irot.Set (IMatrix());
			eclproj = true;
		}
		skp->SetPen (GetDefaultPen (2, 1));
		skp->Ellipse (ICNTX-elref_rad, ICNTY-elref_rad, ICNTX+elref_rad, ICNTY+elref_rad);
		if (bValidShpEl) {
			rot1 = RotMatrix (shpel->coso, shpel->sino, shpel->cost, shpel->sint, shpel->cosi, shpel->sini);
			if (!eclproj) rot1.premul (irot);
			UpdateOrbitGraph (ICNTX, ICNTY, IW, IH, scale, shpel, rot1, irot, o_pt1);
			DisplayOrbit (skp, 0, o_pt1);
		}
		if (bValidTgtEl) {
			rot2 = RotMatrix (tgtel->coso, tgtel->sino, tgtel->cost, tgtel->sint, tgtel->cosi, tgtel->sini);
			if (!eclproj) rot2.premul (irot);
			UpdateOrbitGraph (ICNTX, ICNTY, IW, IH, scale, tgtel, rot2, irot, o_pt2);
			DisplayOrbit (skp, 1, o_pt2);
		}
	} else {
		{	// Frame header (list mode), right-aligned.
			const char *frmStr = orbiter::Tr(frmmode == FRM_ECL ? "ECL" : "EQU");
			const char *lbl = orbiter::Tr("Frm");
			int lw = (int)skp->GetTextWidth(lbl, (int)strlen(lbl));
			int fw = (int)skp->GetTextWidth(frmStr, (int)strlen(frmStr));
			int hx = IW - (lw + cw + fw) - cw;
			skp->SetTextColor (draw[2][0].col);
			skp->Text (hx, 1, lbl, (int)strlen(lbl)); hx += lw + cw;
			skp->Text (hx, 1, frmStr, (int)strlen(frmStr));
		}
	}

	DisplayTitle (skp, title_str);
	// output elref's contribution to gravity field
	bool dominant;
	double gfrac = g_psys->GetGravityContribution (elref, vessel->GPos(), &dominant);
	char cbuf[128];
	sprintf (cbuf, "%s %0.2f", orbiter::Tr("G"), gfrac);
	skp->SetTextColor (draw[!dominant ? 3 : gfrac < 0.0 ? 1 : 0][0].col);
	int gw = (int)skp->GetTextWidth(cbuf, (int)strlen(cbuf));
	skp->Text ((IW-gw)/2, IH-(3*ch)/2, cbuf, (int)strlen(cbuf));

	if (dispmode != DISP_GRAPH) { // list ship and target orbit parameters
		if (bValidShpEl) {
			skp->SetTextColor (draw[0][0].col);
			skp->Text (cw/2, (ch*3)/2, orbiter::Tr("--OSC.EL.--"), (int)strlen(orbiter::Tr("--OSC.EL.--")));
			DisplayElements (skp, shpel, cw/2, (ch*5)/2, 0, 0);
		}
		if (bValidTgtEl) {
			skp->SetTextColor (draw[1][0].col);
			int len = strlen (tgt->Name());
			if (len >= 11) {
				strncpy (cbuf, tgt->Name(), 11);
				cbuf[11] = '\0';
			} else {
				memset (cbuf, '-', 11);
				cbuf[11] = '\0';
				strncpy (cbuf+(11-len)/2, tgt->Name(), len);
			}
			// fixed-width box anchored at the right edge, left-aligned text
			// Measure the widest target row, then anchor the box so that its
			// widest row ends at the right edge of the MFD (text left-aligned).
			int tgtMaxW = 0;
			DisplayElements (skp, tgtel, 0, 0, 1, &tgtMaxW);
			int tgtX = IW - cw/2 - tgtMaxW;
			if (tgtX < IW/2) tgtX = IW/2;
			skp->Text (tgtX, (ch*3)/2, cbuf, (int)strlen(cbuf));
			DisplayElements (skp, tgtel, tgtX, (ch*5)/2, 0, 0);
		}
	}
}

// Helper for DisplayElements: either measures the rendered width of one row
// (used to size the target column box dynamically) or draws it.
static void DrawElemRow (oapi::Sketchpad *skp, int x, int y, const char *text,
	int measure, int *pMaxW)
{
	if (measure) {
		int w = (int)skp->GetTextWidth(text, (int)strlen(text));
		if (pMaxW && w > *pMaxW) *pMaxW = w;
	} else {
		skp->Text(x, y, text, (int)strlen(text));
	}
}
void Instrument_Orbit::DisplayElements (oapi::Sketchpad *skp, const Elements *el, int x, int y, int measure, int *pMaxW)
{
	char cbuf[128];
	int dy = ch;
	if (el) {
		if (el->e < 1.0) { // closed orbit
			sprintf (cbuf, "%s %s", orbiter::Tr("SMa"), el->a < 1e5*AU ? DistStr (el->a)+1 : orbiter::Tr("N/A"));
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %s", orbiter::Tr("SMi"), el->SMi() < 1e5*AU ? DistStr (el->SMi())+1 : orbiter::Tr("N/A"));
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			if (dstmode == DIST_RAD) {
				sprintf (cbuf, "%s %s", orbiter::Tr("PeR"), DistStr (el->PeDist())+1);
				DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
				sprintf (cbuf, "%s %s", orbiter::Tr("ApR"), el->ApDist() < 1e5*AU ? DistStr (el->ApDist())+1 : orbiter::Tr("N/A"));
				DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
				sprintf (cbuf, "%s %s", orbiter::Tr("Rad"), DistStr (el->Radius())+1);
				DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			} else {
				double r = elref->Size();
				sprintf (cbuf, "%s %s", orbiter::Tr("PeA"), DistStr (el->PeDist()-r)+1);
				DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
				sprintf (cbuf, "%s %s", orbiter::Tr("ApA"), el->ApDist()-r < 1e5*AU ? DistStr (el->ApDist()-r)+1 : orbiter::Tr("N/A"));
				DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
				sprintf (cbuf, "%s %s", orbiter::Tr("Alt"), DistStr (el->Radius()-r)+1);
				DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			}
			sprintf (cbuf, "%s% 0.4f", orbiter::Tr("Ecc"), el->e);
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %s", orbiter::Tr("T"), FloatStr (el->OrbitT())+1);
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %s", orbiter::Tr("PeT"), FloatStr (el->PeT())+1);
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %s", orbiter::Tr("ApT"), FloatStr (el->ApT())+1);
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %s", orbiter::Tr("Vel"), FloatStr (el->Vel())+1);
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %.2fº", orbiter::Tr("Inc"), Deg(el->i));
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %.2fº", orbiter::Tr("LAN"), Deg(el->theta));
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %.2fº", orbiter::Tr("LPe"), Deg(el->omegab));
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %.2fº", orbiter::Tr("AgP"), Deg(el->ArgPer()));
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %.2fº", orbiter::Tr("TrA"), Deg(el->TrueAnm()));
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %.2fº", orbiter::Tr("TrL"), Deg(el->TrueLng()));
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %.2fº", orbiter::Tr("MnA"), Deg(posangle(el->MeanAnm())));
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %.2fº", orbiter::Tr("MnL"), Deg(el->MeanLng()));
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
		} else { // open orbit
			sprintf (cbuf, "%s%s", orbiter::Tr("SMa"), -el->a < 1e5*AU ? DistStr (el->a) : orbiter::Tr(" N/A"));
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s%s", orbiter::Tr("SMi"), -el->SMi() < 1e5*AU ? DistStr (-el->SMi()) : orbiter::Tr(" N/A"));
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			if (dstmode == DIST_RAD) {
				sprintf (cbuf, "%s %s", orbiter::Tr("PeR"), DistStr (el->PeDist())+1);
				DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
				sprintf (cbuf, "%s %s", orbiter::Tr("ApR"), orbiter::Tr("N/A")); DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
				sprintf (cbuf, "%s %s", orbiter::Tr("Rad"), DistStr (el->Radius())+1);
				DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			} else {
				double r = elref->Size();
				sprintf (cbuf, "%s %s", orbiter::Tr("PeA"), DistStr (el->PeDist()-r)+1);
				DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
				sprintf (cbuf, "%s %s", orbiter::Tr("ApA"), orbiter::Tr("N/A")); DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
				sprintf (cbuf, "%s %s", orbiter::Tr("Alt"), DistStr (el->Radius()-r)+1);
				DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			}
			if      (el->e < 1e2) sprintf (cbuf, "%s %0.4f", orbiter::Tr("Ecc"), el->e);
			else if (el->e < 1e3) sprintf (cbuf, "%s %0.3f", orbiter::Tr("Ecc"), el->e);
			else if (el->e < 1e4) sprintf (cbuf, "%s %0.2f", orbiter::Tr("Ecc"), el->e);
			else if (el->e < 1e5) sprintf (cbuf, "%s %0.1f", orbiter::Tr("Ecc"), el->e);
			else {
				double e = el->e;
				int eexp = 0;
				while (e >= 10.0) e *= 0.1, eexp += 1;
				sprintf (cbuf, "%s %0.3fe%d", orbiter::Tr("Ecc"), e, eexp);
			}
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %s", orbiter::Tr("T"), orbiter::Tr("N/A")); DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %s", orbiter::Tr("PeT"), DistStr (el->PeT())+1);
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %s", orbiter::Tr("ApT"), orbiter::Tr("N/A")); DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %s", orbiter::Tr("Vel"), DistStr (el->Vel())+1);
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %.2fº", orbiter::Tr("Inc"), Deg(el->i));
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %.2fº", orbiter::Tr("LAN"), Deg(el->theta));
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %.2fº", orbiter::Tr("LPe"), Deg(el->omegab));
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %.2fº", orbiter::Tr("AgP"), Deg(el->ArgPer()));
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %.2fº", orbiter::Tr("TrA"), Deg(el->TrueAnm()));
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %.2fº", orbiter::Tr("TrL"), Deg(el->TrueLng()));
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %sº", orbiter::Tr("MnA"), FloatStr (DEG*el->MeanAnm(),4)+1);
			DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
			sprintf (cbuf, "%s %s", orbiter::Tr("MnL"), orbiter::Tr("N/A")); DrawElemRow (skp, x, y, cbuf, measure, pMaxW); y += dy;
		}
	} else {
		DrawElemRow (skp, x, y, orbiter::Tr("Orbital elements"), measure, pMaxW); y += dy;
		DrawElemRow (skp, x, y, orbiter::Tr("not available"), measure, pMaxW);
	}
}

int Instrument_Orbit::ProcessMessage (int msg, void *data)
{
	switch (msg) {
	case MSG_KILLVESSEL:
		if ((Body*)data == tgt) {
			tgt = 0; // current target object was destroyed
			if (projmode == PRJ_TGT) projmode = PRJ_FRM;
		}
		return 1;
	}
	return 0;
}

void Instrument_Orbit::SetSize (const Spec &spec)
{
	pixrad = (spec.w*4)/9;
	ICNTX = spec.w/2;
	ICNTY = spec.h/2;
}

bool Instrument_Orbit::ReadParams (ifstream &ifs)
{
	char cbuf[256], *pc;
	char cref[128] = "", ctgt[128] = "", cprj[128] = "";
	dstmode = DIST_RAD;
	if (!FindScnHeader (ifs)) return false;
	for (;;) {
		if (!ifs.getline (cbuf, 256)) return false;
		pc = trim_string (cbuf);
		if (!_strnicmp (pc, "END_MFD", 7)) break;
		if (!_strnicmp (pc, "PROJ", 4)) {
			strcpy (cprj, trim_string (pc+4));
		} else if (!_strnicmp (pc, "ALT", 3)) {
			dstmode = DIST_ALT;
		} else if (!_strnicmp (pc, "REF", 3)) {
			strcpy (cref, trim_string (pc+3));
		} else if (!_strnicmp (pc, "TARGET", 6)) {
			strcpy (ctgt, trim_string (pc+6));
		} else if (!_strnicmp (pc, "FRAME", 5)) {
			pc = trim_string (pc+5);
			if (!_stricmp (pc, "Ecliptic"))
				frmmode = FRM_ECL;
			else if (!_stricmp (pc, "Equator"))
				frmmode = FRM_EQU;
		}
	}
	if (cref[0]) SelectRef (cref);
	if (ctgt[0]) SelectTarget (ctgt);
	if (cprj[0]) {
		if (!_strnicmp (cprj, "Ship", 4))
			projmode = PRJ_SHIP;
		else if (!_strnicmp (cprj, "Target", 6) && tgt)
			projmode = PRJ_TGT;
		else if (!_strnicmp (cprj, "Ecliptic", 8) || !_strnicmp (cprj, "Frame", 5))
			projmode = PRJ_FRM;
	}
	return true;
}

void Instrument_Orbit::WriteParams (ostream &ofs) const
{
	ofs << "  TYPE Orbit" << endl;
	ofs << "  PROJ " <<
		(projmode == PRJ_FRM ? "Frame" :
		 projmode == PRJ_SHIP ? "Ship" : "Target") << endl;
	ofs << "  FRAME " <<
		(frmmode == FRM_ECL ? "Ecliptic" : "Equator") << endl;
	if (dstmode == DIST_ALT)
		ofs << "  ALT" << endl;
	if (elref) ofs << "  REF " << elref->Name() << endl;
	if (tgt) ofs << "  TARGET " << tgt->Name() << endl;
}
