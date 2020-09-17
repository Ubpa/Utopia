struct DirectionalLight {
	float3 L;
	float _pad0;
	float3 dir;
	float _pad1;
};

#define PI 3.1415926
#define EPSILON 0.000001

Texture2D    gbuffer0       : register(t0);
Texture2D    gbuffer1       : register(t1);
Texture2D    gbuffer2       : register(t2);
TextureCube  gIrradianceMap : register(t3);

SamplerState gSamLinear  : register(s0);

// Constant data that varies per frame.
cbuffer cbLights : register(b0)
{
	uint gDirectionalLightNum;
	uint _g_cbLights_pad0;
	uint _g_cbLights_pad1;
	uint _g_cbLights_pad2;
	DirectionalLight gDirectionalLights[4];
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
    vout.PosH = float4(2.0f*vout.TexC.x - 1.0f, 1.0f - 2.0f*vout.TexC.y, 0.0f, 1.0f);

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

float Pow5(float x) {
	float x2 = x * x;
	return x2 * x2 * x;
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

float4 PS(VertexOut pin) : SV_Target
{
    float4 data0 = gbuffer0.Sample(gSamLinear, pin.TexC);
    float4 data1 = gbuffer1.Sample(gSamLinear, pin.TexC);
    float4 data2 = gbuffer2.Sample(gSamLinear, pin.TexC);
	
	float3 albedo = data0.xyz;
	float roughness = data0.w;
	
	float3 N = data1.xyz;
	float metalness = data1.w;
	
	float3 posW = data2.xyz;
	
	// -------------
	
	float3 Lo = float3(0.0f, 0.0f, 0.0f);
	
	float alpha = roughness * roughness;
	float3 V = normalize(gEyePosW - posW);
	float3 F0 = MetalWorkflow_F0(albedo, metalness);
	
	// dir light
	for(uint i = 0u; i < gDirectionalLightNum; i++) {
		float3 L = -gDirectionalLights[i].dir;
		float3 H = normalize(L + V);
				
		float cos_theta = dot(N, L);
		
		float3 fr = Fresnel(F0, cos_theta);
		float D = GGX_D(alpha, N, H);
		float G = GGX_G(alpha, L, V, N);
				
		float3 diffuse = (1 - fr) * (1 - metalness) * albedo / PI;
				
		float3 specular = fr * D * G / (4 * max(dot(L, N)*dot(V, N), EPSILON));
		
		float3 brdf = diffuse + specular;
		//Lo += brdf * gDirectionalLights[i].L * max(cos_theta, 0);
	}
	
	// ambient
	float3 FrR = SchlickFrR(V, N, F0, roughness);
	float3 kS = FrR;
	float3 kD = (1 - metalness) * (float3(1, 1, 1) - kS);
	
	float3 irradiance = gIrradianceMap.Sample(gSamLinear, N).rgb;
	float3 diffuse = irradiance * albedo;
	Lo += diffuse;
	
    return float4(Lo, 1.0f);
}
