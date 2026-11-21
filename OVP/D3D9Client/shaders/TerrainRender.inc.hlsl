// ============================================================================
// TerrainRender.inc.hlsl (D3D9Client shaders)
//
// Terrain/shadow shader code kept outside the official NewPlanet.hlsl.
// Included by NewPlanet.hlsl after the shared uniforms/samplers; the official
// shader only keeps the include hook and the TerrainPS call sites.
// ============================================================================
// Terrain LOD cross fade: 1 = fully sharp (normal rendering), 0 = maximum
// low-pass (start of the transition). The fade is done by sampling the tile's
// own texture with a coarser mip level during the transition (opaque, no extra
// sampler and no alpha blending), see TerrainPS.
uniform extern float TerrainTileFade;

// ---------------------------------------------------------------------------------------------------
// Unified scene shadow map (G1) sampling (bound to the tShadowMap sampler). Same 9 tap pattern as the near
// field map; the coverage border handling/fade is done by the caller.
//
float SampleShadowsT(float2 sp, float pd)
{
	float2 dx = float2(Prm.vSHD2.y, 0) * 1.5f;
	float2 dy = float2(0, Prm.vSHD2.y) * 1.5f;
	float  va = 0;

	sp -= dy;
	if ((tex2D(tShadowMap, sp - dx).r) > pd) va++;
	if ((tex2D(tShadowMap, sp).r) > pd) va++;
	if ((tex2D(tShadowMap, sp + dx).r) > pd) va++;
	sp += dy;
	if ((tex2D(tShadowMap, sp - dx).r) > pd) va++;
	if ((tex2D(tShadowMap, sp).r) > pd) va++;
	if ((tex2D(tShadowMap, sp + dx).r) > pd) va++;
	sp += dy;
	if ((tex2D(tShadowMap, sp - dx).r) > pd) va++;
	if ((tex2D(tShadowMap, sp).r) > pd) va++;
	if ((tex2D(tShadowMap, sp + dx).r) > pd) va++;

	return va * 0.1111111f;
}

// ============================================================================
// Unified scene shadow map (G1) caster rendering: depth only rendering of the
// terrain tiles. The output convention matches the near field vessel shadow
// map (NewMesh.hlsl ShdMapVS/ShdMapPS): the stored value is 1 - z/w, so a
// larger value means "closer to the light".
//
// These entries are compiled by the ShadowSystem into a dedicated depth only
// shader (see TerrainRender.inc) - keeping them here avoids an extra shader file.
// ============================================================================

uniform extern struct ShadowCasterConst {
	float4x4 mVP;		// scene shadow light view projection
	float4x4 mW;		// tile world matrix
} shd_const;

struct SHDVERTEX
{
	float3 posL : POSITION0;
};

struct SHDVSOUT
{
	float4 posH : POSITION0;
	float2 dstW : TEXCOORD0;
};

SHDVSOUT ShadowVS(SHDVERTEX vrt)
{
	SHDVSOUT outVS = (SHDVSOUT)0;
	float3 posW = mul(float4(vrt.posL, 1.0f), shd_const.mW).xyz;
	outVS.posH = mul(float4(posW, 1.0f), shd_const.mVP);
	outVS.dstW = outVS.posH.zw;
	return outVS;
}

float4 ShadowPS(SHDVSOUT frg) : COLOR
{
	return 1 - (frg.dstW.x / frg.dstW.y);
}
