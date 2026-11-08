#include "Localization.h"
// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// ======================================================================
// Object info window
// ======================================================================
#include "DlgInfo.h"
#include "Celbody.h"
#include "Psys.h"
#include "Astro.h"
#include "Element.h"
#include "imgui.h"
#include "imgui_extras.h"
#include "IconsFontAwesome6.h"

extern PlanetarySystem *g_psys;

DlgInfo::DlgInfo() : ImGuiDialog(ICON_FA_CIRCLE_INFO " Orbiter: Object info", {753,423}) {
	SetHelp("html/orbiter.chm", "/objinfo.htm");
	m_SelectedTarget = g_psys->GetStar(0)->Name();
}

void DlgInfo::AddCbodyNode(const CelestialBody *cbody) {
    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    const bool is_selected = m_SelectedTarget == cbody->Name();
    if (is_selected)
        node_flags |= ImGuiTreeNodeFlags_Selected;
    if(cbody->nSecondary()) {
        bool node_open = ImGui::TreeNodeEx(orbiter::Tr(cbody->Name()), node_flags);
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            m_SelectedTarget = cbody->Name();
        if(node_open) {
            for (int i = 0; i < cbody->nSecondary(); i++) {
                AddCbodyNode (cbody->Secondary(i));
            }
            ImGui::TreePop();
        }
    } else {
        ImGui::TreeNodeEx(orbiter::Tr(cbody->Name()), node_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            m_SelectedTarget = cbody->Name();
    }
}

void DlgInfo::DrawTree() {
    for (int i = 0; i < g_psys->nStar(); i++)
        AddCbodyNode (g_psys->GetStar(i));

    if (ImGui::TreeNode(orbiter::Tr("Spaceports"))) {
        for (int i = 0; i < g_psys->nGrav(); i++) {
            Body *obj = g_psys->GetGravObj (i);
            if (obj->Type() != OBJTP_PLANET) continue;
            Planet *planet = (Planet*)obj;
            if (g_psys->nBase(planet) > 0) {
				if(ImGui::TreeNode(orbiter::Tr(planet->Name()))) {
					for (int j = 0; j < g_psys->nBase(planet); j++) {
						const char *name = g_psys->GetBase (planet,j)->Name();
						const bool is_selected = m_SelectedTarget == name;
						ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
						if(is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;
						ImGui::TreeNodeEx(orbiter::Tr(name), node_flags);
						if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
							m_SelectedTarget = name;
					}
					ImGui::TreePop();
				}
            }
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode(orbiter::Tr("Vessels"))) {
        for (int i = 0; i < g_psys->nVessel(); i++) {
            const char *name = g_psys->GetVessel(i)->Name();
            const bool is_selected = m_SelectedTarget == name;
            ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if(is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;
            ImGui::TreeNodeEx(orbiter::Tr(name), node_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                m_SelectedTarget = name;
        }
        ImGui::TreePop();
    }
}

void DlgInfo::DrawInfo() {
    Vessel *vessel = g_psys->GetVessel(m_SelectedTarget.c_str(), true);
    if(vessel) {
        DrawInfoVessel(vessel);
        return;
    }
    CelestialBody *cb = g_psys->GetGravObj(m_SelectedTarget.c_str(), true);
    if(cb) {
        DrawInfoCelestialBody(cb);
        return;
    }
    Base *base = g_psys->GetBase(m_SelectedTarget.c_str(), true);
    if(base) {
        DrawInfoBase(base);
        return;
    }
    ImGui::Text(orbiter::Tr("Select an object on the left panel"));
}

void DlgInfo::DrawInfoVessel(Vessel *vessel) {
    ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
    if(ImGui::BeginAnimatedCollapsingHeader(orbiter::Tr("Designation"), ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("table Designation", 2, flags))
        {
            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Name"));
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(orbiter::Tr(vessel->Name()));
            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Class"));
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(orbiter::Tr(vessel->ClassName()));
            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Transponder Frequency"));
                ImGui::TableSetColumnIndex(1);
                float f;
                if (vessel->GetXpdrFreq (f))
                    ImGui::Text("%0.2fMHz", f);
                else
                    ImGui::TextUnformatted(orbiter::Tr("N/A"));
            ImGui::EndTable();
        }
        ImGui::EndAnimatedCollapsingHeader();
    }
    if(ImGui::BeginAnimatedCollapsingHeader(orbiter::Tr("Physical Parameters"), ImGuiTreeNodeFlags_DefaultOpen)) {
        // Total mass, dry mass, propellant mass, mean radius, P. moment of inertias
        if (ImGui::BeginTable("table Physical Parameters", 2, flags))
        {
            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Total mass"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%g kg", vessel->Mass());
            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Dry mass"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%g kg", vessel->EmptyMass());
            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Propellant mass"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%g kg", vessel->FuelMass());
            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Mean radius"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s m", DistStr (vessel->Size())+1);
            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("P. moments of inertia"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text(orbiter::Tr("(%4g, %4g, %4g) kg.m²"), vessel->PMI().x, vessel->PMI().y, vessel->PMI().z);

            ImGui::EndTable();
        }
        ImGui::EndAnimatedCollapsingHeader();
    }
    if(ImGui::BeginAnimatedCollapsingHeader(orbiter::Tr("Thrusters Group Ratings (vacuum)"), ImGuiTreeNodeFlags_DefaultOpen)) {
        //Main, retro, hover   
        const THGROUP_TYPE thgrp[3] = {THGROUP_MAIN, THGROUP_RETRO, THGROUP_HOVER};
        const char *thrtype[] = {"Main", "Retro", "Hover"};
        if (ImGui::BeginTable("table Thrusters Group Ratings", 2, flags))
        {
            for (int i = 0; i < 3; i++) {
                ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(orbiter::Tr(thrtype[i]));
                    ImGui::TableSetColumnIndex(1);
                    double th = vessel->GetThrusterGroupMaxth (thgrp[i]);
                    if (th > 0.0) {
                        ImGui::Text("%sN", FloatStr (th)+1);
                    } else {
                        ImGui::Text(orbiter::Tr("N/A"));
                    }
            }
            ImGui::EndTable();
        }
        ImGui::EndAnimatedCollapsingHeader();
    }

    const Body *ref = vessel->ElRef();
    const Elements *el = vessel->Els();
    if(el && ref && ImGui::BeginAnimatedCollapsingHeader(orbiter::Tr("Osculating Elements (Ecliptic Frame)"), ImGuiTreeNodeFlags_DefaultOpen)) {
        //Reference, semi major axis, excentricity, inclination, longitude of AN, longitude of periapsis, mean longitude
        if (ImGui::BeginTable("table Osculating Elements", 2, flags))
        {
            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Reference"));
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(orbiter::Tr(ref->Name()));

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Semi-major Axis (a)"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s m", SciStr (el->a, 5));

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Excentricity (e)"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%g", el->e);

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Inclination (igg)"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%0.2f°", el->i*DEG);

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Longitude of Ascending Node"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%0.2f°", el->theta*DEG);

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Longitude of Periapsis"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%0.2f°", el->omegab*DEG);

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Mean longitude"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%0.2f°", el->MeanLng()*DEG);
            ImGui::EndTable();
        }
        ImGui::EndAnimatedCollapsingHeader();
    }

    const SurfParam *sp = vessel->GetSurfParam();
    if(sp && ImGui::BeginAnimatedCollapsingHeader(orbiter::Tr("Surface-relative Parameters"), ImGuiTreeNodeFlags_DefaultOpen)) {
        //reference, position, altitude, ground speed, vertical speed, heading, pitch, bank
        if (ImGui::BeginTable("table Surface-relative Parameters", 2, flags))
        {
            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Reference"));
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(orbiter::Tr(sp->ref->Name()));

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Position"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%07.3f°%c %06.3f°%c", fabs(sp->lng)*DEG, sp->lng >= 0.0 ? 'E':'W', fabs(sp->lat)*DEG, sp->lat >= 0.0 ? 'N':'S');

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Altitude"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%sm", DistStr (sp->alt)+1);

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Ground Speed"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%sm/s", FloatStr (sp->groundspd)+1);

            Vector V (mul (sp->L2H, tmul (vessel->ProxyBody()->GRot(), sp->groundvel_glob)));
            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Vertical Speed"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%sm/s", FloatStr (V.y)+1);

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Heading"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%0.0f°", sp->dir*DEG);

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Pitch"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%0.0f°", sp->pitch*DEG);

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Bank"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%0.0f° %s", fabs(sp->bank)*DEG, orbiter::Tr(sp->bank >= 0.0 ? "left" : "right"));

            ImGui::EndTable();
        }
        ImGui::EndAnimatedCollapsingHeader();
    }

   	if (vessel->isInAtmosphere()) {
        if(ImGui::BeginAnimatedCollapsingHeader(orbiter::Tr("Atmospheric Parameters"), ImGuiTreeNodeFlags_DefaultOpen)) {
            // Temperature, density, pressure
            if (ImGui::BeginTable("table Atmospheric Parameters", 2, flags))
            {
                double T, rho, p;
                vessel->AtmTemperature (T);
                vessel->AtmPressureAndDensity (p, rho);
                ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(orbiter::Tr("Temperature"));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%0.2f K", T);

                ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(orbiter::Tr("Density"));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%0.4g kg/m^3", rho);

                ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(orbiter::Tr("Pressure"));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%sPa", FloatStr(p)+1);

                ImGui::EndTable();
            }
            ImGui::EndAnimatedCollapsingHeader();
        }
        if(ImGui::BeginAnimatedCollapsingHeader(orbiter::Tr("Aerodynamic Parameters"), ImGuiTreeNodeFlags_DefaultOpen)) {
            // Dynamic pressure, true airspeed, mach number, lift, drag, weight, lift/drag ratio, angle of attack
            if (ImGui::BeginTable("table Aerodynamic Parameters", 2, flags))
            {
                double dynp, M, L, D;
                vessel->DynPressure (dynp);
                L = vessel->GetLift();
                D = vessel->GetDrag();
                vessel->MachNumber (M);

                ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(orbiter::Tr("Dynamic Pressure"));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%sPa", FloatStr(dynp)+1);

                ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(orbiter::Tr("True Airspeed"));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text( "%sm/s", FloatStr(sp->airspd)+1);

                ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(orbiter::Tr("Mach Number"));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%g", M);

                ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(orbiter::Tr("Lift"));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%sN", FloatStr(L)+(L >= 0 ? 1:0));

                ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(orbiter::Tr("Drag"));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%sN", FloatStr(D)+1);

                ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(orbiter::Tr("Weight"));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%sN", FloatStr(vessel->GetWeight())+1);

                ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(orbiter::Tr("Lift/Drag Ratio"));
                    ImGui::TableSetColumnIndex(1);
                    if (D)
                        ImGui::Text("%g", L/D);
                    else
                        ImGui::TextUnformatted(orbiter::Tr("N/A"));

                ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(orbiter::Tr("Angle of Attack"));
                    ImGui::TableSetColumnIndex(1);
                    if(sp)
                        ImGui::Text("%+0.1f°", -atan2 (sp->groundvel_ship.y, sp->groundvel_ship.z)*DEG);
                    else
                        ImGui::TextUnformatted(orbiter::Tr("N/A"));

                ImGui::EndTable();
            }
            ImGui::EndAnimatedCollapsingHeader();
        }
    }

    if (vessel->nDock()) {
        if(ImGui::BeginAnimatedCollapsingHeader(orbiter::Tr("Docking Ports"), ImGuiTreeNodeFlags_DefaultOpen)) {
            // port 1,2,3...
            if (ImGui::BeginTable("table Docking Ports", 2, flags))
            {
                for(int i = 0;i<vessel->nDock();i++) {
                    char buf[128];
                    if (vessel->GetDockParams(i)->ids)
    					sprintf (buf, "IDS %06.2f ", vessel->GetDockParams(i)->ids->GetFreq());
                    else
                        buf[0]='\0';
                    Vessel *mate = vessel->DockMate (i);
                    if (mate) sprintf (buf+strlen(buf), "[%s %s]", orbiter::Tr("Docked to"), orbiter::Tr(mate->Name()));
                    else sprintf (buf+strlen(buf), "[%s]", orbiter::Tr("free"));

                    ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text(orbiter::Tr("Port %d"), i+1);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextUnformatted(buf);
                }

                ImGui::EndTable();
            }
            ImGui::EndAnimatedCollapsingHeader();
        }
    }

    if(ImGui::BeginAnimatedCollapsingHeader(orbiter::Tr("State Propagation"), ImGuiTreeNodeFlags_DefaultOpen)) {
        // update mode, state propagator, subsamples, gravity sources
        if (ImGui::BeginTable("table State Propagation", 2, flags))
        {
            if (vessel->GetStatus() == FLIGHTSTATUS_LANDED) {
                ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(orbiter::Tr("Update mode"));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(orbiter::Tr("IDLE (landed)"));
            } else if (vessel->isAttached()) {
                ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(orbiter::Tr("Update mode"));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(orbiter::Tr("PASSIVE (attached)"));
            } else {
                ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(orbiter::Tr("Update mode"));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(orbiter::Tr("ACTIVE (dynamic)"));

                ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(orbiter::Tr("State Propagator"));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(orbiter::Tr(vessel->isOrbitStabilised() ? "stabilised" : vessel->CurPropagatorStr()));

                ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(orbiter::Tr("Subsamples"));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%d", vessel->CurPropagatorSubsamples());

                char cbuf[256];
                cbuf[0] = '\0';
                const GFieldData &gfd = vessel->GetGFieldData();
                for (int i = 0; i < gfd.ngrav; i++) {
                    if (i) strcat (cbuf, ", ");
                    strcat (cbuf, orbiter::Tr(g_psys->GetGravObj(gfd.gravidx[i])->Name()));
                }
                ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(orbiter::Tr("Gravity Sources"));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(cbuf);

            }
            ImGui::EndTable();
        }
        ImGui::EndAnimatedCollapsingHeader();
    }
}


void DlgInfo::DrawInfoCelestialBody(CelestialBody *cbody) {
    const Elements *el = nullptr;
    const Planet *planet = nullptr;
    const CELBODY *cb = cbody->GetModuleInterface();

    switch (cbody->Type()) {
    case OBJTP_PLANET:
        planet = (Planet*)cbody;
        el = planet->Els();
        break;
    }

    ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
    if(ImGui::BeginAnimatedCollapsingHeader(orbiter::Tr("Designation"), ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("table celbody designation", 2, flags))
        {
            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Name"));
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(orbiter::Tr(cbody->Name()));

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Primary"));
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(cbody->ElRef() ? orbiter::Tr(cbody->ElRef()->Name()) : orbiter::Tr("N/A"));

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Solar System"));
                ImGui::TableSetColumnIndex(1);
                const char *psys_name = g_psys->Name().c_str();
                ImGui::TextUnformatted(psys_name ? orbiter::Tr(psys_name) : orbiter::Tr("N/A"));

            ImGui::EndTable();
        }
        ImGui::EndAnimatedCollapsingHeader();
    }
    if(ImGui::BeginAnimatedCollapsingHeader(orbiter::Tr("Physical Parameters"), ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("table celbody designation", 2, flags))
        {
            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Mass"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s kg", SciStr (cbody->Mass(), 4));

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Mean Radius"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s m", SciStr (cbody->Size(), 4));

            int nj;
            char cbuf[128];
            if ((nj = cbody->nJcoeff())) {
                cbuf[0] = '\0';
                for (int i = 0; i < nj; i++) {
                    sprintf (cbuf + strlen(cbuf), "J%d=%s", i+2, SciStr (cbody->Jcoeff (i),3));
                    if (i < nj-1) strcat (cbuf, ", ");
                }
            } else strcpy (cbuf, "N/A");
            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Gravitational Moments"));
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(cbuf);

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Siderial day"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s s", SciStr (cbody->RotT(), 4));

            if (el) sprintf (cbuf, "%s s", SciStr (el->OrbitT(), 4));
            else    strcpy (cbuf, "N/A");

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Orbit Period"));
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(cbuf);

            if (planet) sprintf (cbuf, "%0.2f°", planet->Obliquity()*DEG);
            else strcpy (cbuf, "N/A");

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Obliquity of Ecliptic"));
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(cbuf);

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Atmosphere"));
                ImGui::TableSetColumnIndex(1);
                if(planet && planet->AtmParams()) {
                    ImGui::TextUnformatted(orbiter::Tr("Yes"));
                } else {
                    ImGui::TextUnformatted(orbiter::Tr("No"));
                }


            ImGui::EndTable();
        }
        ImGui::EndAnimatedCollapsingHeader();
    }
    if(planet && planet->AtmParams() && ImGui::BeginAnimatedCollapsingHeader(orbiter::Tr("Atmosphere"), ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("table celbody Atmosphere", 2, flags))
        {
            char cbuf[128];
            const ATMCONST *ap = planet->AtmParams();
            strcpy (cbuf, "Generic");
            if (cb && cb->Version() >= 2) {
                CELBODY2 *cb2 = (CELBODY2*)cb;
                ATMOSPHERE *atm = cb2->GetAtmosphere();
                if (atm) strcpy (cbuf, atm->clbkName());
            }

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Atmosphere Model"));
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(cbuf);

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Surface Pressure"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%sPa", FloatStr (ap->p0)+1);

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Surface density"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%skg/m^3", SciStr (ap->rho0));

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Specific Gas Constant"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%0.2fJ/(K kg)", ap->R);

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Specific Heat Ratio"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%0.2f", ap->gamma);

            ImGui::EndTable();
        }
        ImGui::EndAnimatedCollapsingHeader();
    }

    if(el && ImGui::BeginAnimatedCollapsingHeader(orbiter::Tr("Osculating Elements (Ecliptic Frame)"), ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("table celbody Osculating elements (ecliptic frame)", 2, flags))
        {
            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Semi-major Axis (a)"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s m", SciStr (el->a, 5));

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Eccentricity (e)"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%0.5g", el->e);

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Inclination (i)"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%0.2f°", el->i*DEG);

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Longitude of Ascending Node"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%0.2f°", el->theta*DEG);

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Longitude of Periapsis"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%0.2f°", el->omegab*DEG);

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Mean Longitude"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%0.2f°", el->MeanLng()*DEG);

            ImGui::EndTable();
        }
        ImGui::EndAnimatedCollapsingHeader();
    }
    if(strcmp(cbody->Name(), "Earth") && ImGui::BeginAnimatedCollapsingHeader(orbiter::Tr("Geocentric Celestial Position"), ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("table celbody Geocentric Celestial Position", 2, flags))
        {
            char cbuf[128];
            Planet *earth = g_psys->GetPlanet ("Earth");
			Vector p (cbody->GPos() - earth->GPos());
			double r   = p.length();
			double lng = atan2 (p.z, p.x);
			double lat = p.y/r;
			double ra, dc, rah, ram, ras, dcd, dcm, dcs;
			static double ob = earth->Obliquity();
			static double cosob = cos(ob), sinob = sin(ob);
			Ecl2Equ (cosob, sinob, lng, lat, ra, dc);
			ram = modf (posangle(ra) * 24.0/Pi2, &rah) * 60.0;
			ras = modf (ram, &ram) * 60.0;
			dcm = fabs (modf (dc*DEG, &dcd)) * 60.0;
			dcs = modf (dcm, &dcm) * 60.0;

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Right Ascension (RA)"));
                ImGui::TableSetColumnIndex(1);
    			sprintf (cbuf, "%02.0fh %02.0fm %02.2fs", rah, ram, ras);
                ImGui::TextUnformatted(cbuf);

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Declination (Dec)"));
                ImGui::TableSetColumnIndex(1);
    			sprintf (cbuf, "%+02.0f° %02.0f' %02.2f''", dcd, dcm, dcs);
                ImGui::TextUnformatted(cbuf);

            ImGui::EndTable();
        }
        ImGui::EndAnimatedCollapsingHeader();
    }

    if(el && ImGui::BeginAnimatedCollapsingHeader(orbiter::Tr("Ecliptic position from primary"), ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("table celbody Ecliptic position from primary", 2, flags))
        {
			Vector p (cbody->GPos() - cbody->ElRef()->GPos());
			double r   = p.length();
			double lng = atan2 (p.z, p.x);
			double lat = p.y/r;

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Longitude"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%0.3f°", DEG*posangle(lng));

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Latitude"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%0.3f°", DEG*lat);

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Radial Distance"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s m", SciStr (r));

            ImGui::EndTable();
        }
        ImGui::EndAnimatedCollapsingHeader();
    }
    if(ImGui::BeginAnimatedCollapsingHeader(orbiter::Tr("State Propagation"), ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("table celbody State propagation", 2, flags))
        {
            if (cbody->canDynamicPosVel()) {
                ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(orbiter::Tr("Mode"));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(orbiter::Tr("Numerical"));

                const GFieldData &gfd = cbody->GetGFieldData();
                if (gfd.ngrav) {
                    char cbuf[128];
                    cbuf[0] = '\0';
                    for (int i = 0; i < gfd.ngrav; i++) {
                        strcat (cbuf, orbiter::Tr(g_psys->GetGravObj(gfd.gravidx[i])->Name()));
                        if (i < gfd.ngrav-1) strcat (cbuf, ", ");
                    }
                    ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextUnformatted(orbiter::Tr("Gravity Sources"));
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextUnformatted(cbuf);

                }
            } else {
                ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(orbiter::Tr("Mode"));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text(orbiter::Tr("Analytic (%s)"), cb ? orbiter::Tr("from module") : orbiter::Tr("2-body"));
            }

            ImGui::EndTable();
        }
        ImGui::EndAnimatedCollapsingHeader();
    }
}
void DlgInfo::DrawInfoBase(Base *base) {
    ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
    if(ImGui::BeginAnimatedCollapsingHeader(orbiter::Tr("Designation"), ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("table base designation", 2, flags))
        {
            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Name"));
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(orbiter::Tr(base->Name()));

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Located on"));
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(orbiter::Tr(base->RefPlanet()->Name()));

           	double lng, lat;
        	base->EquPos (lng, lat);

            ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(orbiter::Tr("Position"));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%07.3f°%c  %06.3f°%c",fabs(lng)*DEG, lng >= 0.0 ? 'E':'W',fabs(lat)*DEG, lat >= 0.0 ? 'N':'S');

            ImGui::EndTable();
        }
        ImGui::EndAnimatedCollapsingHeader();
    }

    // Landing pads
    //     pad 1
    //     pad xxx
    if(base->nPad()) {
        if(ImGui::BeginAnimatedCollapsingHeader(orbiter::Tr("Landing Pads"), ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginTable("table Landing Pads", 2, flags))
            {
                const char *c, *statusstr[3] = {"free", "", "reserved"};
                for(int i=0;i<base->nPad();i++) {
                    char cbuf[256];
                    cbuf[0] = '\0';
                    int status = base->PadStatus(i)->status;
                    if (status == 1) c = base->PadStatus(i)->vessel->Name();
                    else c = statusstr[status];
                    if (base->PadStatus(i)->nav)
                        sprintf (cbuf, "ILS %06.2f ", base->PadStatus(i)->nav->GetFreq());
    				sprintf (cbuf+strlen (cbuf), "[%s]", orbiter::Tr(c));

                    ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text(orbiter::Tr("Pad %d"), i+1);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextUnformatted(cbuf);
                }
                ImGui::EndTable();
            }
            ImGui::EndAnimatedCollapsingHeader();
        }
    }

    // Runways
    if(base->nRwy()) {
        if(ImGui::BeginAnimatedCollapsingHeader(orbiter::Tr("Runways"), ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginTable("table Runways", 2, flags))
            {
                for(int i=0;i<base->nRwy();i++) {
                    char cbuf[256];
                    const RwySpec *rwy = base->RwyStatus (i);
                    int dir = (int)(posangle(rwy->appr1)*DEG*0.1+0.5);
                    char ils1[20], ils2[20];
                    if (rwy->ils1) sprintf (ils1, "%06.2f", rwy->ils1->GetFreq());
                    else strcpy (ils1, "--");
                    if (rwy->ils2) sprintf (ils2, "%06.2f", rwy->ils2->GetFreq());
                    else strcpy (ils2, "--");
                    sprintf (cbuf, "ILS %s/%s, length %0.0fm", ils1, ils2, rwy->length);

                    ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text(orbiter::Tr("Runway %02d/%02d"), dir, (dir+18)%36);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextUnformatted(cbuf);
                }
                ImGui::EndTable();
            }
            ImGui::EndAnimatedCollapsingHeader();
        }
    }

    // VOR
    if(base->nVOR()) {
        if(ImGui::BeginAnimatedCollapsingHeader(orbiter::Tr("VOR Transmitters"), ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginTable("table VOR", 2, flags))
            {
                for(int i=0;i<base->nVOR();i++) {
                    const Nav *nav = base->VOR(i);
                    if(nav->Type() == TRANSMITTER_VOR) {
                        ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextUnformatted(nav->GetId());
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text(orbiter::Tr("%06.2f, range %sm"), nav->GetFreq(), DistStr (nav->GetRange()));
                    }
                }
                ImGui::EndTable();
            }
            ImGui::EndAnimatedCollapsingHeader();
        }
    }
}

void DlgInfo::OnDraw() {
        ImGuiWindowFlags window_flags = ImGuiChildFlags_ResizeX;
        ImGui::BeginChild("ChildL", ImVec2(250, 0), true, window_flags);
        {
            ImVec2 button_sz(ImVec2(ImGui::GetContentRegionAvail().x/2, 20));

            ImGui::Button(orbiter::Tr("Focus Vessel"));
            ImGui::SameLine();
            ImGui::Button(orbiter::Tr("Camera Target"));
            DrawTree();
        }
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("ChildR", ImVec2(0, 0), 0);
            ImVec2 button_sz(ImVec2(ImGui::GetContentRegionAvail().x, 20));
            ImGui::Text(orbiter::Tr("Object: %s"), orbiter::Tr(m_SelectedTarget.c_str()));

            DrawInfo();

        ImGui::EndChild();
}

void DlgInfo::SetBody(Body *body) {
	m_SelectedTarget = body->Name();
}
