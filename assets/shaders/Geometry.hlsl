Texture2D gAlbedoMap    : register(t0);
Texture2D gRoughnessMap : register(t1);
Texture2D gMetalnessMap : register(t2);
Texture2D gNormalMap    : register(t3);

SamplerState gSamplerLinearWrap  : register(s2);

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
	float4x4 gInvWorld;
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
	float3 PosL     : POSITION;
    float3 NormalL  : NORMAL;
	float2 TexC     : TEXCOORD;
	float3 TangentL : TENGENT;
};

struct VertexOut
{
	float4   PosH    : SV_POSITION;
    float3   PosW    : POSITION;
	float2   TexC    : TEXCOORD;
    float3   T       : TENGENT;
	float3   B       : BINORMAL;
    float3   N       : NORMAL;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
	
    // Transform to world space.
    float4 posW = mul(gWorld, float4(vin.PosL, 1.0f));
    vout.PosW = posW.xyz;

	float3x3 normalMatrix = transpose((float3x3)gInvWorld);
    vout.N = normalize(mul(normalMatrix, vin.NormalL));
	float4 TangentH = mul(gWorld, float4(vin.TangentL, 1));
	float3 TangentW = TangentH.xyz / TangentH.w;
    vout.T = normalize(TangentW - dot(TangentW, vout.N) * vout.N);
	vout.B = cross(vout.N, vout.T);

    // Transform to homogeneous clip space.
    vout.PosH = mul(gViewProj, posW);
	
	// Output vertex attributes for interpolation across triangle.
    vout.TexC = vin.TexC;

    return vout;
}

struct PixelOut {
	// 0  [albedo roughness]
	// 1  [  N    metalness]
	// 2  [ pos     mat ID ]
	float4 gbuffer0    : SV_Target0;
	float4 gbuffer1    : SV_Target1;
	float4 gbuffer2    : SV_Target2;
};

PixelOut PS(VertexOut pin)
{
	PixelOut pout;
	
    float3 albedo    = gAlbedoFactor    * gAlbedoMap   .Sample(gSamplerLinearWrap, pin.TexC).xyz;
    float  roughness = gRoughnessFactor * gRoughnessMap.Sample(gSamplerLinearWrap, pin.TexC).x;
    float  metalness = gMetalnessFactor * gMetalnessMap.Sample(gSamplerLinearWrap, pin.TexC).x;
	
	float3 NormalM = gNormalMap.Sample(gSamplerLinearWrap, pin.TexC).xyz;
	float3 NormalS = normalize(2 * NormalM - 1);
	float3 N = normalize(NormalS.x * pin.T + NormalS.y * pin.B + NormalS.z * pin.N);
	
	pout.gbuffer0 = float4(albedo, roughness);
	pout.gbuffer1 = float4(N, metalness);
	pout.gbuffer2 = float4(pin.PosW, 0.0f);
	
	return pout;
}
