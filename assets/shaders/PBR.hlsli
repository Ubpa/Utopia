#ifndef PBR_HLSLI
#define PBR_HLSLI

#define PI 3.1415926
#define EPSILON 0.000001

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

#endif // PBR_HLSLI
