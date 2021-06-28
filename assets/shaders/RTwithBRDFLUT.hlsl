#include "StdPipeline.hlsli"
#include "PBR.hlsli"

STD_PIPELINE_CB_PER_CAMERA(0);

Texture2D gSn : register(t0);
Texture2D gUn : register(t1);

// 0  [albedo   roughness]
// 1  [  N      metalness]
// 2  [emission matID    ]
Texture2D gGBuffer0     : register(t2);
Texture2D gGBuffer1     : register(t3);
Texture2D gGBuffer2     : register(t4);
Texture2D gDepthStencil : register(t5);

Texture2D StdPipeline_BRDFLUT : register(t6);

static const float2 gTexCoords[6] =
{
    float2(0.0f, 1.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 1.0f)
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float2 TexC : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vout;

    vout.TexC = gTexCoords[vid];

    // Quad covering screen in NDC space.
    vout.PosH = float4(2.0f*vout.TexC.x - 1.0f, 1.0f - 2.0f*vout.TexC.y, 0.0f, 1.0f);

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 data0 = gGBuffer0.Sample(gSamplerPointClamp, pin.TexC);
    float4 data1 = gGBuffer1.Sample(gSamplerPointClamp, pin.TexC);
    float4 data2 = gGBuffer2.Sample(gSamplerPointClamp, pin.TexC);
    float depth = gDepthStencil.Sample(gSamplerPointClamp, pin.TexC).r;
	
	float3 Sn = gSn.Sample(gSamplerPointClamp, pin.TexC).xyz;
	float3 Un = gUn.Sample(gSamplerPointClamp, pin.TexC).xyz;
	
	float4 posHC = float4(
		2 * pin.TexC.x - 1,
		1 - 2 * pin.TexC.y,
		depth,
		1.f
	);
	float4 posHW = mul(gInvViewProj, posHC);
	float3 posW = posHW.xyz / posHW.w;
	
	float3 albedo = data0.xyz;
	float roughness = lerp(0.01,1,data0.w);
	float3 N = data1.xyz;
	float metalness = data1.w;
	float matID = data2.w;

	if (matID == 0)
	    return float4(0.f,0.f,0.f,1.f);

	float alpha = roughness * roughness;
	float3 V = normalize(gEyePosW - posW);
	float3 F0 = MetalWorkflow_F0(albedo, metalness);
    float3 up = float3(0.0, 1.0, 0.0);
    float3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));
	float NdotV = saturate(dot(N, V));
	float2 scale_bias = STD_PIPELINE_SAMPLE_BRDFLUT(NdotV, roughness);
	float3 diffuseBRDF = (1-metalness)*albedo;
	float3 specBRDF = F0 * scale_bias.x + scale_bias.y;
	return float4(diffuseBRDF * Sn + specBRDF * Un, 1);
}
