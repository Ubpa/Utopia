#include "StdPipeline.hlsli"

STD_PIPELINE_CB_PER_OBJECT(0);

cbuffer cbPerMaterial : register(b1)
{
	float3 gColor;
};

STD_PIPELINE_CB_PER_CAMERA(2);

struct VertexIn
{
	float3 PosL : POSITION;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
	
    // Transform to world space.
    float4 posW = mul(gWorld, float4(vin.PosL, 1.0f));

    // Transform to homogeneous clip space.
    vout.PosH = mul(gViewProj, posW);

    return vout;
}

struct PixelOut {
	float4 color : SV_Target0;
};

PixelOut PS(VertexOut pin)
{
	PixelOut pout;
	pout.color = float4(gColor, 1);
	
	return pout;
}
