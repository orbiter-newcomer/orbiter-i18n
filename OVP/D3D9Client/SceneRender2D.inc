// SceneRender2D.inc - 2D/HUD overlay part of the main scene (project extension).
// Included by the official Scene.cpp at the end of the file.

// ---------------------------------------------------------------------------
// Scene::RenderMainScene2D
//
// HUD/2D overlay part of the main scene, split out for the dual thread
// experiment: the 3D part (RenderMainScene) can run on the render thread while
// this 2D part runs on the main thread after the 3D frame is complete.
// ---------------------------------------------------------------------------
void Scene::RenderMainScene2D()
{
	D3D9Pad *pSketch = NULL;
	LPDIRECT3DTEXTURE9 pShdMap = smap.pShadowMap;	// debug visualisation source (2D)

	// Render HUD Overlay to backbuffer directly
	// -------------------------------------------------------------------------------------------------------


	gc->PushRenderTarget(gc->GetBackBuffer(), gc->GetDepthStencil(), RENDERPASS_MAINOVERLAY);	// Overlay

	pSketch = GetPooledSketchpad(SKETCHPAD_2D_OVERLAY);

	if (pSketch) {
		gc->MakeRenderProcCall(pSketch, RENDERPROC_HUD_1ST, NULL, NULL);
		// wreck message (relativistic collision debris 2D drawing was removed)
		RenderWreckMessage (pSketch);

		pSketch->EndDrawing(); // SKETCHPAD_2D_OVERLAY
	}

	gc->Render2DOverlay();
	pSketch = GetPooledSketchpad(SKETCHPAD_2D_OVERLAY);
	if (pSketch) {
		gc->MakeRenderProcCall(pSketch, RENDERPROC_HUD_2ND, NULL, NULL);
		pSketch->EndDrawing(); // SKETCHPAD_2D_OVERLAY
	}


	// Enable Freeze mode after the main scene is complete
	//
	if (bFreezeEnable) bFreeze = true;

	
	Perf_Mark(PERF_STAGE_DEBUGOVL);

	// -------------------------------------------------------------------------------------------------------
	// EnvMap Debugger  TODO: Should be allowed to visualize other maps as well, not just index 0
	// -------------------------------------------------------------------------------------------------------

	if (DebugControls::IsActive()) {
		
		int sel = DebugControls::GetSelectedEnvMap();

		switch (sel) {
		case 1:		case 2:		case 3:		case 4:
		case 5:
			VisualizeCubeMap(vFocus->GetEnvMap(ENVMAP_MAIN), sel - 1);
			break;
		case 6:
			VisualizeCubeMap(vFocus->GetIrradEnv(), 0);
			break;
		case 7:
			VisualizeCubeMap(pIrradTemp, 0);
			break;
		case 8:
			if (pShdMap) {
				pSketch = GetPooledSketchpad(SKETCHPAD_2D_OVERLAY);
				pSketch->CopyRectNative(pShdMap, NULL, 0, 0);
				pSketch->EndDrawing();
			}
			break;
		case 9:
			if (vFocus->GetIrradianceMap()) {
				pSketch = GetPooledSketchpad(SKETCHPAD_2D_OVERLAY);
				pSketch->CopyRectNative(vFocus->GetIrradianceMap(), NULL, 0, 0);
				pSketch->EndDrawing();
			}
			break;
		case 10:
			if (ptgBuffer[GBUF_BLUR]) {
				pSketch = GetPooledSketchpad(SKETCHPAD_2D_OVERLAY);
				pSketch->CopyRectNative(ptgBuffer[GBUF_BLUR], NULL, 0, 0);
				pSketch->EndDrawing();
			}
			break;
		case 11:
			if (ptgBuffer[GBUF_DEPTH]) {
				if (pVisDepth) {
					if (pVisDepth->IsOK()) {
						pVisDepth->Activate("PSDepth");
						pVisDepth->SetTextureNative("tBack", ptgBuffer[GBUF_DEPTH], IPF_POINT | IPF_CLAMP);
						pVisDepth->SetOutputNative(0, gc->GetBackBuffer());
						pVisDepth->Execute(true);
					}
				}
			}
			break;
		case 12:
			if (ptgBuffer[GBUF_DEPTH]) {
				if (pVisDepth) {
					if (pVisDepth->IsOK()) {
						pVisDepth->Activate("PSNormal");
						pVisDepth->SetTextureNative("tBack", ptgBuffer[GBUF_DEPTH], IPF_POINT | IPF_CLAMP);
						pVisDepth->SetOutputNative(0, gc->GetBackBuffer());
						pVisDepth->Execute(true);
					}
				}
			}
			break;
		case 13:
			if (pLocalResults) {
				pSketch = GetPooledSketchpad(SKETCHPAD_2D_OVERLAY);
				pSketch->SetBlendState(Sketchpad::BlendState::FILTER_POINT);
				pSketch->StretchRectNative(pLocalResults, NULL, ptr(_RECT(0, 0, viewW, 10)));
				pSketch->SetBlendState(Sketchpad::BlendState::FILTER_LINEAR);
				pSketch->EndDrawing();
			}
			break;
		case 14:
			if (Camera.vNear) {
				auto ptE = Camera.vNear->GetEclipse();
				if (ptE) {
					pSketch = GetPooledSketchpad(SKETCHPAD_2D_OVERLAY);
					pSketch->SetBlendState(Sketchpad::BlendState::FILTER_POINT);
					pSketch->StretchRectNative(ptE, NULL, ptr(_RECT(0, 0, viewW, 10)));
					pSketch->SetBlendState(Sketchpad::BlendState::FILTER_LINEAR);
					pSketch->EndDrawing();
				}
			}
			break;
		default:
			break;
		}
	}


	if (AtmoControls::Visualize())
	{
		vPlanet* vP = GetCameraProxyVisual();
		pSketch = GetPooledSketchpad(SKETCHPAD_2D_OVERLAY);
		pSketch->SetBlendState(Sketchpad::COPY);
		int x = 0, y = ViewH();

		LPDIRECT3DTEXTURE9 pTab = vP->GetScatterTable(RAY_LAND);
		D3DSURFACE_DESC desc;
		if (pTab) {
			pTab->GetLevelDesc(0, &desc);
			pSketch->StretchRectNative(pTab, NULL, ptr(_R(0, y - desc.Height, desc.Width, y)));
			y -= (desc.Height + 5);
		}
		pTab = vP->GetScatterTable(MIE_LAND);
		if (pTab) {
			pTab->GetLevelDesc(0, &desc);
			pSketch->StretchRectNative(pTab, NULL, ptr(_R(0, y - desc.Height, desc.Width, y)));
			y -= (desc.Height + 5);
		}
		pTab = vP->GetScatterTable(ATN_LAND);
		if (pTab) {
			pTab->GetLevelDesc(0, &desc);
			pSketch->StretchRectNative(pTab, NULL, ptr(_R(0, y - desc.Height, desc.Width, y)));
			y -= (desc.Height + 5);
		}
		for (int i=0;i<9;i++)
		{
			if (i == RAY_LAND || i == MIE_LAND || i == ATN_LAND) continue;
			pTab = vP->GetScatterTable(i);
			if (!pTab) continue;
			pTab->GetLevelDesc(0, &desc);
			pSketch->CopyRectNative(pTab, NULL, x, y - desc.Height);
			x += desc.Width + 5;
		}
		pSketch->SetBlendState(Sketchpad::ALPHABLEND);
		pSketch->EndDrawing();
	}


	// -------------------------------------------------------------------------------------------------------
	// Draw Debug String on a bottom of the screen
	// -------------------------------------------------------------------------------------------------------

	const char* dbgString = oapiDebugString();
	int len = lstrlen(dbgString);

	if (len>0 || !D3D9DebugQueue.empty()) {

		pSketch = GetPooledSketchpad(SKETCHPAD_DEBUG_TEXT);

		DWORD height = Config->DebugFontSize;

		// Display Orbiter's debug string
		if (len > 0) {
			DWORD width = pSketch->GetTextWidth(dbgString, len);
			pSketch->Rectangle(-1, viewH - height - 1, width + 4, viewH);
			pSketch->Text(2, viewH - 2, dbgString, len);
		}

		DWORD pos = viewH;

		// Display additional debug string queue
		//
		while (!D3D9DebugQueue.empty()) {
			pos -= (height * 3) / 2;
			std::string str = D3D9DebugQueue.front();
			len = lstrlen(str.c_str());
			DWORD width = pSketch->GetTextWidth(str.c_str(), len);
			pSketch->Rectangle(-1, pos - height - 1, width + 4, pos);
			pSketch->Text(2, pos - 2, str.c_str(), len);
			D3D9DebugQueue.pop();
		}

		pSketch->EndDrawing(); // SKETCHPAD_DEBUG_TEXT
	}


	Perf_Mark(PERF_STAGE_SCENE_OTHER);

	gc->PopRenderTargets();	// Overlay
	gc->PopRenderTargets();	// Main Scene

	gc->HackFriendlyHack();
	gc->EndScene();

	dwTurn++;
}

