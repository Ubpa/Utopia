#include "StdPipeline.hlsli"

STD_PIPELINE_CB_PER_OBJECT(0);

STD_PIPELINE_CB_DIRECTIONAL_SHADOW(1);

struct VertexIn
{
    float3 PosL     : POSITION;
};

struct VertexOut
{
    float4 PosH     : SV_POSITION;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut)0.0f;
    
    // Transform to world space.
    float4 posW = mul(gWorld, float4(vin.PosL, 1.0f));

    // Transform to homogeneous clip space.
    vout.PosH = mul(gDirectionalShadowViewProj, posW);

    return vout;
}

void PS(VertexOut pin)
{
    // do nothing
}
