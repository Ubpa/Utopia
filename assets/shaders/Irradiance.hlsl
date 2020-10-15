static const float PI = 3.14159265359;

TextureCube  gCubeMap           : register(t0);
SamplerState gSamplerLinearWrap : register(s2);

struct VertexOut {
    float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
};

cbuffer cbPositionLs : register(b0)
{
	float4 gPositionL4x;
	float4 gPositionL4y;
	float4 gPositionL4z;
}

static const float2 gPositionHs[6] = {
	float2( 1,-1),
	float2( 1, 1),
	float2(-1,-1),
	float2(-1, 1),
	float2(-1,-1),
	float2( 1, 1)
};

static const uint gIndexMap[6] = {
	1, 2, 0,
	3, 0, 2
};

float _RadicalInverse_VdC(uint bits) {
	// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
	// efficient VanDerCorpus calculation.

	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint N) {
	return float2(float(i) / float(N), _RadicalInverse_VdC(i));
}

float2 UniformOnDisk(float Xi) {
	float theta = 2 * PI * Xi;
	return float2(cos(theta), sin(theta));
}

float2 UniformInDisk(float2 Xi) {
	// Map uniform random numbers to $[-1,1]^2$
	float2 uOffset = 2.f * Xi - float2(1,1);

	// Handle degeneracy at the origin
	//if (uOffset.x == 0 && uOffset.y == 0)
	//	return float2(0, 0);

	// Apply concentric mapping to point
	float theta, r;
	if (abs(uOffset.x) > abs(uOffset.y)) {
		r = uOffset.x;
		theta = PI / 4.f * (uOffset.y / uOffset.x);
	}
	else {
		r = uOffset.y;
		theta = PI / 2.f - PI / 4.f * (uOffset.x / uOffset.y);
	}
	return r * float2(cos(theta), sin(theta));
}

float3 CosOnHalfSphere(float2 Xi) {
	float2 pInDisk = UniformInDisk(Xi);
	float z = sqrt(1 - dot(pInDisk, pInDisk));
	//float r = sqrt(Xi.x);
	//float2 pInDisk = r * UniformOnDisk(Xi.y);
	//float z = sqrt(1 - Xi.x);
	return float3(pInDisk, z);
}

VertexOut VS(uint vid : SV_VertexID) {
	VertexOut vout;
    vout.PosH = float4(gPositionHs[vid], 0, 1);
	uint idx = gIndexMap[vid];
	vout.PosL = float3(gPositionL4x[idx], gPositionL4y[idx], gPositionL4z[idx]);
	return vout;
}

float4 PS(VertexOut pin) : SV_Target {
    float3 N = normalize(pin.PosL);
    float3 up = float3(0.0, 1.0, 0.0);
    float3 right = cross(up, N);
    up = cross(N, right);

    float3 irradiance = float3(0, 0, 0);
    
    const uint SAMPLE_NUM = 256u;
	
	for(uint i = 0u; i < SAMPLE_NUM; i++) {
		float2 Xi = Hammersley(i, SAMPLE_NUM);
		
		float3 sL = CosOnHalfSphere(Xi);
		float3 L = sL.x * right + sL.y * up + sL.z * N;
		
		irradiance += gCubeMap.Sample(gSamplerLinearWrap, L).xyz;
	}
	irradiance /= float(SAMPLE_NUM);
	
	/*
    const uint SAMPLE_NUM = 32u;
	float delta = 1.0f / float(SAMPLE_NUM);
	
	for(float x=0.f;x<=1.0f;x+=delta) {
		for(float y=0.f;y<=1.0f;y+=delta){
			float2 Xi = float2(x,y);
			float3 sL = CosOnHalfSphere(Xi);
			float3 L = sL.x * right + sL.y * up + sL.z * N;
			
			irradiance += gCubeMap.Sample(gSamplerLinearWrap, L).xyz;
		}
	}
	irradiance /= float(SAMPLE_NUM) * float(SAMPLE_NUM);
	*/
	
	/*
    const uint SAMPLE_NUM = 32u;
	float delta = 1.0f / float(SAMPLE_NUM);
	
	for(float x=0.f;x<=1.0f;x+=delta) {
		for(float y=0.f;y<=1.0f;y+=delta){
			float phi = 2 * PI * x;
			float theta = 0.5 * PI * y;
			float3 sL = float3(sin(theta)*cos(phi),sin(theta)*sin(phi),cos(theta));
			float3 L = sL.x * right + sL.y * up + sL.z * N;
			
			irradiance += gCubeMap.Sample(gSamplerLinearWrap, L).xyz * cos(theta)*sin(theta);
		}
	}
	
    irradiance = irradiance * PI / (float(SAMPLE_NUM) * float(SAMPLE_NUM));
	*/
    
    return float4(irradiance, 1);
}
