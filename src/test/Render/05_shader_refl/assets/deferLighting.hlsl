#define STD_PIPELINE_ENABLE_LTC
#include "StdPipeline.hlsli"

#include "PBR.hlsli"

Texture2D    gbuffer0       : register(t0);
Texture2D    gbuffer1       : register(t1);
Texture2D    gbuffer2       : register(t2);
Texture2D    gDepthStencil  : register(t3);
STD_PIPELINE_SR3_IBL(4);
STD_PIPELINE_SR2_LTC(7);

STD_PIPELINE_CB_LIGHT_ARRAY(0);

STD_PIPELINE_CB_PER_CAMERA(1);

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float2 TexC    : TEXCOORD;
};

static const float2 gTexCoords[6] =
{
    float2(0.0f, 1.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 1.0f)
};

VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vout;

    vout.TexC = gTexCoords[vid];

    // Quad covering screen in NDC space.
    vout.PosH = float4(2.0f*vout.TexC.x - 1.0f, 1.0f - 2.0f*vout.TexC.y, 1.0f, 1.0f);

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 data0 = gbuffer0.Sample(gSamplerPointWrap, pin.TexC);
    float4 data1 = gbuffer1.Sample(gSamplerPointWrap, pin.TexC);
    float4 data2 = gbuffer2.Sample(gSamplerPointWrap, pin.TexC);
    
	float depth = gDepthStencil.Sample(gSamplerPointWrap, pin.TexC).r;
	float4 posHC = float4(
		2 * pin.TexC.x - 1,
		1 - 2 * pin.TexC.y,
		depth,
		1.f
	);
	float4 posHW = mul(gInvViewProj, posHC);
	float3 posW = posHW.xyz / posHW.w;
	
	float3 albedo = data0.xyz;
	float3 emission = data2.xyz;
	float roughness = data0.w;
	
	float3 N = data1.xyz;
	float metalness = data1.w;
	
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
	offset += gSpotLightNum;
	for (i = offset; i < offset + gRectLightNum; i++) {
		float3 up = cross(gLights[i].dir, gLights[i].horizontal);

		float3 halfWidthVec = gLights[i].horizontal * gLights[i].f0 / 2;
		float3 halfHeightVec = up * gLights[i].f1 / 2;

		float3 p0 = gLights[i].position - halfHeightVec - halfWidthVec;
		float3 p1 = gLights[i].position - halfHeightVec + halfWidthVec;
		float3 p2 = gLights[i].position + halfHeightVec + halfWidthVec;
		float3 p3 = gLights[i].position + halfHeightVec - halfWidthVec;

		float3 spec = LTC_Rect_Spec(N, V, posW, F0, roughness, p0, p1, p2, p3);
		float3 diffuse = (1 - metalness) * albedo * (1 - F0)
			* LTC_Rect_Diffuse(N, V, posW, roughness, p0, p1, p2, p3);

		Lo += (spec + diffuse) / (2 * PI) * gLights[i].color;
	}
	offset += gRectLightNum;
	for (i = offset; i < offset + gDiskLightNum; i++) {
		float3 up = cross(gLights[i].dir, gLights[i].horizontal);

		float3 halfWidthVec = gLights[i].horizontal * gLights[i].f0 / 2;
		float3 halfHeightVec = up * gLights[i].f1 / 2;

		float3 p0 = gLights[i].position - halfHeightVec - halfWidthVec;
		float3 p1 = gLights[i].position - halfHeightVec + halfWidthVec;
		float3 p2 = gLights[i].position + halfHeightVec + halfWidthVec;
		//float3 p3 = gLights[i].position + halfHeightVec - halfWidthVec;

		float3 spec = LTC_Disk_Spec(N, V, posW, F0, roughness, p0, p1, p2);
		float3 diffuse = (1 - metalness) * albedo * (1 - F0)
			* LTC_Disk_Diffuse(N, V, posW, roughness, p0, p1, p2);

		Lo += (spec + diffuse) / (2 * PI) * gLights[i].color;
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

	Lo += emission;
	
    return float4(Lo, 1.0f);
}
