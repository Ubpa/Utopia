Texture2D gAlbedoMap    : register(t0);
Texture2D gRoughnessMap : register(t1);
Texture2D gMetalnessMap : register(t2);

SamplerState gsamLinear  : register(s0);

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
};

cbuffer cbPerMaterial : register(b1)
{
	float3 gAlbedoFactor;
    float  gRoughnessFactor;
    float  gMetalnessFactor;
};

cbuffer cbPerCamera: register(b2)
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

struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
	float2 TexC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
	
    // Transform to world space.
    float4 posW = mul(gWorld, float4(vin.PosL, 1.0f));
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul((float3x3)gWorld, vin.NormalL);

    // Transform to homogeneous clip space.
    vout.PosH = mul(gViewProj, posW);
	
	// Output vertex attributes for interpolation across triangle.
    vout.TexC = vin.TexC;

    return vout;
}

struct PixelOut {
	float4 gbuffer0    : SV_Target0;
	float4 gbuffer1    : SV_Target1;
	float4 gbuffer2    : SV_Target2;
};

PixelOut PS(VertexOut pin)
{
	PixelOut pout;
	
    float3 albedo    = gAlbedoFactor    * gAlbedoMap.Sample(gsamLinear, pin.TexC).xyz;
    float  roughness = gRoughnessFactor * gRoughnessMap.Sample(gsamLinear, pin.TexC).x;
    float  metalness = gMetalnessFactor * gMetalnessMap.Sample(gsamLinear, pin.TexC).x;
	
	pout.gbuffer0 = float4(albedo, roughness);
	pout.gbuffer1 = float4(normalize(pin.NormalW), metalness);
	pout.gbuffer2 = float4(pin.PosW, 0.0f);
	
	return pout;
}
