// ==============================================================
// TabExtraContact.h
// Launchpad "Extra" page: contact model configuration (project extension).
//
// Included at the end of TabExtra.h. The implementation lives in
// TabExtraContact.inc (included at the end of TabExtra.cpp).
// ==============================================================

#ifndef __TABEXTRACONTACT_H
#define __TABEXTRACONTACT_H

class ExtraContact: public BuiltinLaunchpadItem {
public:
	ExtraContact (const orbiter::ExtraTab *tab): BuiltinLaunchpadItem (tab) {}
	char *Name ();
	char *Description ();
	bool clbkOpen (HWND hParent);

private:
	void InitDialog (HWND hWnd);
	void ResetDialog (HWND hWnd);
	void SetDialog (HWND hWnd, const CFG_PHYSICSPRM &prm);
	bool StoreParams (HWND hWnd);
	static INT_PTR CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif // !__TABEXTRACONTACT_H
