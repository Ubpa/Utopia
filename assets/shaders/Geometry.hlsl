#include "StdPipeline.hlsli"

uint DirToOct(float3 normal) {
	float2 p = normal.xy * (1.0 / dot(abs(normal), 1.0.xxx));
	float2 e = normal.z > 0.0 ? p : (1.0 - abs(p.yx)) * (step(0.0, p)*2.0 - (float2)(1.0));
	return (asuint(f32tof16(e.y)) << 16) + (asuint(f32tof16(e.x)));
}

Texture2D gAlbedoMap    : register(t0);
Texture2D gRoughnessMap : register(t1);
Texture2D gMetalnessMap : register(t2);
Texture2D gNormalMap    : register(t3);

STD_PIPELINE_CB_PER_OBJECT(0);

cbuffer cbPerMaterial : register(b1)
{
    float3 gAlbedoFactor;
    float  gRoughnessFactor;
    float  gMetalnessFactor;
};

STD_PIPELINE_CB_PER_CAMERA(2);

struct VertexIn
{
    float3 PosL     : POSITION;
    float3 NormalL  : NORMAL;
    float2 TexC     : TEXCOORD;
    float3 TangentL : TENGENT;
};

struct VertexOut
{
    float4 PosH     : SV_POSITION;
    float2 TexC     : TEXCOORD;
    float3 T        : TENGENT;
    float3 B        : BINORMAL;
    float3 N        : NORMAL0;
    float3 NormalL  : NORMAL1;
    float4 CurrPosH : POSITION0; // clip space, unjittered
    float4 PrevPosH : POSITION1; // clip space, jittered
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut)0.0f;
    
    // Transform to world space.
    float4 posW = mul(gWorld, float4(vin.PosL, 1.0f));

    vout.NormalL = vin.NormalL;
    float3x3 normalMatrix = transpose((float3x3)gInvWorld);
    vout.N = normalize(mul(normalMatrix, vin.NormalL));
    float4 TangentH = mul(gWorld, float4(vin.TangentL, 1));
    float3 TangentW = TangentH.xyz / TangentH.w;
    vout.T = normalize(TangentW - dot(TangentW, vout.N) * vout.N);
    vout.B = cross(vout.N, vout.T);

    // Transform to homogeneous clip space.
    vout.PosH = mul(gViewProj, posW);
    
    vout.CurrPosH = mul(gViewProj, posW); // TODO: unjittered
    vout.PrevPosH = mul(gPrevViewProj, posW);
    
    // Output vertex attributes for interpolation across triangle.
    vout.TexC = vin.TexC;

    return vout;
}

struct PixelOut {
    //    [rgb    a        ]
    // 0  [albedo roughness]
    // 1  [N      metalness]
    // 2  [0      matID    ]
    float4 gbuffer0 : SV_Target0;
    float4 gbuffer1 : SV_Target1;
    float4 gbuffer2 : SV_Target2;
#ifdef UBPA_STD_RAY_TRACING
    float4 linearZ  : SV_Target3; // x: Z, y: maxChangeZ, z: prevZ, w: objN
    float4 motion   : SV_Target4; // xy: motion (prev -> curr), z: 0, w: normFWidth
#endif // UBPA_STD_RAY_TRACING
};

PixelOut PS(VertexOut pin)
{
    PixelOut pout;
    
    float3 albedo    = gAlbedoFactor    * gAlbedoMap   .Sample(gSamplerLinearWrap, pin.TexC).xyz;
    float  roughness = gRoughnessFactor * gRoughnessMap.Sample(gSamplerPointWrap, pin.TexC).x;
    float  metalness = gMetalnessFactor * gMetalnessMap.Sample(gSamplerLinearWrap, pin.TexC).x;
    
    float3 NormalM = gNormalMap.Sample(gSamplerLinearWrap, pin.TexC).xyz;
    float3 NormalS = normalize(2 * NormalM - 1);
    float3 N = normalize(NormalS.x * pin.T + NormalS.y * pin.B + NormalS.z * pin.N);
    float Z = pin.PosH.z * pin.PosH.w; // SV_POSITION'xyz will be divided by SV_POSITION.w, so we multiply back
    float maxChangeZ = max(abs(ddx(Z)), abs(ddy(Z)));
    float prevZ = pin.PrevPosH.z;
    float4 currPos = pin.CurrPosH;
    currPos = currPos / currPos.w;
    currPos.xy = currPos.xy / float2(2.0f, -2.0f) + float2(0.5f, 0.5f);//negate Y because world coord and tex coord have different Y axis.
    float2 currTexc = currPos.xy;
    float4 prevPos = pin.PrevPosH;
    prevPos = prevPos / prevPos.w;
    prevPos.xy = prevPos.xy / float2(2.0f, -2.0f) + float2(0.5f, 0.5f);//negate Y because world coord and tex coord have different Y axis.
    float2 prevTexc = prevPos.xy;
    float normFWidth = fwidth(length(pin.N));

    pout.gbuffer0 = float4(albedo , roughness);
    pout.gbuffer1 = float4(N      , metalness);
    pout.gbuffer2 = float4(0, 0, 0, 1        );

#ifdef UBPA_STD_RAY_TRACING
    pout.linearZ = float4(Z, maxChangeZ, prevZ, asfloat(DirToOct(pin.NormalL)));
    pout.motion = float4(currTexc - prevTexc, 0, 1/*normFWidth*/);
#endif // UBPA_STD_RAY_TRACING

    return pout;
}
