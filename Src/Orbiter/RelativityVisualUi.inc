// ============================================================================
// RelativityVisualUi.inc
// Host-side UI helpers for the relativistic visual effects (Options -> Visual):
//   - exposure slider label update
//   - scroll handler of the exposure slider
//
// Included by Src/Orbiter/OptionsPages.cpp. The OptionsPage_Visual class
#include "resource.h"   // control IDs (inc may be included before the host's own resource.h include)
// declaration (OptionsPages.h) only carries the member declaration for
// OnHScroll; everything else lives here.
// ============================================================================

// Update the "K=x.x / off" readout of the relativistic Doppler exposure slider.
// The rightmost tick (8) means "exposure adaptation disabled".
static void UpdateRelExposureLabel (HWND hPage)
{
	int pos = (int)SendDlgItemMessage (hPage, IDC_OPT_VIS_RELEXPOSURE, TBM_GETPOS, 0, 0);
	char buf[32];
	if (pos >= 8) strcpy_s (buf, 32, "off");
	else sprintf_s (buf, 32, "K=%.1f", 1.0 + (pos - 1) * 0.5);
	SetWindowTextA (GetDlgItem (hPage, IDC_OPT_VIS_RELEXP_DSP), buf);
}

BOOL OptionsPage_Visual::OnHScroll (HWND hPage, WPARAM wParam, LPARAM lParam)
{
	if (HWND(lParam) == GetDlgItem (hPage, IDC_OPT_VIS_RELEXPOSURE)) {
		UpdateRelExposureLabel (hPage);
	}
	return FALSE;
}
