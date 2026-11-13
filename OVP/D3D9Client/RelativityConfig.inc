// -----------------------------------------------------------------------------
// Relativity: D3D9Client configuration defaults and file I/O.
//
// Implementation moved out of the official D3D9Config.cpp (included there).
// The configuration fields themselves remain in D3D9Config.h.
// -----------------------------------------------------------------------------

static void RelD3D9ConfigReset (D3D9Config *cfg)
{
	cfg->RelDoppler			= 0;
	cfg->RelAberration		= 0;
	cfg->RelLensing			= 0;
	cfg->RelGravRedshift	= 0;
	cfg->RelBodyMarkers		= 0;
	cfg->RelGravityMethod	= 0;
	cfg->RelInternalMode		= 0;
	cfg->RelCharge			= 0;
	cfg->RelChargeRatio		= 0.5;
	cfg->RelExposureWhite	= 2.0;
	cfg->RelNearGuard		= 500.0;
	cfg->LensCubemapSize		= 1024;
	cfg->LensBlendStart		= 0.2;
	cfg->LensBlendEnd		= 1.0;
	cfg->LensBrightness		= 1.0;
	cfg->RelLensWireframe	= false;
}

static void RelD3D9ConfigRead (FILEHANDLE hFile, D3D9Config *cfg)
{
	int i;
	double d;
	if (oapiReadItem_int   (hFile, (char*)"RelInternalMode", i))				cfg->RelInternalMode = max(0, min(2, i));
	if (oapiReadItem_float (hFile, (char*)"RelNearGuard", d))				cfg->RelNearGuard = max(0.0, min(1e6, d));
	if (oapiReadItem_int   (hFile, (char*)"LensCubemapSize", i)) {
		// 512, 1024 or 2048 pixels per cubemap face (higher = smoother lensing
		// boundaries, higher memory and render cost)
		if (i <= 512) cfg->LensCubemapSize = 512;
		else if (i >= 2048) cfg->LensCubemapSize = 2048;
		else cfg->LensCubemapSize = 1024;
	}
	if (oapiReadItem_float (hFile, (char*)"LensBlendStart", d))		cfg->LensBlendStart = max(0.0, min(45.0, d)); 	if (oapiReadItem_float (hFile, (char*)"LensBlendEnd", d))		cfg->LensBlendEnd = max(0.0, min(90.0, d)); 	if (oapiReadItem_float (hFile, (char*)"LensBrightness", d))		cfg->LensBrightness = max(-10.0, min(4.0, d));   // negative = debug: show the raw lens cubemap 	if (cfg->LensBlendEnd <= cfg->LensBlendStart) 		cfg->LensBlendEnd = cfg->LensBlendStart + 0.1;
}

static void RelD3D9ConfigWrite (FILEHANDLE hFile, const D3D9Config *cfg)
{
	oapiWriteItem_int   (hFile, (char*)"RelInternalMode", cfg->RelInternalMode);
	oapiWriteItem_float (hFile, (char*)"RelNearGuard", cfg->RelNearGuard);
	oapiWriteItem_int   (hFile, (char*)"LensCubemapSize", cfg->LensCubemapSize);
	oapiWriteItem_float (hFile, (char*)"LensBlendStart", cfg->LensBlendStart);
	oapiWriteItem_float (hFile, (char*)"LensBlendEnd", cfg->LensBlendEnd);
	oapiWriteItem_float (hFile, (char*)"LensBrightness", cfg->LensBrightness);
}
