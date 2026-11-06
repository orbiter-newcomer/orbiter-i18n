// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ======================================================================
// Menu bar configuration dialog
// ======================================================================

#include "DlgMenuCfg.h"
#include "MenuInfoBar.h"
#include "Pane.h"
#include "imgui.h"
#include "imgui_extras.h"
#include "Orbiter.h"
#include "Localization.h"
#include "IconsFontAwesome6.h"

extern Orbiter *g_pOrbiter;
extern Pane *g_pane;

DlgMenuCfg::DlgMenuCfg(): ImGuiDialog(ICON_FA_SLIDERS " Orbiter: Configure menu bars")
{
	SetHelp("html/orbiter.chm", "/menucfg.htm");
}

void DlgMenuCfg::OnDraw()
{
	const char *modes[] = {"Show", "Hide", "Auto-hide"};

	CFG_UIPRM &prm = g_pOrbiter->Cfg()->CfgUIPrm;
	MenuInfoBar *mib = g_pane->MIBar();

	if(ImGui::BeginTabBar("##MenuCfgTab")) {
		if(ImGui::BeginTabItem(orbiter::Tr("Appearance"))) {
			ImGui::BeginChild("##Child");
			ImGui::SeparatorText(orbiter::Tr("Menu bar"));
			ImGui::PushID(1);
			ImGui::RadioButton(orbiter::Tr("Show"), &prm.MenuMode, 0);
			ImGui::SameLine();
			ImGui::RadioButton(orbiter::Tr("Hide"), &prm.MenuMode, 1);
			ImGui::SameLine();
			ImGui::RadioButton(orbiter::Tr("Auto-hide"), &prm.MenuMode, 2);

			ImGui::Checkbox(orbiter::Tr("Always show labels"), &prm.bMenuLabelAlways);
			ImGui::SliderInt(orbiter::Tr("Opacity"), &prm.MenuOpacity, 0, 10);
			ImGui::SliderInt(orbiter::Tr("Animation speed"), &prm.MenuScrollspeed, 1, 20);
			ImGui::SliderInt(orbiter::Tr("Button size"), &prm.MenuButtonSize, 10, 64);
			ImGui::SliderInt(orbiter::Tr("Hovered size"), &prm.MenuButtonHoverSize, 10, 64);
			ImGui::SliderInt(orbiter::Tr("Spacing"), &prm.MenuButtonSpacing, 1, 32);
			ImGui::PopID();

			ImGui::SeparatorText(orbiter::Tr("Info bars"));
			ImGui::PushID(2);
			ImGui::RadioButton(orbiter::Tr("Show"), &prm.InfoMode, 0);
			ImGui::SameLine();
			ImGui::RadioButton(orbiter::Tr("Hide"), &prm.InfoMode, 1);
			ImGui::SameLine();
			ImGui::RadioButton(orbiter::Tr("Auto-hide"), &prm.InfoMode, 2);
			ImGui::Checkbox(orbiter::Tr("Always show warp factor"), &prm.bWarpAlways);
			ImGui::SliderInt(orbiter::Tr("Opacity"), &prm.InfoOpacity, 0, 10);
			ImGui::PopID();

			ImGui::SeparatorText(orbiter::Tr("FPS info bar"));
			const char *fpsmode[]={"None", "On the left", "On the right"};
			ImGui::Combo("##FPSMode", &prm.FPS, fpsmode, IM_ARRAYSIZE(fpsmode));

			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		if(ImGui::BeginTabItem(orbiter::Tr("Menu items"))) {
			ImGui::BeginChild("##Child");
			bool changed = false;
			for(auto &pref: g_pane->MIBar()->GetPreferences()) {
				changed |= ImGui::Checkbox(pref.label.c_str(), &pref.enabled);
			}
			if(changed)
				g_pane->MIBar()->SyncPreferences();
			ImGui::EndChild();
			ImGui::EndTabItem();  
		}
		ImGui::EndTabBar();
	}
}
