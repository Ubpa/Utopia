#include "Forward.hlsl"

float4 HoleVS(float3 PosL : POSITION) : SV_POSITION {
	float4 posW = mul(gWorld, float4(0.80f * PosL, 1.0f));
	float4 posH = mul(gViewProj, posW);
	return posH;
}

void HolePS() {}
