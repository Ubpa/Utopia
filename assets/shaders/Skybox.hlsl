#include "StdPipeline.hlsli"

TextureCube  gSkybox : register(t0);

static const float3 gPositions[8] =
{
    float3(-1.0f, -1.0f, -1.0f),
    float3(-1.0f, -1.0f, 1.0f),
    float3(-1.0f, 1.0f, -1.0f),
    float3(-1.0f, 1.0f, 1.0f),
    float3(1.0f, -1.0f, -1.0f),
    float3(1.0f, -1.0f, 1.0f),
    float3(1.0f, 1.0f, -1.0f),
    float3(1.0f, 1.0f, 1.0f),
};

STD_PIPELINE_CB_PER_CAMERA(0);

// front to inner
static const uint gIndexMap[36] =
{
	4, 6, 0,
	2, 0, 6,
	
	2, 3, 0,
	1, 0, 3,
	
	1, 5, 0,
	4, 0, 5,
	
	3, 7, 1,
	5, 1, 7,
	
	5, 7, 4,
	6, 4, 7,
	
	6, 7, 2,
	3, 2, 7
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float3 TexC    : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vout;

	float3x3 view_without_pos = (float3x3)gView;
    float3 pos = mul(view_without_pos, gPositions[gIndexMap[vid]]);
    vout.PosH = mul(gProj, float4(pos, 1.0f));
	vout.PosH.z = vout.PosH.w;
	vout.TexC = gPositions[gIndexMap[vid]];

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return gSkybox.Sample(gSamplerLinearWrap, normalize(pin.TexC));
}
