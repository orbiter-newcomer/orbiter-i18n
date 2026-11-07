// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#include "MfdComms.h"
#include "Localization.h"
#include "Nav.h"
#include <stdio.h>
#include "Orbiter.h"

using namespace std;

extern TimeData td;

// =======================================================================
// class Instrument_Comms
// navigation radio manipulation

Instrument_Comms::Instrument_Comms (Pane *_pane, INT_PTR _id, const Spec &spec, Vessel *_vessel)
: Instrument (_pane, _id, spec, _vessel)
{
	sel = 0;
	scanning = 0;
	nsel = vessel->nnav;
	if (vessel->xpdr) nsel++;

	if (gc) {
		brush[0] = gc->clbkCreateBrush (col_green2);
		brush[1] = gc->clbkCreateBrush (col_yellow2);
	} else {
		for (int i = 0; i < 2; i++) brush[i] = 0;
	}
}

Instrument_Comms::~Instrument_Comms ()
{
	if (gc)
		for (int i = 0; i < 2; i++)
			if (brush[i]) gc->clbkReleaseBrush (brush[i]);
}

HELPCONTEXT *Instrument_Comms::HelpTopic () const
{
	extern HELPCONTEXT DefHelpContext;
	DefHelpContext.topic = (char*)"/mfd_com.htm";
	return &DefHelpContext;
}

bool Instrument_Comms::Update (double upDTscale)
{
	const double stept = 0.05;
	if (scanning && td.SysT1-t_scan > stept) {
		const Nav *nav;
		if (nav = vessel->nav[sel].sender)
			scanning = false;
		else {
			SwitchFreq (sel, scanning, false);
			t_scan = td.SysT1+stept;
			Refresh();
		}
	}

	return Instrument::Update (upDTscale);
}

void Instrument_Comms::UpdateDraw (oapi::Sketchpad *skp)
{
	DWORD n;
	int i, sig, len, x = cw/2, y = (3*ch)/2, dy = ch, xx, yy, dd;
	char cbuf[256];
	float freq;
	const Nav *nav;

	for (n = 0; n < vessel->nnav; n++) {
		sprintf (cbuf, "%s%d：%6.2f MHz", orbiter::Tr("NAV"), n+1, vessel->nav[n].freq);
		skp->SetTextColor (draw[n == sel ? 1:0][0].col);
		int rowW = (int)skp->GetTextWidth(cbuf, (int)strlen(cbuf));
		skp->Text (x, y, cbuf, strlen (cbuf)); y += dy;
		if (nav = vessel->nav[n].sender) {
			len = nav->IdString (cbuf, 256);
			skp->Text (x+rowW+cw, y, cbuf, min (len, 30));
			sig = min(5,1+(int)(0.5*log(nav->FieldStrength(vessel->GPos()))));
			skp->SetBrush (brush[n == sel ? 1:0]);
			skp->SetPen (GetDefaultPen (n == sel ? 1:0, 1));
			xx = x+cw*20; yy = y-dy+(ch-cw)/2, dd = (cw*2)/3;
			for (i = 0; i < sig; i++) {
				skp->Rectangle (xx, yy, xx+dd, yy+cw);
				xx += cw;
			}
			skp->SetBrush (0);
			for (; i < 5; i++) {
				skp->Rectangle (xx, yy, xx+dd, yy+cw);
				xx += cw;
			}
		} else {
			skp->Text (x+rowW+cw, y, "-----------", 11);
		}
		y += (3*dy)/2;
	}
	if (vessel->GetXpdrFreq (freq)) {
		skp->SetPen (GetDefaultPen (0));
		skp->Line (x, y, IW-x, y); y += dy/2;
		skp->SetTextColor (draw[2][0].col);
		skp->Text (x, y, orbiter::Tr("XPDR Transmitter"), (int)strlen(orbiter::Tr("XPDR Transmitter"))); y += (3*dy)/2;
		skp->SetTextColor (draw[sel == vessel->nnav ? 1:0][0].col);
		sprintf (cbuf, "%s：%6.2f MHz", orbiter::Tr("XPDR"), freq);
		int rowW = (int)skp->GetTextWidth(cbuf, (int)strlen(cbuf));
		skp->Text (x, y, cbuf, strlen (cbuf)); y += dy;
	}

	DisplayTitle (skp, orbiter::Tr("NAV Receiver Stack"));
}

bool Instrument_Comms::KeyBuffered (DWORD key)
{
	switch (key) {
	case OAPI_KEY_COMMA: // select radio (up)
		scanning = 0;
		if (sel) { sel--; Refresh(); }
		return true;
	case OAPI_KEY_PERIOD:
		scanning = 0;
		if (sel < nsel-1) { sel++; Refresh(); }
		return true;
	case OAPI_KEY_MINUS:
		scanning = 0;
		SwitchFreq (sel, -20, false);
		Refresh();
		return true;
	case OAPI_KEY_EQUALS:
		scanning = 0;
		SwitchFreq (sel, 20, false);
		Refresh();
		return true;
	case OAPI_KEY_LBRACKET:
		scanning = 0;
		SwitchFreq (sel, -1, true);
		Refresh();
		return true;
	case OAPI_KEY_RBRACKET:
		scanning = 0;
		SwitchFreq (sel, 1, true);
		Refresh();
		return true;
	case OAPI_KEY_Z:
		if (sel < vessel->nnav) {
			SwitchFreq (sel, -1, false);
			scanning = -1;
			t_scan = td.SysT0;
			Refresh();
		}
		return true;
	case OAPI_KEY_X:
		if (sel < vessel->nnav) {
			SwitchFreq (sel, 1, false);
			scanning = 1;
			t_scan = td.SysT0;
			Refresh();
		}
		return true;
	}
	return false;
}

void Instrument_Comms::SwitchFreq (DWORD line, int step, bool minor)
{
	if (minor) {
		DWORD ch = (line < vessel->nnav ? vessel->GetNavChannel(line) : vessel->GetXpdrChannel());
		DWORD minor = ch % 20;
		DWORD major = ch / 20;
		if (step > 0) minor = (minor < 19 ? minor+1 : 0);
		else          minor = (minor >  0 ? minor-1 : 19);
		step = major*20 + minor - ch;
	}
	if (line < vessel->nnav) vessel->IncNavChannel (line, step);
	else                     vessel->IncXpdrChannel (step);
}

bool Instrument_Comms::ProcessButton (int bt, int event)
{
	static const DWORD btkey[8] = { OAPI_KEY_COMMA, OAPI_KEY_PERIOD, OAPI_KEY_LBRACKET, OAPI_KEY_MINUS,
		OAPI_KEY_RBRACKET, OAPI_KEY_EQUALS, OAPI_KEY_Z, OAPI_KEY_X
	};
	if (event & PANEL_MOUSE_LBDOWN) {
		if (bt < 8) return KeyBuffered (btkey[bt]);
	}
	return false;
}

const char *Instrument_Comms::BtnLabel (int bt) const
{
	static const char *label[8] = {"SL-", "SL+", " < ", "<<", " > ", ">>", "SC<", "SC>" };
	return (bt < 8 ? label[bt] : 0);
}

int Instrument_Comms::BtnMenu (const MFDBUTTONMENU **menu) const
{
	static const MFDBUTTONMENU mnu[8] = {
		{orbiter::Tr("Select receiver"), 0, ','},
		{orbiter::Tr("Select receiver"), 0, '.'},
		{orbiter::Tr("Dec frequency"), orbiter::Tr("(step 0.05)"), '['},
		{orbiter::Tr("Dec frequency"), orbiter::Tr("(step 1.0)"), '-'},
		{orbiter::Tr("Inc frequency"), orbiter::Tr("(step 0.05)"), ']'},
		{orbiter::Tr("Inc frequency"), orbiter::Tr("(step 1.0)"), '='},
		{orbiter::Tr("Scan down"), 0, 'z'},
		{orbiter::Tr("Scan up"), 0, 'x'},
	};
	if (menu) *menu = mnu;
	return 8;
}

void Instrument_Comms::WriteParams (ostream &ofs) const
{
	ofs << "  TYPE COM/NAV" << endl;
}

