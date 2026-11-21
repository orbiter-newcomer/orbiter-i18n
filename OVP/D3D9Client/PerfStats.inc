// ============================================================================
// PerfStats.inc (D3D9Client)
//
// Implementation of the frame profiler declared in PerfStats.h. Included by
// D3D9Client.cpp only (single translation unit owns the profiler state and the
// GPU query objects).
//
// The profiler measures:
//   - the frame rate (frame interval between two clbkRenderScene calls),
//   - the CPU time spent between the stage marks of the renderer,
//   - the GPU time per stage with D3D9 event queries (PerfLog >= 2). The
//     queries are issued at the stage marks and polled after the render call,
//     so a stage time is the GPU wall time between the completions of two
//     consecutive queries. On a GPU-bound frame this is the real GPU time; when
//     the CPU is the bottleneck the GPU queries finish early and the values are
//     a lower bound (the CPU stage times are meaningful then).
//   - per frame counters (terrain tiles, shadow tiles, draw calls, vertices).
//
// Everything is logged to Orbiter.log in one "PERF:" line every 2 seconds.
// ============================================================================
#ifndef D3D9CLIENT_PERFSTATS_INC
#define D3D9CLIENT_PERFSTATS_INC

#include "PerfStats.h"
#include "D3D9Config.h"
#include "Log.h"
#include "D3D9Client.h"

#define PERF_LOG_INTERVAL_US	2000000.0	// log window [us] (2 s)
#define PERF_PROFILE_PERIOD_US	8000000.0	// GPU profiling period [us] (8 s)
#define PERF_PROFILE_FRAMES		20			// consecutively profiled frames per window
#define PERF_PROFILE_START_US	5000000.0	// delay of the first profiling window [us]

static const char *PerfStageName[PERF_STAGE_COUNT] = {
	"setup",	// PERF_STAGE_SETUP
	"sec",		// PERF_STAGE_SECSCENE
	"gbuf",		// PERF_STAGE_GBUFFER
	"sky",		// PERF_STAGE_CELSPHERE
	"shG0",		// PERF_STAGE_SHADOWG0
	"plPre",	// PERF_STAGE_PLANET_PRE
	"atmo",		// PERF_STAGE_ATMO
	"tLock",	// PERF_STAGE_TERRAIN_LOCK
	"tProc",	// PERF_STAGE_TERRAIN_PROCESS
	"tDraw",	// PERF_STAGE_TERRAIN_DRAW
	"tTail",	// PERF_STAGE_TERRAIN_TAIL
	"plPost",	// PERF_STAGE_PLANET_POST
	"mrk",		// PERF_STAGE_MARKERS
	"shG0b",	// PERF_STAGE_SHADOWG0B
	"ves",		// PERF_STAGE_VESSELS
	"sub",		// PERF_STAGE_SUBSYS
	"ckpt",		// PERF_STAGE_COCKPIT
	"fx",		// PERF_STAGE_POSTFX
	"hud",		// PERF_STAGE_HUD
	"dbg",		// PERF_STAGE_DEBUGOVL
	"oth"		// PERF_STAGE_SCENE_OTHER
};


class PerfStatsSystem
{
public:
	PerfStatsSystem();

	void BeginFrame(IDirect3DDevice9 *dev, double camera_alt);
	void Mark(int stage);
	void EndFrame();

	void AddUpdate(double us)  { if (enabled) updAcc += us; }
	void AddPresent(double us) { if (enabled) presAcc += us; }
	void AddTerrainTile()      { if (enabled) tileCount += 1.0; }

private:
	void EnsureQueries(IDirect3DDevice9 *dev);
	void ReleaseQueries();
	bool IssueQuery();
	void PollQueries();
	void LogWindow(double now);

	int    level;				// config PerfLog level (0=off)
	bool   enabled;				// profiling active this session/frame
	bool   profiling;			// this frame is inside a GPU profiling window
	bool   profilingPrev;		// previous frame was profiled (frame interval class)
	int    profileFrames;		// remaining frames of the current profiling window
	double profileNext;			// time [us] when the next window may start

	double frameBegin;			// time [us] of the current frame begin
	double prevFrameBegin;		// time [us] of the previous frame begin
	double fpsDtSum;			// frame interval sum of normal frames [us]
	int    fpsFrames;			// number of normal frame intervals
	double profDtSum;			// frame interval sum of profiled frames [us]
	int    profFrames;			// number of profiled frame intervals

	double cpuLast;				// time [us] of the last stage mark
	int    cpuStage;			// stage being measured right now
	double cpuAcc[PERF_STAGE_COUNT];
	double updAcc, presAcc;

	IDirect3DDevice9  *dev;
	IDirect3DQuery9   *query[PERF_MAX_SEG];
	int    nQuery;				// number of queries issued this frame
	int    segStage[PERF_MAX_SEG];	// stage that the segment ending at query[i] belongs to
	bool   gpuOK;				// GPU queries usable
	double gpuAcc[PERF_STAGE_COUNT];
	int    gpuFrames;			// number of profiled frames in the window

	double tileCount;
	DWORD  meshGrpLast, meshVertLast;
	double grpSum, vertSum;
	double updSum, presSum;

	double winStart;
	int    winFrames;
	double camAlt;
};


static PerfStatsSystem g_Perf;

bool PerfActive = false;

// ---------------------------------------------------------------------------
// public interface used by the official sources
// ---------------------------------------------------------------------------

void Perf_BeginFrame(IDirect3DDevice9 *dev, double camera_alt)	{ g_Perf.BeginFrame(dev, camera_alt); }
void Perf_MarkInternal(int stage)								{ g_Perf.Mark(stage); }
void Perf_EndFrame()											{ g_Perf.EndFrame(); }
void Perf_AddUpdate(double us)									{ g_Perf.AddUpdate(us); }
void Perf_AddPresent(double us)									{ g_Perf.AddPresent(us); }
void Perf_AddTerrainTileInternal()								{ g_Perf.AddTerrainTile(); }


// ---------------------------------------------------------------------------
// implementation
// ---------------------------------------------------------------------------

PerfStatsSystem::PerfStatsSystem()
{
	level = 0;
	enabled = false;
	profiling = false;
	profilingPrev = false;
	profileFrames = 0;
	profileNext = 0.0;
	frameBegin = prevFrameBegin = 0.0;
	fpsDtSum = profDtSum = 0.0;
	fpsFrames = profFrames = 0;
	cpuLast = 0.0;
	cpuStage = PERF_STAGE_SETUP;
	memset(cpuAcc, 0, sizeof(cpuAcc));
	updAcc = presAcc = 0.0;
	dev = NULL;
	memset(query, 0, sizeof(query));
	nQuery = 0;
	memset(segStage, 0, sizeof(segStage));
	gpuOK = false;
	memset(gpuAcc, 0, sizeof(gpuAcc));
	gpuFrames = 0;
	tileCount = 0.0;
	meshGrpLast = meshVertLast = 0;
	grpSum = vertSum = 0.0;
	updSum = presSum = 0.0;
	winStart = 0.0;
	winFrames = 0;
	camAlt = 0.0;
}

void PerfStatsSystem::BeginFrame(IDirect3DDevice9 *pDev, double camera_alt)
{
	camAlt = camera_alt;
	level = (Config ? Config->PerfLog : 0);
	if (level < 0) level = 0;
	if (level > 3) level = 3;
	enabled = (level > 0);
	PerfActive = enabled;
	if (!enabled) return;

	double now = D3D9GetTime();
	if (winStart == 0.0) winStart = now;

	// frame interval of the previous frame
	if (prevFrameBegin > 0.0) {
		double dt = now - prevFrameBegin;
		if (profilingPrev) { profDtSum += dt; profFrames++; }
		else               { fpsDtSum += dt; fpsFrames++; }
	}
	prevFrameBegin = now;
	frameBegin = now;

	cpuLast = now;
	cpuStage = PERF_STAGE_SETUP;

	if (profileNext == 0.0) profileNext = now + PERF_PROFILE_START_US;

	// start a GPU profiling window when it's due
	if (level >= 2 && !profiling && now >= profileNext) {
		if (pDev != dev) { ReleaseQueries(); dev = pDev; }
		if (!gpuOK) EnsureQueries(pDev);
		if (gpuOK) {
			profiling = true;
			profileFrames = PERF_PROFILE_FRAMES;
		}
		else {
			profileNext = now + PERF_PROFILE_PERIOD_US;	// retry later
		}
	}

	profilingPrev = profiling;
	nQuery = 0;

	if (profiling) {
		// frame start marker (query 0, no stage)
		segStage[0] = -1;
		if (gpuOK && query[0] && SUCCEEDED(query[0]->Issue(D3DISSUE_END))) {
			nQuery = 1;
		}
		else {
			profiling = false;
			gpuOK = false;
			profileNext = now + PERF_PROFILE_PERIOD_US;
		}
	}
}

void PerfStatsSystem::Mark(int stage)
{
	if (!enabled) return;

	double now = D3D9GetTime();
	cpuAcc[cpuStage] += now - cpuLast;
	cpuLast = now;

	if (profiling) IssueQuery();	// closes the segment of the previous stage
	cpuStage = stage;
}

bool PerfStatsSystem::IssueQuery()
{
	if (!gpuOK || !profiling) return false;
	if (nQuery < 1 || nQuery >= PERF_MAX_SEG) return false;
	if (!query[nQuery] || FAILED(query[nQuery]->Issue(D3DISSUE_END))) {
		gpuOK = false;
		return false;
	}
	segStage[nQuery] = cpuStage;
	nQuery++;
	return true;
}

void PerfStatsSystem::EndFrame()
{
	if (!enabled) return;

	double now = D3D9GetTime();
	cpuAcc[cpuStage] += now - cpuLast;
	cpuLast = now;

	if (profiling) {
		IssueQuery();		// closes the last stage segment
		PollQueries();
		profileFrames--;
		if (profileFrames <= 0) {
			profiling = false;
			profileNext = now + PERF_PROFILE_PERIOD_US;
		}
	}

	// per frame counters (the mesh counters are free running, use deltas)
	DWORD grp = D3D9Stats.Mesh.MeshGrps;
	DWORD vtx = D3D9Stats.Mesh.Vertices;
	grpSum += (grp >= meshGrpLast) ? double(grp - meshGrpLast) : double(grp);
	vertSum += (vtx >= meshVertLast) ? double(vtx - meshVertLast) : double(vtx);
	meshGrpLast = grp;
	meshVertLast = vtx;

	winFrames++;

	if (now - winStart >= PERF_LOG_INTERVAL_US) LogWindow(now);
}

void PerfStatsSystem::EnsureQueries(IDirect3DDevice9 *pDev)
{
	if (!pDev) return;
	gpuOK = true;
	for (int i = 0; i < PERF_MAX_SEG; i++) {
		query[i] = NULL;
		if (FAILED(pDev->CreateQuery(D3DQUERYTYPE_EVENT, &query[i])) || !query[i]) {
			gpuOK = false;
			break;
		}
	}
	if (!gpuOK) {
		ReleaseQueries();
		oapiWriteLogV("PERF: GPU event queries not available, GPU profiling disabled");
	}
	else {
		oapiWriteLogV("PERF: GPU profiling active (%d event queries, %d frames every %.1f s)",
			PERF_MAX_SEG, PERF_PROFILE_FRAMES, PERF_PROFILE_PERIOD_US * 1e-6);
	}
}

void PerfStatsSystem::ReleaseQueries()
{
	for (int i = 0; i < PERF_MAX_SEG; i++) {
		if (query[i]) { query[i]->Release(); query[i] = NULL; }
	}
	gpuOK = false;
}

void PerfStatsSystem::PollQueries()
{
	if (!gpuOK || nQuery <= 0) return;

	double tPrev = 0.0;
	bool bOK = true;
	HRESULT hrFail = S_OK;
	int mFail = -1;

	for (int m = 0; m < nQuery && bOK; m++) {
		if (!query[m]) { bOK = false; break; }

		DWORD dwData = 0;
		HRESULT hr = query[m]->GetData(&dwData, 0, D3DGETDATA_FLUSH);
		int spin = 0;
		while (hr == S_FALSE) {
			Sleep(0);
			hr = query[m]->GetData(&dwData, 0, D3DGETDATA_FLUSH);
			if (++spin > 5000) { hr = E_FAIL; break; }
		}
		if (hr != S_OK) { bOK = false; hrFail = hr; mFail = m; break; }

		double t = D3D9GetTime();
		if (m > 0) {
			int st = segStage[m];
			if (st >= 0 && st < PERF_STAGE_COUNT) gpuAcc[st] += (t - tPrev);
		}
		tPrev = t;
	}

	if (bOK) gpuFrames++;
	else {
		ReleaseQueries();
		oapiWriteLogV("PERF: GPU query polling failed (hr=0x%08X at query %d of %d), GPU profiling disabled",
			(unsigned)hrFail, mFail, nQuery);
	}
}

void PerfStatsSystem::LogWindow(double now)
{
	if (winFrames < 1) winFrames = 1;

	char buf[2048];
	int n = 0;

	double fps = (fpsFrames > 0) ? 1e6 * double(fpsFrames) / fpsDtSum : 0.0;
	double fpsp = (profFrames > 0) ? 1e6 * double(profFrames) / profDtSum : 0.0;
	double ms = (fpsFrames > 0) ? fpsDtSum / double(fpsFrames) * 1e-3 : 0.0;

	n += _snprintf_s(buf + n, sizeof(buf) - n, _TRUNCATE,
		"PERF fps=%.1f ms=%.1f alt=%.0f cfg aa=%d pp=%d smm=%d sms=%d env=%d irr=%d atmo=%d trd=%d | cpu",
		fps, ms, camAlt, Config->SceneAntialias, Config->PostProcess,
		Config->ShadowMapMode, Config->ShadowMapSize,
		Config->EnvMapMode, Config->bIrradiance, Config->bAtmoQuality, Config->TerrainTransitionMs);

	for (int i = 0; i < PERF_STAGE_COUNT && n < (int)sizeof(buf) - 32; i++) {
		n += _snprintf_s(buf + n, sizeof(buf) - n, _TRUNCATE, " %s=%.1f",
			PerfStageName[i], cpuAcc[i] / double(winFrames) * 1e-3);
	}

	if (level >= 2) {
		double fpsp_ms = (profFrames > 0) ? profDtSum / double(profFrames) * 1e-3 : 0.0;
		n += _snprintf_s(buf + n, sizeof(buf) - n, _TRUNCATE, " | fpsp=%.1f msP=%.1f gpu", fpsp, fpsp_ms);
		for (int i = 0; i < PERF_STAGE_COUNT && n < (int)sizeof(buf) - 32; i++) {
			double g = (gpuFrames > 0) ? gpuAcc[i] / double(gpuFrames) * 1e-3 : 0.0;
			n += _snprintf_s(buf + n, sizeof(buf) - n, _TRUNCATE, " %s=%.1f", PerfStageName[i], g);
		}
	}

	n += _snprintf_s(buf + n, sizeof(buf) - n, _TRUNCATE,
		" | cnt tile=%.0f grp=%.0f vtx=%.0f upd=%.1f pres=%.1f",
		tileCount / double(winFrames),
		grpSum / double(winFrames), vertSum / double(winFrames),
		updAcc / double(winFrames) * 1e-3, presAcc / double(winFrames) * 1e-3);

	oapiWriteLogV("%s", buf);

	// reset the window
	memset(cpuAcc, 0, sizeof(cpuAcc));
	memset(gpuAcc, 0, sizeof(gpuAcc));
	gpuFrames = 0;
	updAcc = presAcc = 0.0;
	updSum = presSum = 0.0;
	tileCount = 0.0;
	grpSum = vertSum = 0.0;
	fpsDtSum = profDtSum = 0.0;
	fpsFrames = profFrames = 0;
	winFrames = 0;
	winStart = now;
}

#endif // D3D9CLIENT_PERFSTATS_INC
