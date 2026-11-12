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

// Relativistic aberration: shift a celestial-sphere direction from the rest
// frame to the observer frame.  cos(theta') = (cos(theta)+beta)/(1+beta*cos(theta))
// (all orders in beta; the transverse direction is preserved).
float3 AberrateDir(float3 d)
{
	float beta = Const.vAberr.w;
	float3 vdir = Const.vAberr.xyz;

	float c  = dot(d, vdir);
	float c2 = (c + beta) / max(1.0 + beta * c, 1e-6);
	c2 = clamp(c2, -1.0, 1.0);

	float3 perp = d - vdir * c;
	float pl = length(perp);
	perp = (pl > 1e-6) ? perp / pl : float3(0.0, 0.0, 0.0);
	return normalize(vdir * c2 + perp * sqrt(max(0.0, 1.0 - c2 * c2)));
}

// Gravitational lensing: bend the sampling direction near the celestial
// bodies. The returned direction is the source direction to sample for a given
// observed direction. The weak field model (1PN) uses alpha = 4*mu/b, the Kerr
// model adds the higher order Schwarzschild post-Newtonian terms.
float3 LensDirection(float3 d)
{
	if (Const.fLensCount < 0.5) return d;

	for (int i = 0; i < 12; i++) {
		if (float(i) >= Const.fLensCount) break;

		float4 ld = Const.vLens[i];
		float dst = ld.w;
		float mu = Const.fLensMu[i];
		if (dst < 1.0 || mu < 1e-6) continue;

		float cosA = dot(d, ld.xyz);
		float sinA = sqrt(max(1e-8, 1.0 - cosA * cosA));
		float b = dst * sinA;					// impact parameter [m]
		if (b < 1e-3) b = 1e-3;

		float alpha;
		if (Const.fLensModel > 0.5) {
			// Schwarzschild post-Newtonian expansion (higher order terms)
			float x = mu / b;
			alpha = 4.0*x
			      + 15.0*3.14159265*0.25*x*x
			      + 128.0/3.0*x*x*x
			      + 3465.0*3.14159265/64.0*x*x*x*x;
		}
		else {
			alpha = 4.0 * mu / b;				// 1PN weak field
		}
		alpha = min(alpha, 2.5);				// keep the sampling stable

		if (alpha < 1e-12) continue;

		float3 perp = ld.xyz - d * cosA;
		float pl = length(perp);
		if (pl < 1e-6) continue;
		perp = perp / pl;
		// the source appears shifted towards the body: sample away from it
		d = normalize(d - perp * alpha);
	}
	return d;
}

CelSphereVS CelVS(TILEVERTEX vrt)
{
	// Zero output.
	CelSphereVS outVS = (CelSphereVS)0;
	float3 posW = mul(float4(vrt.posL, 1.0f), Const.mWorld).xyz;

	// Relativistic aberration: move the background image direction from the
	// rest frame to the observer frame (texture coordinates stay attached to
	// the vertices, so the image content follows the shifted direction).
	if (Const.fAberrOn > 0.0) {
		float len = length(posW);
		if (len > 1e-6) posW = AberrateDir(posW / len) * len;
	}

	// Gravitational lensing: bend the star field near the celestial bodies
	{
		float len2 = length(posW);
		if (len2 > 1e-6) posW = LensDirection(posW / len2) * len2;
	}

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

