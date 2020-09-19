// 1. directional light
// color
// dir

// 2. point light
// color
// position
// range

// 3. spot light
// color
// position
// dir
// range
// cosHalfInnerSpotAngle : f0
// cosHalfOuterSpotAngle : f1

// 4. rect light
// color
// position
// dir
// horizontal
// range
// width  : f0
// height : f1

// 5. disk light
// color
// position
// dir
// range
// radius : f0

struct Light {
	float3 color;
	float range;
	float3 dir;
	float f0;
	float3 position;
	float f1;
	float3 horizontal;
	float f2;
};

#define PI 3.1415926
#define EPSILON 0.000001
#define MAX_DETIAL_MIP_LEVEL 4

Texture2D    gbuffer0       : register(t0);
Texture2D    gbuffer1       : register(t1);
Texture2D    gbuffer2       : register(t2);
Texture2D    gDepthStencil  : register(t3);
TextureCube  gIrradianceMap : register(t4);
TextureCube  gPreFilterMap  : register(t5);
Texture2D    gBRDFLUT       : register(t6);

SamplerState gSamplerPointWrap   : register(s0);
SamplerState gSamplerLinearWrap  : register(s2);

// Constant data that varies per frame.
cbuffer cbLightArray : register(b0)
{
	uint gDirectionalLightNum;
	uint gPointLightNum;
	uint gSpotLightNum;
	uint gRectLightNum;
	uint gDiskLightNum;
	uint _g_cbLightArray_pad0;
	uint _g_cbLightArray_pad1;
	uint _g_cbLightArray_pad2;
	Light gLights[16];
};

cbuffer cbPerCamera: register(b1)
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

float3 MetalWorkflow_F0(float3 albedo, float metalness) {
	float reflectance = 0.04;
	return lerp(float3(reflectance, reflectance, reflectance), albedo, metalness);
}

float3 Fresnel(float3 F0, float cos_theta) {
	float x = 1 - cos_theta;
	float x2 = x * x;
	float x5 = x2 * x2 * x;
	return F0 + (1-F0)*x5;
}

float Pow2(float x) {
	return x * x;
}
float Pow4(float x) {
	return Pow2(Pow2(x));
}
float Pow5(float x) {
	return Pow4(x) * x;
}

float3 SchlickFrR(float3 V, float3 N, float3 F0, float roughness) {
	float cosTheta = max(dot(V, N), 0);
	float x = 1.0 - roughness;
    return F0 + (max(float3(x,x,x), F0) - F0) * Pow5(1.0 - cosTheta);
}

float GGX_G(float alpha, float3 L, float3 V, float3 N) {
	float alpha2 = alpha * alpha;
	
	float cos_sthetai = dot(L, N);
	float cos_sthetao = dot(V, N);
	
	float tan2_sthetai = 1 / (cos_sthetai * cos_sthetai) - 1;
	float tan2_sthetao = 1 / (cos_sthetao * cos_sthetao) - 1;
	
	return step(0, cos_sthetai) * step(0, cos_sthetao) * 2 / (sqrt(1 + alpha2*tan2_sthetai) + sqrt(1 + alpha2*tan2_sthetao));
}

float GGX_D(float alpha, float3 N, float3 H) {
	float alpha2 = alpha * alpha;
	float cos_stheta = dot(H, N);
	float x = 1 + (alpha2 - 1) * cos_stheta * cos_stheta;
	float denominator = PI * x * x;
	return step(0, cos_stheta) * alpha2 / denominator;
}

float Fwin(float d, float range) {
	float falloff = max(0, 1 - Pow4(d / range));
	return falloff * falloff;
}

float DirFwin(float3 x, float3 dir, float cosHalfInnerSpotAngle, float cosHalfOuterSpotAngle) {
	float cosTheta = dot(x, dir);
	if(cosTheta < cosHalfOuterSpotAngle)
		return 0;
	else if(cosTheta > cosHalfInnerSpotAngle)
		return 1;
	else{
		float t = (cosTheta - cosHalfOuterSpotAngle) /
			(cosHalfInnerSpotAngle - cosHalfOuterSpotAngle);
		return Pow4(t);
	}
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 data0 = gbuffer0.Sample(gSamplerPointWrap, pin.TexC);
    float4 data1 = gbuffer1.Sample(gSamplerPointWrap, pin.TexC);
    //float4 data2 = gbuffer2.Sample(gSamplerPointWrap, pin.TexC);
    
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
	
	float3 FrR = SchlickFrR(V, N, F0, roughness);
	float3 kS = FrR;
	float3 kD = (1 - metalness) * (float3(1, 1, 1) - kS);
	
	float3 R = reflect(-V, N);
	float3 prefilterColor = gPreFilterMap.SampleLevel(gSamplerLinearWrap, R, roughness * MAX_DETIAL_MIP_LEVEL).rgb;
	float NdotV = saturate(dot(N, V));
	float2 scale_bias = gBRDFLUT.Sample(gSamplerLinearWrap, float2(NdotV, roughness)).rg;
	float3 specular = prefilterColor * (F0 * scale_bias.x + scale_bias.y);
	
	float3 irradiance = gIrradianceMap.Sample(gSamplerLinearWrap, N).rgb;
	float3 diffuse = irradiance * albedo;
	Lo += kD * diffuse + specular;
	
    return float4(Lo, 1.0f);
}
