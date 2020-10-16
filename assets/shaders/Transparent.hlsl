#include "StdPipeline.hlsli"
#include "PBR.hlsli"

Texture2D gAlbedoMap    : register(t0);
Texture2D gRoughnessMap : register(t1);
Texture2D gMetalnessMap : register(t2);
Texture2D gNormalMap    : register(t3);
STD_PIPELINE_SR3_IBL(4); // cover 3 register

STD_PIPELINE_CB_PER_OBJECT(0);

cbuffer cbPerMaterial : register(b1)
{
	float3 gAlbedoFactor;
    float  gRoughnessFactor;
    float  gMetalnessFactor;
};

STD_PIPELINE_CB_PER_CAMERA(2);
STD_PIPELINE_CB_LIGHT_ARRAY(3);

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
	float4 color : SV_Target0;
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
	
	float3 posW = pin.PosW;
	
	// -------------
	
	float3 Lo = float3(0.0f, 0.0f, 0.0f);
	
	float alpha = roughness * roughness;
	float3 V = normalize(gEyePosW - posW);
	float3 F0 = MetalWorkflow_F0(albedo, metalness);
	
	uint offset = 0u;
	uint i;
	for(i = offset; i < offset + gDirectionalLightNum; i++) {
		float3 L = -gLights[i].dir;
		float3 H = normalize(L + V);
				
		float cos_theta = max(0, dot(N, L));
		
		float3 fr = Fresnel(F0, cos_theta);
		float D = GGX_D(alpha, N, H);
		float G = GGX_G(alpha, L, V, N);
				
		float3 diffuse = (1 - fr) * (1 - metalness) * albedo / PI;
				
		float3 specular = fr * D * G / (4 * max(dot(L, N)*dot(V, N), EPSILON));
		
		float3 brdf = diffuse + specular;
		Lo += brdf * gLights[i].color * cos_theta;
	}
	offset += gDirectionalLightNum;
	for(i = offset; i < offset + gPointLightNum; i++) {
		float3 pixelToLight = gLights[i].position - posW;
		float dist2 = dot(pixelToLight, pixelToLight);
		float dist = sqrt(dist2);
		float3 L = pixelToLight / dist;
		float3 H = normalize(L + V);
		
		float cos_theta = max(0, dot(N, L));
		
		float3 fr = Fresnel(F0, cos_theta);
		float D = GGX_D(alpha, N, H);
		float G = GGX_G(alpha, L, V, N);
				
		float3 diffuse = (1 - fr) * (1 - metalness) * albedo / PI;
				
		float3 specular = fr * D * G / (4 * max(dot(L, N)*dot(V, N), EPSILON));
		
		float3 brdf = diffuse + specular;
		float3 color = gLights[i].color * Fwin(dist, gLights[i].range) / (max(0.0001, dist2) * 4 * PI);
		Lo += brdf * color * cos_theta;
	}
	offset += gPointLightNum;
	for(i = offset; i < offset + gSpotLightNum; i++) {
		float3 pixelToLight = gLights[i].position - posW;
		float dist2 = dot(pixelToLight, pixelToLight);
		float dist = sqrt(dist2);
		float3 L = pixelToLight / dist;
		float3 H = normalize(L + V);
		
		float cos_theta = max(0, dot(N, L));
		
		float3 fr = Fresnel(F0, cos_theta);
		float D = GGX_D(alpha, N, H);
		float G = GGX_G(alpha, L, V, N);
				
		float3 diffuse = (1 - fr) * (1 - metalness) * albedo / PI;
				
		float3 specular = fr * D * G / (4 * max(dot(L, N)*dot(V, N), EPSILON));
		
		float3 brdf = diffuse + specular;
		float3 color = gLights[i].color
		    * Fwin(dist, gLights[i].range)
			* DirFwin(-L, gLights[i].dir, gLights[i].f0, gLights[i].f1)
			/ (max(0.0001, dist2));
		Lo += brdf * color * cos_theta;
	}
	
	float3 FrR = SchlickFrR(V, N, F0, roughness);
	float3 kS = FrR;
	float3 kD = (1 - metalness) * (float3(1, 1, 1) - kS);
	
	float3 irradiance = STD_PIPELINE_SAMPLE_IRRADIANCE(N);
	float3 diffuse = irradiance * albedo;
	
	float3 R = reflect(-V, N);
	float3 prefilterColor = STD_PIPELINE_SAMPLE_PREFILTER(R, roughness);
	float NdotV = saturate(dot(N, V));
	float2 scale_bias = STD_PIPELINE_SAMPLE_BRDFLUT(NdotV, roughness);
	float3 specular = prefilterColor * (F0 * scale_bias.x + scale_bias.y);
	
	Lo += kD * diffuse + specular;
	
	pout.color = float4(Lo, 0.5f);
	
	return pout;
}
