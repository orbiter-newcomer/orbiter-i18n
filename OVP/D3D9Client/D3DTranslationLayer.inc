//-----------------------------------------------------------------------------
// D3D9 translation layer loader for D3D9Client backend variants.
//
// The same D3D9Client renderer is built as three engine plugins:
//   D3D9Client.dll      : system D3D9 (or Direct3D9On12) - original behaviour
//   D3D9ClientDXVK.dll  : loads RenderingBackends\dxvk\d3d9.dll    (Vulkan)
//   D3D9ClientDG.dll    : loads RenderingBackends\dgvoodoo\D3D9.dll (D3D11)
//
// The backend is selected at build time with a compile definition, so each
// variant is a regular Orbiter graphics-engine plugin and can be selected in
// the Launchpad video tab through the official engine mechanism.
//-----------------------------------------------------------------------------
static HMODULE g_hD3D9TranslationLayer = NULL;

static IDirect3D9 *CreateD3D9Object()
{
#if defined(D3D9CLIENT_BACKEND_DXVK) || defined(D3D9CLIENT_BACKEND_DGVoodoo)
#  if defined(D3D9CLIENT_BACKEND_DXVK)
	const char *dllpath = "RenderingBackends\\dxvk\\d3d9.dll";
	const char *label = "DXVK (Vulkan)";
#  else
	const char *dllpath = "RenderingBackends\\dgvoodoo\\D3D9.dll";
	const char *label = "dgVoodoo2 (D3D11)";
#  endif
	HMODULE h = LoadLibraryA(dllpath);
	if (h) {
		typedef IDirect3D9 *(__stdcall *PFN_Direct3DCreate9)(UINT);
		PFN_Direct3DCreate9 pfn = (PFN_Direct3DCreate9)GetProcAddress(h, "Direct3DCreate9");
		if (pfn) {
			g_hD3D9TranslationLayer = h;
			char msg[128];
			sprintf_s(msg, "[D3D9] Render backend: %s", label);
			oapiWriteLog(msg);
			return pfn(D3D_SDK_VERSION);
		}
		FreeLibrary(h);
	}
	char msg[256];
	sprintf_s(msg, "[D3D9][WARNING] Render backend wrapper not available (%s), using system D3D9", dllpath);
	oapiWriteLog(msg);
	return Direct3DCreate9(D3D_SDK_VERSION);
#else
	return Direct3DCreate9(D3D_SDK_VERSION);
#endif
}
