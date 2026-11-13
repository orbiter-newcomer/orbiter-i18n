// ==============================================================
// Part of the ORBITER VISUALISATION PROJECT (OVP)
// Licensed under LGPL v2
// Copyright (C) 2021-2026 Jarmo Nikkanen
// ==============================================================

#define BOOL bool

struct CelDataStruct
{
	float4x4 mWorld;
	float4x4 mViewProj;
	float	 fAlpha;
	float	 fBeta;
	float	 fPad0;
	float	 fPad1;
	float4	 vAberr;		// xyz = velocity direction (world frame), w = beta
	float	 fAberrOn;		// 1 = relativistic aberration enabled
	float	 fPad2;
	float	 fPad3;
	float	 fPad4;
	// gravitational lensing (up to 12 celestial bodies)
	float4	 vLens[12];		// xyz = body direction (camera frame), w = distance [m]
	float	 fLensMu[12];	// GM/c^2 of the body [m]
	float	 fLensCount;	// number of valid bodies (0 = lensing disabled)
	float	 fLensModel;	// 0 = 1PN weak field, 1 = Kerr/Schwarzschild
	float	 fPad5;
	float	 fPad6;
	// diagnostic: render the tiles as white wireframe (tile boundary check)
	float	 fColorMode;	// 1 = flat white output
	float	 fPad7;
	float	 fPad8;
	float	 fPad9;
	// equirectangular panorama projection (lensing source frame rendering)
	float	 fPanoMode;		// 1 = equirectangular projection
	float	 fPanoCenterU;	// tile centre longitude (panorama u), for wrapping
	float	 fPad10;
	float	 fPad11;
	// gravitational aberration (static observer inside the potential well)
	float4	 vFrameDir;		// xyz = observer -> dominant body (world frame), w = enable flag
	float4	 fFrame;		// x = GM/c^2 [m], y = observer distance [m]
};

// Booleans go to sepatate structure. (not mixing different datatypes in a structure)
// Also bool is 32-bits in HLSL therefore must use BOOL in C++ structure
struct CelDataFlow
{
	BOOL	 bAlpha;
	BOOL	 bBeta;
};

struct TILEVERTEX					// (VERTEX_2TEX) Vertex declaration used for surface tiles and cloud layer
{
	float3 posL     : POSITION0;
	float3 normalL  : NORMAL0;
	float2 tex0     : TEXCOORD0;
	float  elev		: TEXCOORD1;
};

struct CelSphereVS
{
	float4 posH    : POSITION0;
	float2 tex0    : TEXCOORD0;
};

uniform extern CelDataStruct Const;
uniform extern CelDataFlow Flow;

sampler2D tTexA;
sampler2D tTexB;

#include "RelativityShaders.hlsl"

// Gravitational aberration (inverse): coordinate/source frame direction ->
// local rest frame direction of a static observer at distance r from the
// dominant source. Inverse of FrameInverse() in StarsLens.hlsl:
//     tan(alpha_local) = tan(alpha_geom) * sqrt(1 - rs/r)
// where alpha is measured from the outward radial direction.
float3 FrameInverseInv(float3 d)
{
	// shared implementation: RelativityMapping.hlsl
	return RelFrameMapInv(d, Const.vFrameDir.xyz, Const.fFrame.x, Const.fFrame.y);
}

CelSphereVS CelVS(TILEVERTEX vrt)
{
	// Zero output.
	CelSphereVS outVS = (CelSphereVS)0;
	float3 posW = mul(float4(vrt.posL, 1.0f), Const.mWorld).xyz;

	// Gravitational lensing (exact inverse, stabilized): the plain star field
	// must follow the same mapping as the lensed cubemap near the "hole"
	// boundary. The cubemap composite uses LensInverseCompat(observed)->source;
	// here the inverse problem is solved with the smooth first-order inverse as
	// the initial guess and a few damped, displacement-limited refinement steps
	// on the exact mapping. If the refinement does not converge (strong or
	// folded field) the smooth first-order result is kept, so the tile mesh
	// never tears; the strong-field part is fully covered by the cubemap.
	if (Const.fLensCount > 0.5) {
		float lenL = length(posW);
		if (lenL > 1e-6) {
			float3 dSrc = posW / lenL;
			// smooth first-order inverse as the initial guess
			float3 dFirst = LensDirection(dSrc);			// observed -> source
			float3 delta0 = dFirst - dSrc;
			float3 dGuess = normalize(dSrc - delta0);		// observed direction guess
			float3 dObs = dGuess;
			float3 dCheck = LensInverseCompat(dObs);		// observed -> source
			[unroll] for (int itL = 0; itL < 2; itL++) {
				float3 corr = dSrc - dCheck;
				float cl = length(corr);
				if (cl > 1e-6f) {
					float lim = min(cl, 0.0175f);			// max ~1 deg correction step
					dObs = normalize(dObs + corr * (lim / cl));
					dCheck = LensInverseCompat(dObs);
				}
			}
			// convergence guard: fall back to the smooth first-order result
			float res = length(dSrc - dCheck);
			if (!(res < 0.02f)) dObs = dGuess;
			// final displacement limit (~5 deg); larger deflections are cubemap-only
			float3 delta = dObs - dSrc;
			float dl = length(delta);
			const float maxA = 0.0873;
			if (dl > maxA) dObs = normalize(dSrc + delta * (maxA / dl));
			posW = dObs * lenL;
		}
	}

	// Gravitational aberration (static observer inside the potential well):
	// bring the source/coordinate frame direction into the local rest frame
	// first. This is the inverse of FrameInverse() used by the lensed cubemap
	// composite path, so the plain star field and the lensed cubemap stay
	// consistent at the "hole" boundary.
	if (Const.vFrameDir.w > 0.5) {
		float lenG = length(posW);
		if (lenG > 1e-6) posW = FrameInverseInv(posW / lenG) * lenG;
	}

	// Relativistic aberration: move the background image direction from the
	// rest frame to the observer frame (texture coordinates stay attached to
	// the vertices, so the image content follows the shifted direction).
	if (Const.fAberrOn > 0.0) {
		float len = length(posW);
		if (len > 1e-6) posW = AberrateDir(posW / len) * len;
	}

	// NOTE: no gravitational lensing here. The plain star field / background
	// image path must stay un-lensed (only the kinematic aberration above is
	// applied): the lensing is handled by the lensed-cubemap composite pass.
	// The former LensDirection() call here bent the background image away and
	// made the milky way disappear from the plain star field rendering.

	// Equirectangular panorama projection (lensing source frame render):
	// output the spherical longitude/latitude as clip space coordinates.
	if (Const.fPanoMode > 0.5) {
		float3 dir = normalize(posW);
		float u = atan2(dir.z, dir.x) * (0.5 / 3.14159265) + 0.5;
		float v = acos(clamp(dir.y, -1.0, 1.0)) / 3.14159265;

		// keep the longitude continuous across the +-180 degree wrap for the
		// triangles of this tile
		if (u - Const.fPanoCenterU > 0.5) u -= 1.0;
		else if (u - Const.fPanoCenterU < -0.5) u += 1.0;

		outVS.posH = float4(u * 2.0 - 1.0, 1.0 - v * 2.0, 0.0, 1.0);
	}
	else {
		outVS.posH = mul(float4(posW, 1.0f), Const.mViewProj);
	}

	outVS.tex0 = vrt.tex0;
	return outVS;
}

float4 CelPS(CelSphereVS frg) : COLOR
{
	// diagnostic: flat white output (used together with the wireframe fill
	// mode to visualise the background tile boundaries)
	if (Const.fColorMode > 0.5) return float4(1.0, 1.0, 1.0, 1.0);

	float3 vColor = 0;
	if (Flow.bAlpha) vColor += tex2D(tTexA, frg.tex0).rgb * Const.fAlpha;
	if (Flow.bBeta)  vColor += tex2D(tTexB, frg.tex0).rgb * Const.fBeta;
	return float4(vColor, 1.0);
}

