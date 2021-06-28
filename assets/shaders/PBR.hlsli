#ifndef PBR_HLSLI
#define PBR_HLSLI

#include "Constants.hlsli"

float3 MetalWorkflow_F0(float3 albedo, float metalness) {
	float reflectance = 0.04;
	return lerp(float3(reflectance, reflectance, reflectance), albedo, metalness);
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

float3 Fresnel(float3 F0, float cos_theta) {
    return F0 + (1-F0) * Pow5(1 - cos_theta);
}

float3 SchlickFrR(float3 V, float3 N, float3 F0, float roughness) {
	float cosTheta = saturate(dot(V, N));
	float x = 1.0 - roughness;
    return F0 + (max(float3(x,x,x), F0) - F0) * Pow5(1.0 - cosTheta);
}

float GGX_G(float alpha, float NoL, float NoV) {
    float alpha2 = Pow2(alpha);

    //float tan2_sthetai = 1 / Pow2(NoL) - 1;
    //float tan2_sthetao = 1 / Pow2(NoV) - 1;
    
    //return 2 / (sqrt(1 + alpha2*tan2_sthetai) + sqrt(1 + alpha2*tan2_sthetao));
    
    float one_minus_alpha2 = 1 - alpha2;
    return (2 * NoL * NoV) /
        (NoV*sqrt(one_minus_alpha2*Pow2(NoL)+alpha2) + NoL*sqrt(one_minus_alpha2*Pow2(NoV)+alpha2));
}

float GGX_D(float alpha, float NoH) {
    float alpha2 = alpha * alpha;
    float x = 1 + (alpha2 - 1) * NoH * NoH;
    float denominator = PI * x * x;
    return alpha2 / denominator;
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

float ComputeLuminance(float3 color) {
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

float2 UniformInDisk(float2 Xi) {
    // Map uniform random numbers to $[-1,1]^2$
    float2 uOffset = 2.f * Xi - float2(1,1);

    // Handle degeneracy at the origin
    //if (uOffset.x == 0 && uOffset.y == 0)
    //    return float2(0, 0);

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

// surface vector
float3 CosOnHalfSphere(float2 Xi) {
    float2 pInDisk = UniformInDisk(Xi);
    float z = sqrt(1 - dot(pInDisk, pInDisk));
    return float3(pInDisk, z);
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

// ref: Sampling the GGX Distribution of Visible Normals
//      http://jcgt.org/published/0007/04/01/paper.pdf
// ---------------------------------------------------------------
// Input Ve: view direction
// Input alpha: roughness parameters
// Input U1, U2: uniform random numbers
// Output Ne: normal sampled with PDF D_Ve(Ne) = G1(Ve) * max(0, dot(Ve, Ne)) * D(Ne) / Ve.z
float3 SampleGGXVNDF(float3 Ve, float alpha, float U1, float U2)
{
    // Section 3.2: transforming the view direction to the hemisphere configuration
    float3 Vh = normalize(float3(alpha * Ve.x, alpha * Ve.y, Ve.z));
    // Section 4.1: orthonormal basis (with special case if cross product is zero)
    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    float3 T1 = lensq > 0 ? float3(-Vh.y, Vh.x, 0) * rsqrt(lensq) : float3(1,0,0);
    float3 T2 = cross(Vh, T1);
    // Section 4.2: parameterization of the projected area
    float r = sqrt(U1);
    float phi = 2.0 * PI * U2;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5 * (1.0 + Vh.z);
    t2 = (1.0 - s)*sqrt(1.0 - t1*t1) + s*t2;
    // Section 4.3: reprojection onto hemisphere
    float3 Nh = t1*T1 + t2*T2 + sqrt(max(0.0, 1.0 - t1*t1 - t2*t2))*Vh;
    // Section 3.4: transforming the normal back to the ellipsoid configuration
    float3 Ne = normalize(float3(alpha * Nh.x, alpha * Nh.y, max(0.f, Nh.z)));
    return Ne;
}

float G1(float alpha, float NoV) {
    float tan2_sthetao = 1 / Pow2(NoV) - 1;
    return 2 / (1 + sqrt(1 + Pow2(alpha) * tan2_sthetao));
}

float PDF_VNDF(float alpha, float NoV, float NoH, float HoV) {
    return G1(alpha, NoV) * GGX_D(alpha, NoH) / (4 * NoV);
    //float cos2_sthetao = Pow2(HoV);
    //float sin2_sthetao = 1 - cos2_sthetao;
    //float alpha2 = Pow2(alpha);
    //return GGX_D(alpha, NoH) / (2 * (HoV + sqrt(cos2_sthetao + alpha2 * sin2_sthetao)));
}

#endif // PBR_HLSLI
