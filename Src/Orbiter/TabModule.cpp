// Copyright (c) Martin Schweiger
// Licensed under the MIT License

//=============================================================================
// ModuleTab class
//=============================================================================

#include <windows.h>
#include <cstdarg>
#include "Orbiter.h"
#include "Launchpad.h"
#include "TabModule.h"
#include "resource.h"
#include "Localization.h"
#include <filesystem>
namespace fs = std::filesystem;

using std::max;

extern char DBG_MSG[256];
static int counter = -1;
static bool inCheckCascade = false;   // guard against recursive TVN_ITEMCHANGED

// Temporary diagnostic log for the module tab, written to an independent file
// so that it also works in Launchpad mode (the Orbiter log may not be
// initialised there yet).
static void dbgModLog (const char *fmt, ...)
{
	FILE *f = NULL;
	if (fopen_s (&f, "ModuleTab.log", "a") != 0 || !f) return;
	va_list ap;
	va_start (ap, fmt);
	vfprintf (f, fmt, ap);
	va_end (ap);
	fprintf (f, "\n");
	fclose (f);
}

//-----------------------------------------------------------------------------

orbiter::ModuleTab::ModuleTab (const LaunchpadDialog *lp): LaunchpadTab (lp)
{
	nmodulerec = 0;
	pendCheckItem = NULL;
}

//-----------------------------------------------------------------------------

orbiter::ModuleTab::~ModuleTab ()
{
	int i;

	if (nmodulerec) {
		for (i = 0; i < nmodulerec; i++) {
			delete []modulerec[i]->name;
			modulerec[i]->name = NULL;
			if (modulerec[i]->info) {
				delete []modulerec[i]->info;
				modulerec[i]->info = NULL;
			}
			delete modulerec[i];
		}
		delete []modulerec;
		modulerec = NULL;
	}
}

//-----------------------------------------------------------------------------

void orbiter::ModuleTab::Create ()
{
	hTab = CreateTab (IDD_PAGE_MOD);

	r_lst0 = GetClientPos (hTab, GetDlgItem (hTab, IDC_MOD_TREE)); // REMOVE!
	r_dsc0 = GetClientPos (hTab, GetDlgItem (hTab, IDC_MOD_INFO)); // REMOVE!
	r_pane  = GetClientPos (hTab, GetDlgItem (hTab, IDC_MOD_SPLIT1));
	r_bt0 = GetClientPos (hTab, GetDlgItem (hTab, IDC_MOD_BUTTON1));
	r_bt1 = GetClientPos (hTab, GetDlgItem (hTab, IDC_MOD_BUTTON2));
	r_bt2 = GetClientPos (hTab, GetDlgItem (hTab, IDC_MOD_DEACTALL));
	splitListDesc.SetHwnd (GetDlgItem (hTab, IDC_MOD_SPLIT1), GetDlgItem (hTab, IDC_MOD_TREE), GetDlgItem (hTab, IDC_MOD_INFO));
}

//-----------------------------------------------------------------------------

BOOL orbiter::ModuleTab::OnInitDialog (HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	// Do NOT re-set the full tree style here: the original code replaced it
	// with a style list that omitted TVS_CHECKBOXES, which also discarded the
	// state image list the control creates for the check boxes - the saved
	// check states were then drawn blank/garbled. Keep the styles from the
	// dialog resource and only remove the horizontal scroll bar.
	LONG style = GetWindowLongPtr (GetDlgItem (hTab, IDC_MOD_TREE), GWL_STYLE);
	SetWindowLongPtr (GetDlgItem (hTab, IDC_MOD_TREE), GWL_STYLE, style & ~WS_HSCROLL);
	SetWindowPos (GetDlgItem (hTab, IDC_MOD_TREE), NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);

	return FALSE;
}

//-----------------------------------------------------------------------------

void orbiter::ModuleTab::GetConfig (const Config *cfg)
{
	RefreshLists();
	orbiter::SetWindowTextUtf8(GetDlgItem (hTab, IDC_MOD_INFO), orbiter::Tr("Optional Orbiter plugin modules.\r\n\r\nDouble-click on a category to show or hide its entries.\r\n\r\nCheck or uncheck items to activate the corresponding modules.\r\n\r\nSelect an item to see a description of the module function."));
	int listw = cfg->CfgWindowPos.LaunchpadModListWidth;
	if (!listw) {
		RECT r;
		GetClientRect (GetDlgItem (hTab, IDC_MOD_TREE), &r);
		listw = r.right;
	}
	splitListDesc.SetStaticPane (SplitterCtrl::PANE1, listw);
}

//-----------------------------------------------------------------------------

void orbiter::ModuleTab::SetConfig (Config *cfg)
{
	cfg->CfgWindowPos.LaunchpadModListWidth = splitListDesc.GetPaneWidth (SplitterCtrl::PANE1);
}

//-----------------------------------------------------------------------------

bool orbiter::ModuleTab::OpenHelp ()
{
	OpenTabHelp ("tab_modules");
	return true;
}

//-----------------------------------------------------------------------------

BOOL orbiter::ModuleTab::OnSize (int w, int h)
{
	int dw = w - (int)(pos0.right-pos0.left);
	int dh = h - (int)(pos0.bottom-pos0.top);
	int w0 = r_pane.right - r_pane.left; // initial splitter pane width
	int h0 = r_pane.bottom - r_pane.top; // initial splitter pane height

	// the elements below may need updating
	int lstw0 = r_lst0.right-r_lst0.left;
	int lsth0 = r_lst0.bottom-r_lst0.top;
	int dscw0 = r_dsc0.right-r_dsc0.left;
	int wg  = r_dsc0.right - r_lst0.left - lstw0 - dscw0;  // gap width
	int wl  = lstw0 + (dw*lstw0)/(lstw0+dscw0);
	wl = max (wl, lstw0/2);
	int xr = r_lst0.left+wl+wg;
	int wr = max(10,lstw0+dscw0+dw-wl);

	SetWindowPos (GetDlgItem (hTab, IDC_MOD_SPLIT1), NULL,
		0, 0, w0+dw, h0+dh,
		SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);
	SetWindowPos (GetDlgItem (hTab, IDC_MOD_BUTTON1), NULL,
		r_bt0.left, r_bt0.top+dh, 0, 0,
		SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOOWNERZORDER|SWP_NOZORDER);
	SetWindowPos (GetDlgItem (hTab, IDC_MOD_BUTTON2), NULL,
		r_bt1.left, r_bt1.top+dh, 0, 0,
		SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOOWNERZORDER|SWP_NOZORDER);
	SetWindowPos (GetDlgItem (hTab, IDC_MOD_DEACTALL), NULL,
		r_bt2.left, r_bt2.top+dh, 0, 0,
		SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOOWNERZORDER|SWP_NOZORDER);

	return NULL;
}

//-----------------------------------------------------------------------------

void orbiter::ModuleTab::Show ()
{
	LaunchpadTab::Show();
	// The tree control can reset its check states during/after its initial
	// layout (known issue, see InitActivation). Re-apply the saved selection
	// whenever the tab becomes visible, so the ticks match the configuration.
	PostMessage (hTab, WM_USER + 2, 0, 0);
	SetTimer (hTab, 1, 100, NULL);
}

//-----------------------------------------------------------------------------
void orbiter::ModuleTab::RefreshLists ()
{
	HWND hTree = GetDlgItem (hTab, IDC_MOD_TREE);
	TreeView_DeleteAllItems (hTree);

	TVINSERTSTRUCTW tvis;
	tvis.item.mask = TVIF_TEXT | TVIF_PARAM;

	int idx, len;
	char catstr[256];

	const fs::path moddir{ "Modules/Plugin" };

	for (const auto& file : fs::directory_iterator(moddir)) {
		if (file.path().extension().string() == ".dll") {
			auto name = file.path().filename().string();
			// add module record
			MODULEREC** tmp = new MODULEREC * [nmodulerec + 1];
			if (nmodulerec) {
				memcpy(tmp, modulerec, nmodulerec * sizeof(MODULEREC*));
				delete[]modulerec;
			}
			modulerec = tmp;

			MODULEREC* rec = modulerec[nmodulerec++] = new MODULEREC;
			len = name.length() - 4;
			rec->name = new char[len + 1];
			strncpy(rec->name, name.c_str(), len);
			rec->name[len] = '\0';
			rec->info = 0;
			rec->active = false;
			rec->locked = false;

			// check if module is set active in config
			if (pCfg->IsActiveModule(rec->name))
				rec->active = true;

			// check if module is set active in command line
			if (std::find(pCfg->CfgCmdlinePrm.LoadPlugins.begin(), pCfg->CfgCmdlinePrm.LoadPlugins.end(), rec->name) != pCfg->CfgCmdlinePrm.LoadPlugins.end()) {
				rec->active = true;
				rec->locked = true; // modules activated from the command line are not to be unloaded
			}

			HMODULE hMod = LoadLibraryEx((moddir / name).string().c_str(), 0, LOAD_LIBRARY_AS_DATAFILE);
			if (hMod) {
				char buf[1024];
				// read module info string
				if (LoadString(hMod, 1000, buf, 1024)) {
					buf[1023] = '\0';
					rec->info = new char[strlen(buf) + 1];
					strcpy(rec->info, buf);
				}
				// read category string
				if (LoadString(hMod, 1001, buf, 1024)) {
					strncpy(catstr, buf, 255);
					catstr[255] = '\0';
				}
				else {
					strcpy(catstr, "Miscellaneous");
				}
				FreeLibrary(hMod);
			}

			if (!strcmp(catstr, "Graphics engines"))
				continue; // graphics client modules are loaded via the Video tab

			// find the category entry
			HTREEITEM catItem = GetCategoryItem(catstr);

			// tree view entry (Unicode insert: the tree also contains translated
			// category names)
			std::wstring wmname = orbiter::Utf8ToWideString(rec->name);
			tvis.item.pszText = const_cast<LPWSTR>(wmname.c_str());
			tvis.item.lParam = (LPARAM)rec;
			tvis.hInsertAfter = TVI_SORT;
			tvis.hParent = catItem;
			HTREEITEM hti = (HTREEITEM)SendMessageW(hTree, TVM_INSERTITEMW, 0, (LPARAM)&tvis);
		}
	}
	counter = 0;
	{
		int nact = 0;
		for (int i = 0; i < nmodulerec; i++)
			if (modulerec[i]->active) nact++;
		
	}
}

HTREEITEM orbiter::ModuleTab::GetCategoryItem (char *cat)
{
	HWND hTree = GetDlgItem (hTab, IDC_MOD_TREE);
	HTREEITEM root = TreeView_GetRoot (hTree);
	// Category names are localized; the tree is stored as Unicode text.
	std::wstring wcat = orbiter::Utf8ToWideString(orbiter::Tr(cat));
	wchar_t wbuf[256];
	TVITEMW item;
	item.mask = TVIF_TEXT;
	item.pszText = wbuf;
	item.cchTextMax = 256;
	item.hItem = root;

	while (SendMessageW(hTree, TVM_GETITEMW, 0, (LPARAM)&item)) {
		if (!_wcsicmp (wcat.c_str(), wbuf)) return item.hItem;
		item.hItem = (HTREEITEM)SendMessageW(hTree, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)item.hItem);
	}
	// not found - create new category item
	TVINSERTSTRUCTW tvis;
	tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
	tvis.item.pszText = const_cast<LPWSTR>(wcat.c_str());
	tvis.item.lParam = NULL;
	tvis.hInsertAfter = TVI_SORT;
	tvis.hParent = NULL;
	return (HTREEITEM)SendMessageW(hTree, TVM_INSERTITEMW, 0, (LPARAM)&tvis);
}

void orbiter::ModuleTab::ExpandCollapseAll (bool expand)
{
	HWND hTree = GetDlgItem (hTab, IDC_MOD_TREE);
	UINT code = (expand ? TVE_EXPAND : TVE_COLLAPSE);
	TVITEM catitem;
	catitem.mask = NULL;
	catitem.hItem = TreeView_GetRoot (hTree);
	while (TreeView_GetItem (hTree, &catitem)) {
		TreeView_Expand (hTree, catitem.hItem, code);
		catitem.hItem = TreeView_GetNextSibling (hTree, catitem.hItem);
	}
}

void orbiter::ModuleTab::InitActivation ()
{
	HWND hTree = GetDlgItem (hTab, IDC_MOD_TREE);
	TVITEM catitem, subitem;
	catitem.mask = TVIF_PARAM;
	HTREEITEM hRoot = TreeView_GetRoot (hTree);
	catitem.hItem = hRoot;
	subitem.mask = TVIF_PARAM;

	// Suppress TVN_ITEMCHANGED processing while the initial check states are
	// applied: SetCheckState sends TVN_ITEMCHANGED synchronously and the
	// handler would call ActivateFromList(), which - seeing few ticks but a
	// full active list - would wrongly deactivate the not-yet-ticked modules
	// and wipe the saved configuration.
	inCheckCascade = true;

	// tick the active modules
	int nset = 0;
	while (TreeView_GetItem (hTree, &catitem)) {
		subitem.hItem = TreeView_GetChild (hTree, catitem.hItem);
		while (TreeView_GetItem (hTree, &subitem)) {
			MODULEREC *rec = (MODULEREC*)subitem.lParam;
			if (rec->active) {
				TreeView_SetCheckState (hTree, subitem.hItem, TRUE);
				nset++;
			}
			subitem.hItem = TreeView_GetNextSibling (hTree, subitem.hItem);
		}
		catitem.hItem = TreeView_GetNextSibling (hTree, catitem.hItem);
	}
	inCheckCascade = false;

	// Keep the check boxes on the category items: clicking a category check box
	// toggles all of its children (handled in OnNotify, TVN_ITEMCHANGED).
	ExpandCollapseAll (true);

	// The check states set above are sometimes undone by the tree control's
	// initial layout/painting. Re-apply them once the control has finished its
	// initialisation (see the WM_USER+2 handler in OnMessage) and once more
	// after a short delay (WM_TIMER), when the control is fully ready.
	PostMessage (hTab, WM_USER + 2, 0, 0);
	SetTimer (hTab, 1, 100, NULL);
}

void orbiter::ModuleTab::ApplyCheckStates ()
{
	HWND hTree = GetDlgItem(hTab, IDC_MOD_TREE);
	inCheckCascade = true;
	HTREEITEM hCat = TreeView_GetRoot(hTree);
	while (hCat) {
		HTREEITEM hChild = TreeView_GetChild(hTree, hCat);
		while (hChild) {
			TVITEM item;
			item.mask = TVIF_PARAM | TVIF_HANDLE;
			item.hItem = hChild;
			if (TreeView_GetItem(hTree, &item)) {
				MODULEREC *rec = (MODULEREC*)item.lParam;
				if (rec)
					TreeView_SetCheckState(hTree, hChild, rec->active ? TRUE : FALSE);
			}
			hChild = TreeView_GetNextSibling(hTree, hChild);
		}
		hCat = TreeView_GetNextSibling(hTree, hCat);
	}
	inCheckCascade = false;
}

void orbiter::ModuleTab::ActivateFromList ()
{
	const char *path = "Modules\\Plugin";

	HWND hTree = GetDlgItem (hTab, IDC_MOD_TREE);
	TVITEM catitem, subitem;
	catitem.mask = TVIF_PARAM;
	catitem.hItem = TreeView_GetRoot (hTree);
	subitem.mask = TVIF_PARAM;

	while (TreeView_GetItem (hTree, &catitem)) {
		subitem.hItem = TreeView_GetChild (hTree, catitem.hItem);
		while (TreeView_GetItem (hTree, &subitem)) {
			MODULEREC *rec = (MODULEREC*)subitem.lParam;
			bool checked = (TreeView_GetCheckState (hTree, subitem.hItem) != 0);
			if (checked != rec->active) {
				if (!rec->locked) {
					rec->active = checked;
					if (checked) {
						pCfg->AddActiveModule(rec->name);
						pLp->App()->LoadModule(path, rec->name);
					}
					else {
						pCfg->DelActiveModule(rec->name);
						pLp->App()->UnloadModule(rec->name);
					}
				}
				else {
					TreeView_SetCheckState(hTree, subitem.hItem, rec->active ? TRUE : FALSE);
					MessageBox(NULL, "This module has been requested on the command line and cannot be deactivated interactively.", "Orbiter: Plugin Modules", MB_ICONWARNING | MB_OK);
				}
			}
			subitem.hItem = TreeView_GetNextSibling (hTree, subitem.hItem);
		}
		catitem.hItem = TreeView_GetNextSibling (hTree, catitem.hItem);
	}
}

void orbiter::ModuleTab::DeactivateAll ()
{
	HWND hTree = GetDlgItem (hTab, IDC_MOD_TREE);
	TVITEM catitem, subitem;
	catitem.mask = NULL;
	catitem.hItem = TreeView_GetRoot (hTree);
	subitem.mask = NULL;

	while (TreeView_GetItem (hTree, &catitem)) {
		subitem.hItem = TreeView_GetChild (hTree, catitem.hItem);
		while (TreeView_GetItem (hTree, &subitem)) {
			TreeView_SetCheckState (hTree, subitem.hItem, FALSE);
			subitem.hItem = TreeView_GetNextSibling (hTree, subitem.hItem);
		}
		catitem.hItem = TreeView_GetNextSibling (hTree, catitem.hItem);
	}
	ActivateFromList ();
}

//-----------------------------------------------------------------------------

BOOL orbiter::ModuleTab::OnNotify(HWND hDlg, int idCtrl, LPNMHDR pnmh)
{
	if (idCtrl == IDC_MOD_TREE) {
		NM_TREEVIEW* pnmtv = (NM_TREEVIEW FAR*)pnmh;
		switch (pnmtv->hdr.code) {
		case TVN_SELCHANGED: {
			TVITEM item = pnmtv->itemNew;
			MODULEREC* rec = (MODULEREC*)item.lParam;
			if (rec && rec->info)
				SetWindowText(GetDlgItem(hDlg, IDC_MOD_INFO), rec->info);
			else
				SetWindowText(GetDlgItem(hDlg, IDC_MOD_INFO), "");
		} return TRUE;
		case NM_CLICK: {
			// A click on a tree check box is processed deferred (WM_USER+1),
			// after the control has finished updating the check state. A
			// category check box is then propagated to all its children, and
			// every change synchronises the active module list with the
			// configuration (this was missing entirely: selections were never
			// written to ACTIVE_MODULES and were lost across restarts).
			HWND hTree = GetDlgItem(hDlg, IDC_MOD_TREE);
			LPNMITEMACTIVATE pia = (LPNMITEMACTIVATE)pnmh;
			TVHITTESTINFO ht;
			ht.pt = pia->ptAction;
			TreeView_HitTest(hTree, &ht);
			if (ht.hItem && (ht.flags & TVHT_ONITEMSTATEICON)) {
				pendCheckItem = ht.hItem;
				PostMessage(hDlg, WM_USER + 1, 0, 0);
			}
		} return TRUE;
		case TVN_ITEMCHANGEDA:
		case TVN_ITEMCHANGEDW: {
			// Check box state change. This is the notification the tree control
			// actually sends when the user clicks a check box (NM_CLICK is not
			// sent for the state icon). A category (parent) check box is
			// propagated to all its children; every change synchronises the
			// active module list with the configuration (ACTIVE_MODULES).
			NMTVITEMCHANGE *pnm = (NMTVITEMCHANGE*)pnmh;
			
			// Only react to genuine user clicks. The tree control also resets
			// the check states during its initialisation, which sends
			// TVN_ITEMCHANGED as well; treating those resets as "user
			// deactivated the module" wiped the saved active module list. A
			// real click means the left mouse button is held down at this point.
			if ((pnm->uChanged & TVIF_STATE) && !inCheckCascade &&
			    (GetKeyState(VK_LBUTTON) & 0x8000)) {
				HWND hTree = GetDlgItem(hDlg, IDC_MOD_TREE);
				HTREEITEM child = TreeView_GetChild(hTree, pnm->hItem);
				inCheckCascade = true;
				if (child) {
					// category node: propagate the new state to all children
					BOOL state = TreeView_GetCheckState(hTree, pnm->hItem) ? TRUE : FALSE;
					while (child) {
						TreeView_SetCheckState(hTree, child, state);
						child = TreeView_GetNextSibling(hTree, child);
					}
				}
				inCheckCascade = false;
				// update Config::m_activeModules and load/unload the modules
				ActivateFromList();
			}
		} return TRUE;
		case NM_CUSTOMDRAW:
			// this is a terrible hack to set the initial activation ticks,
			// because for an unknown reason, setting the check state of the
			// tree items during creation gets undone halfway through the
			// initialisation process
			if (counter >= 0 && counter < 4) {
				if (counter == 2) PostMessage(hDlg, WM_USER, 0, 0);
				counter++;
			}
			else if (counter == 4) {
				// Do NOT call ActivateFromList() here. At this stage the tree
				// check states may not have been applied yet (see the known
				// initialisation issue above), and ActivateFromList would then
				// interpret the missing ticks as "user deactivated everything"
				// and wipe the saved active module list (ACTIVE_MODULES). The
				// module activation is loaded from the configuration when the
				// simulator starts, so no synchronisation is needed here.
			}
			return 0;
		}
	}
	return FALSE;
}

//-----------------------------------------------------------------------------

BOOL orbiter::ModuleTab::OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const int MAXSEL = 100;
	int i;
	NM_TREEVIEW *pnmtv;

	switch (uMsg) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_MOD_DEACTALL:
			DeactivateAll ();
			return TRUE;
		case IDC_MOD_BUTTON1:
			ExpandCollapseAll (true);
			return TRUE;
		case IDC_MOD_BUTTON2:
			ExpandCollapseAll (false);
			return TRUE;
		}
		break;
	case WM_USER + 2: {
		// (Re-)apply the saved module check states (see InitActivation).
		ApplyCheckStates();
		ActivateFromList();   // should be a no-op; keeps the config in sync
	} return TRUE;
	case WM_TIMER:
		if (wParam == 1) {
			// The tree is fully initialised by now: force-apply the check
			// states once more (the control resets them during/after its
			// initial layout, which is why the saved ticks used to vanish).
			KillTimer(hTab, 1);
			ApplyCheckStates();
		}
		return TRUE;
	case WM_USER + 1: {
		// Deferred processing of a tree check-box click (see OnNotify).
		if (pendCheckItem) {
			HWND hTree = GetDlgItem(hTab, IDC_MOD_TREE);
			HTREEITEM child = TreeView_GetChild(hTree, pendCheckItem);
			if (child) {
				// category node: propagate the new state to all child modules
				BOOL state = TreeView_GetCheckState(hTree, pendCheckItem) ? TRUE : FALSE;
				while (child) {
					TreeView_SetCheckState(hTree, child, state);
					child = TreeView_GetNextSibling(hTree, child);
				}
			}
			pendCheckItem = NULL;
			// update Config::m_activeModules and load/unload the modules
			ActivateFromList();
		}
		return TRUE;
	}
	case WM_USER:
		InitActivation();

		// hack: hide horizontal scroll bar
		LONG style = GetWindowLongPtr (GetDlgItem (hTab, IDC_MOD_TREE), GWL_STYLE);
		SetWindowLongPtr (GetDlgItem (hTab, IDC_MOD_TREE), GWL_STYLE, style & ~WS_HSCROLL);
		SetWindowPos (GetDlgItem (hTab, IDC_MOD_TREE), NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);

		return 0;
	}
	return NULL;
}
//-----------------------------------------------------------------------------
// Re-apply the active language to the module category names.

void orbiter::ModuleTab::RefreshLocalization()
{
	HWND hTree = GetDlgItem (hTab, IDC_MOD_TREE);
	if (!hTree) return;

	HTREEITEM item = (HTREEITEM)SendMessageW(hTree, TVM_GETNEXTITEM, TVGN_ROOT, 0);
	while (item) {
		wchar_t wbuf[256];
		TVITEMW tvi;
		memset(&tvi, 0, sizeof(tvi));
		tvi.mask = TVIF_TEXT | TVIF_HANDLE;
		tvi.hItem = item;
		tvi.pszText = wbuf;
		tvi.cchTextMax = 256;
		if (SendMessageW(hTree, TVM_GETITEMW, 0, (LPARAM)&tvi)) {
			std::string cur = orbiter::WideToUtf8String(wbuf);
			std::string next = orbiter::RetranslateCurrent(cur.c_str());
			std::wstring wnext = orbiter::Utf8ToWideString(next.c_str());
			TVITEMW set;
			memset(&set, 0, sizeof(set));
			set.mask = TVIF_TEXT;
			set.hItem = item;
			set.pszText = const_cast<LPWSTR>(wnext.c_str());
			SendMessageW(hTree, TVM_SETITEMW, 0, (LPARAM)&set);
		}
		item = (HTREEITEM)SendMessageW(hTree, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)item);
	}
}