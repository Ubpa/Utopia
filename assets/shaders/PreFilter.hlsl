static const float PI = 3.14159265359;

TextureCube  gCubeMap             : register(t0);
SamplerState gSamplerLinearWrap   : register(s2);

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

cbuffer cbMipInfo : register(b1)
{
	float gRoughness;
	float gResolution;
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

// surface vector
float3 SchlickGGX_Sample(float2 Xi, float roughness) {
	float a = roughness * roughness;

	float phi = 2 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	// from spherical coordinates to cartesian coordinates - halfway vector
	float3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	return H;
}

float SchlickGGX_D(float3 norm, float3 h, float roughness){
	float NoH = saturate(dot(norm, h));
	
	float alpha = roughness * roughness;
	
	float alpha2 = alpha * alpha;
	float cos2Theta = NoH * NoH;
	
	float t = (alpha2 - 1) * cos2Theta + 1;
	
	return alpha2 / (PI * t * t);
}

VertexOut VS(uint vid : SV_VertexID) {
	VertexOut vout;
    vout.PosH = float4(gPositionHs[vid], 0, 1);
	uint idx = gIndexMap[vid];
	vout.PosL = float3(gPositionL4x[idx], gPositionL4y[idx], gPositionL4z[idx]);
	return vout;
}

float4 PS(VertexOut pin) : SV_Target {
	float roughness = gRoughness;
	float resolution = gResolution;
	
	// Solid angle associated to a texel of the cubemap
    const float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
	
    float3 N = normalize(pin.PosL);
    float3 up = float3(0.0, 1.0, 0.0);
    float3 right = cross(up, N);
    up = cross(N, right);
    
    // make the simplyfying assumption that V equals R equals the normal 
    float3 R = N;
    float3 V = R;

    const uint SAMPLE_NUM = 256u;
    float3 prefilteredColor = float3(0,0,0);
    float totalWeight = 0.0;
	
	for(uint i = 0u; i < SAMPLE_NUM; i++) {// generates a sample vector that's biased towards the preferred alignment direction (importance sampling).
        float2 Xi = Hammersley(i, SAMPLE_NUM);
        float3 sH = SchlickGGX_Sample(Xi, roughness);
		float3 H = sH.x * right + sH.y * up + sH.z * N;
        float3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = dot(N, L);
        if(NdotL > 0.0) {
            // sample from the environment's mip level based on roughness/pdf
            float D   = SchlickGGX_D(N, H, roughness);
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);
            float pdf = D * NdotH / max(4.0 * HdotV, 0.000001); 
			
			// Solid angle associated to a sample
            float saSample = 1.0 / max(float(SAMPLE_NUM) * pdf, 0.000001);

            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 
            
			// weight by dot(N, L)
            prefilteredColor += gCubeMap.Sample(gSamplerLinearWrap, L, mipLevel).rgb * NdotL;
            totalWeight      += NdotL;
        }
	}
    prefilteredColor = prefilteredColor / totalWeight;
    
    return float4(prefilteredColor, 1);
}
