TextureCube  gSkybox       : register(t0);
SamplerState gSampleLinear : register(s0);

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

cbuffer cbPerCamera: register(b0)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;

	float3 gEyePosW;
	float _g_cbPerFrame_pad0;

	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;

	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;
};

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
    return gSkybox.Sample(gSampleLinear, normalize(pin.TexC));
}
