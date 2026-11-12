// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ============================================================================
// Relativity extension: UI declarations.
//
// Hosts the launchpad "Relativity" page (gravity field solver) and the
// relativistic controls of the Options -> Visual / Physics pages.
// Implementation: RelativityUi.cpp.
// ============================================================================

#ifndef __RELATIVITYUI_H
#define __RELATIVITYUI_H

#include <windows.h>
#include "TabExtra.h"   // BuiltinLaunchpadItem, orbiter::ExtraTab

class Config;

// ---- Options dialog helpers (Options -> Visual / Physics) -------------------
void UpdateRelExposureLabel (HWND hPage);

void RelativityVisualUpdateControls (HWND hPage, Config *cfg);
void RelativityVisualUpdateConfig (HWND hPage, Config *cfg);
void RelativityVisualInitSlider (HWND hPage);

void RelativityPhysicsUpdateControls (HWND hPage, Config *cfg);
void RelativityPhysicsUpdateConfig (HWND hPage, Config *cfg);
void RelativityPhysicsToggle (HWND hPage, Config *cfg);

// ---- Launchpad Extra page: relativistic gravity field solver ---------------
class ExtraRelativity: public BuiltinLaunchpadItem {
public:
	ExtraRelativity (const orbiter::ExtraTab *tab): BuiltinLaunchpadItem (tab) {}
	char *Name ();
	char *Description ();
	bool clbkOpen (HWND hParent);

private:
	void InitDialog (HWND hWnd);
	void SetDialog (HWND hWnd);
	bool StoreParams (HWND hWnd);
	static INT_PTR CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif // !__RELATIVITYUI_H
