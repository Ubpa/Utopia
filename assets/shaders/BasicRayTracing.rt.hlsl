// util
static const float PI = 3.1415926;
static const float EPSILON = 0.000001;
float3 MetalWorkflow_F0(float3 albedo, float metalness) {
    float reflectance = 0.04;
    return lerp(float3(reflectance, reflectance, reflectance), albedo, metalness);
}
float Pow2(float x) { return x * x; }
float Pow4(float x) { return Pow2(Pow2(x)); }
float Pow5(float x) { return Pow4(x) * x; }
float3 Fresnel(float3 F0, float cos_theta) {
    return F0 + (1-F0) * Pow5(1 - cos_theta);
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
// Generates a seed for a random number generator from 2 inputs plus a backoff
uint initRand(uint val0, uint val1, uint backoff = 16)
{
    uint v0 = val0, v1 = val1, s0 = 0;

    [unroll]
    for (uint n = 0; n < backoff; n++)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }
    return v0;
}
// Takes our seed, updates it, and returns a pseudorandom float in [0..1]
float nextRand(inout uint s)
{
    s = (1664525u * s + 1013904223u);
    return float(s & 0x00FFFFFF) / float(0x01000000);
}

float Fwin(float d, float range) {
    float falloff = max(0, 1 - Pow4(d / range));
    return falloff * falloff;
}

float DirFwin(float3 x, float3 dir, float cosHalfInnerSpotAngle, float cosHalfOuterSpotAngle) {
    float cosTheta = dot(x, dir);
    if (cosTheta < cosHalfOuterSpotAngle)
        return 0;
    else if (cosTheta > cosHalfInnerSpotAngle)
        return 1;
    else {
        float t = (cosTheta - cosHalfOuterSpotAngle) /
            (cosHalfInnerSpotAngle - cosHalfOuterSpotAngle);
        return Pow4(t);
    }
}
// standard pipeline
#define STD_PIPELINE_CB_PER_CAMERA(x)            \
cbuffer StdPipeline_cbPerCamera : register(b##x) \
{                                                \
    float4x4 gView;                              \
    float4x4 gInvView;                           \
    float4x4 gProj;                              \
    float4x4 gInvProj;                           \
    float4x4 gViewProj;                          \
    float4x4 gInvViewProj;                       \
                                                 \
    float4x4 gPrevViewProj;                      \
                                                 \
    float3 gEyePosW;                             \
    uint gFrameCount;                            \
                                                 \
    float2 gRenderTargetSize;                    \
    float2 gInvRenderTargetSize;                 \
                                                 \
    float gNearZ;                                \
    float gFarZ;                                 \
    float gTotalTime;                            \
    float gDeltaTime;                            \
}
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
// horizontal
// range
// width  : f0
// height : f1

#define STD_PIPELINE_CB_LIGHT_ARRAY(x)            \
struct Light {                                    \
    float3 color;                                 \
    float range;                                  \
    float3 dir;                                   \
    float f0;                                     \
    float3 position;                              \
    float f1;                                     \
    float3 horizontal;                            \
    float f2;                                     \
};                                                \
                                                  \
cbuffer StdPipeline_cbLightArray : register(b##x) \
{                                                 \
    uint gDirectionalLightNum;                    \
    uint gPointLightNum;                          \
    uint gSpotLightNum;                           \
    uint gRectLightNum;                           \
    uint gDiskLightNum;                           \
    uint _g_cbLightArray_pad0;                    \
    uint _g_cbLightArray_pad1;                    \
    uint _g_cbLightArray_pad2;                    \
    Light gLights[16];                            \
}

SamplerState gSamplerPointWrap   : register(s0);
SamplerState gSamplerPointClamp  : register(s1);
SamplerState gSamplerLinearWrap  : register(s2);
SamplerState gSamplerLinearClamp : register(s3);
STD_PIPELINE_CB_PER_CAMERA(0);
STD_PIPELINE_CB_LIGHT_ARRAY(1);

RaytracingAccelerationStructure gRtScene : register(t0); // global resource

// 0  [albedo   roughness]
// 1  [  N      metalness]
// 2  [emission matID    ]
Texture2D<float4> gGBuffer0     : register(t1);
Texture2D<float4> gGBuffer1     : register(t2);
Texture2D<float4> gGBuffer2     : register(t3);
Texture2D<float>  gDepthStencil : register(t4);
Texture2D<float2> StdPipeline_BRDFLUT : register(t5);
#define STD_PIPELINE_SAMPLE_BRDFLUT(NdotV, roughness) \
StdPipeline_BRDFLUT.SampleLevel(gSamplerLinearClamp, float2(NdotV, roughness), 0).rg

RWTexture2D<float4> gOutput  : register(u0); // rt result
RWTexture2D<float4> gOutput2 : register(u1); // rt u result

float ComputeLuminance(float3 color) {
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

#define SHADOW_RAY_INDEX             0
#define INDIRECT_RAY_INDEX           1
#define SHADOW_MISS_SHADER_INDEX     0
#define INDIRECT_MISS_SHADER_INDEX   1
#define PER_INSTANCE_HIT_GROUP_COUNT 1
#define MAX_RAY_DEPTH                5
#define RAY_TMIN 0.05f
#define RAY_TMAX 100000

#if MAX_RAY_DEPTH >= 31
  #error "MAX_RAY_DEPTH < D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH"
#endif

struct ShadowRayPayload
{
    float vis;
};

struct IndirectPayload {
    float3 color;
    uint randSeed;
    uint depth;
};

void EvaluateLight(uint randSeed, uint index, float3 pos, out float3 L, out float dist, out float3 intensity) {
    if(index < gDirectionalLightNum) {
        // dir light
        L = -gLights[index].dir;
        dist = RAY_TMAX;
        intensity = gLights[index].color;
    }
    else if (index < gDirectionalLightNum + gPointLightNum) {
        // point light
        float3 diff = gLights[index].position - pos;
        float dist2 = dot(diff, diff);
        dist = sqrt(dist2);
        L = diff / dist;
        intensity = gLights[index].color / (4 * PI * dist2)
            * Fwin(dist, gLights[index].range);
    }
    else if (index < gDirectionalLightNum + gPointLightNum + gSpotLightNum) {
        // spot light
        float3 diff = gLights[index].position - pos;
        float dist2 = dot(diff, diff);
        dist = sqrt(dist2);
        L = diff / dist;
        intensity = gLights[index].color / (4 * PI * dist2)
            * Fwin(dist, gLights[index].range)
            * DirFwin(-L, gLights[index].dir, gLights[index].f0, gLights[index].f1);
    }
    else if (index < gDirectionalLightNum + gPointLightNum + gSpotLightNum + gRectLightNum) {
        // rect light
        float2 Xi = float2(nextRand(randSeed), nextRand(randSeed));

        float3 up = cross(gLights[index].dir, gLights[index].horizontal);

        float3 sample_pos = gLights[index].position
            + lerp(-0.5f,0.5f,Xi.x) * gLights[index].horizontal * gLights[index].f0 /*width*/
            + lerp(-0.5f,0.5f,Xi.y) * up * gLights[index].f1 /*height*/;

        float3 diff = sample_pos - pos;
        float dist2 = dot(diff, diff);
        dist = sqrt(dist2);
        L = diff / dist;
        intensity = gLights[index].color / (2 * PI * dist2)
            * max(0, dot(-L, gLights[index].dir));
    }
    else {
        // TODO: other light
        L = float3(1,0,0);
        dist = 1;
        intensity = float3(0,0,0);
    }
}

float ShadowVisibility(float3 pos, float3 dir, float dist) {
    RayDesc ray;
    ray.Origin = pos;
    ray.Direction = dir;
    ray.TMin = RAY_TMIN;
    ray.TMax = dist;

    ShadowRayPayload payload;
    payload.vis = 0.f;

    // Query if anything is between the current point and the light
    TraceRay(gRtScene,
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
        0xFF, SHADOW_RAY_INDEX, 0, SHADOW_MISS_SHADER_INDEX, ray, payload);

    // Return our ray payload (which is 1 for visible, 0 for occluded)
    return payload.vis;
}

[shader("raygeneration")]
void RayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();
    uint randSeed = initRand(launchIndex.x + launchIndex.y * launchDim.x, (int)gFrameCount, 16);

    float4 data0 = gGBuffer0[launchIndex.xy];
    float4 data1 = gGBuffer1[launchIndex.xy];
    float4 data2 = gGBuffer2[launchIndex.xy];
    float depth = gDepthStencil[launchIndex.xy];

    float2 crd = float2(launchIndex.xy);
    float2 dims = float2(launchDim.xy);
    float2 texc = crd / dims;

    float4 posHC = float4(
        2 * texc.x - 1,
        1 - 2 * texc.y,
        depth,
        1.f
    );
    float4 posHW = mul(gInvViewProj, posHC);
    float3 posW = posHW.xyz / posHW.w;
    
    float3 albedo = data0.xyz;
    float3 emission = data2.xyz;
    float roughness = lerp(0.01,1,data0.w);
    float matID = data2.w;
    if (matID == 0) {
        gOutput[launchIndex.xy] = float4(0.f,0.f,0.f,1.f);
        gOutput2[launchIndex.xy] = float4(0.f,0.f,0.f,1.f);
        return;
    }
    
    float3 N = data1.xyz;
    float metalness = data1.w;

    float alpha = roughness * roughness;
    float3 V = normalize(gEyePosW - posW);
    float3 F0 = MetalWorkflow_F0(albedo, metalness);
    float3 up = float3(0.0, 1.0, 0.0);
    float3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));
    float NdotV = max(dot(N, V), EPSILON);

    uint Num = 1;
    float3 sumcolor = float3(0,0,0);
    float3 sumbrdf = float3(0,0,0);
    for(uint i = 0u; i < Num; i++) {
        ////////////
        // Direct //
        ////////////

        uint lightCnt = gDirectionalLightNum + gPointLightNum + gSpotLightNum + gRectLightNum;
        if(lightCnt > 0) {
            uint lightIdx = min(uint(nextRand(randSeed) * lightCnt), lightCnt - 1);
            float3 L;
            float dist;
            float3 intensity;
            EvaluateLight(randSeed, lightIdx, posW, L, dist, intensity);

            float3 H = normalize(L + V);

            float NdotH = dot(N, H);
            float NdotL = dot(N, L);
            float HdotV = dot(H, V);
            float HdotL = dot(H, L); // == HdotV
            if(NdotH>0&&NdotV>0&&NdotL>0&&HdotV>0&&HdotL>0) {
                float visibility = ShadowVisibility(posW, L, dist);
                float shadowMult = lightCnt * visibility;

                float D = GGX_D(alpha, NdotH);
                float G = GGX_G(alpha, NdotL, NdotV);
                float3 fr = Fresnel(F0, HdotL);

                float3 diffuse = (1-metalness) * albedo * NdotL / PI;
                float3 specular = fr * D * G / (4 * NdotV);

                float3 brdf = diffuse + specular;
                float3 color = shadowMult * brdf * intensity;
                bool colorNan = any(isnan(color));
                if(!colorNan)
                    sumcolor += color;
                sumbrdf += 0; // brdf / pdf, the pdf is delta (a max value), so brdf is 0
            }
        }

        //////////////
        // Indirect //
        //////////////

        float expect_diff = (1-metalness) * ComputeLuminance(albedo);
        float expect_spec = ComputeLuminance(F0);
        float p_diff = expect_diff / (expect_diff + expect_spec);
        float p_spec = 1 - p_diff;
        float2 Xi = float2(nextRand(randSeed), nextRand(randSeed));
        float3 L; // to light
        float3 H; // half vector
        bool chooseDiffuse = nextRand(randSeed) < p_diff;
        if(chooseDiffuse) { // sample diff
            float3 sL = CosOnHalfSphere(Xi);
            L = normalize(sL.x * right + sL.y * up + sL.z * N); // normalized
            H = normalize(L + V);
        }
        else { // sample spec
            //float3 sH = SchlickGGX_Sample(Xi, roughness);
            float Vx = dot(V, right);
            float Vy = dot(V, up);
            float Vz = dot(V, N);
            float3 sV = float3(Vx, Vy, Vz);
            float3 sH = SampleGGXVNDF(sV, alpha, Xi.x, Xi.y);
            H = normalize(sH.x * right + sH.y * up + sH.z * N); // normalized
            L = reflect(-V, H); // to light
        }
        float NdotH = dot(N, H);
        float NdotL = dot(N, L);
        float HdotV = dot(H, V);
        float HdotL = dot(H, L); // == HdotV
        if(NdotH<=0||NdotV<=0||NdotL<=0||HdotV<=0||HdotL<=0)
            continue;
        
        float D = GGX_D(alpha, NdotH);
        float G = GGX_G(alpha, NdotL, NdotV);
        float3 fr = Fresnel(F0, HdotL);

        float pdf_diff = NdotL / PI;
        float pdf_spec = PDF_VNDF(alpha, NdotV, NdotH, HdotV);

        float pdf = p_diff * pdf_diff + p_spec * pdf_spec;
        //float pdf = chooseDiffuse ? pdf_diff / p_diff : pdf_spec / p_spec;

        RayDesc ray;
        ray.Origin = posW;
        ray.Direction = L;

        ray.TMin = RAY_TMIN;
        ray.TMax = RAY_TMAX;

        IndirectPayload payload;
        payload.color = float3(0.f, 0.f, 0.f);
        payload.randSeed = randSeed;
        payload.depth = 1;
        TraceRay(
            gRtScene,
            0 /*flags*/,
            0xFF,
            INDIRECT_RAY_INDEX,
            PER_INSTANCE_HIT_GROUP_COUNT,
            INDIRECT_MISS_SHADER_INDEX,
            ray,
            payload
        );

        // get color
        
        //float3 diffuse = max(5e-3f, (1-metalness) * albedo); // brdf * NoL / pdf : ((1-m)albedo NoL / PI) / (NoL / PI)
        //float3 specular = Fresnel(F0, HdotV) * GGX_G(alpha, NdotV, NdotL) / G1(alpha, NdotV); // brdf * NoL / pdf

        //float3 wdiffuse = 1 / p_diff;
        //float3 wspecular = specular / (diffuse * p_spec);
        //float3 wdiffuse = diffuse / p_diff;
        //float3 wspecular = specular / p_spec;

        float3 diffuse = (1-metalness) * albedo * NdotL / PI;
        float3 specular = fr * D * G / (4 * NdotV);
        
        //float3 kS = fr;

        float3 kD = 1; // 1 - Ks
        
        //float3 brdf = chooseDiffuse ? kD * diffuse : specular / ((1 - metalness) * albedo);
        float3 brdf = diffuse + specular;

        float3 wbrdf = brdf / pdf;
        float3 color = wbrdf * payload.color;

        // clamp
        //float luminance = ComputeLuminance(color);
        //if(luminance > 4)
        //    color /= luminance * 4;

        // Since we didn't do a good job above catching NaN's, div by 0, infs, etc.,
        //    zero out samples that would blow up our frame buffer.  Note:  You can and should
        //    do better, but the code gets more complex with all error checking conditions.
        bool colorNan = any(isnan(color));
        if(!colorNan) {
            sumcolor += color;
            sumbrdf += wbrdf;
        }
        //else{
        //    sumcolor += float3(1000,0,1000);
        //}
    }

    sumcolor /= Num;
    sumbrdf /= Num;
    
    gOutput [launchIndex.xy] = float4(sumcolor, 0);
    gOutput2[launchIndex.xy] = float4(sumbrdf, 0);
}

////////////
// Shadow //
////////////

[shader("miss")]
void ShadowMiss(inout ShadowRayPayload payload)
{
    payload.vis = 1.f;
}

//////////////
// Indirect //
//////////////

TextureCube gCubeMap : register(t0, space1);
[shader("miss")]
 void IndirectMiss(inout IndirectPayload payload)
{
    payload.color = gCubeMap.SampleLevel(gSamplerLinearWrap, WorldRayDirection(), 0).xyz; 
}

struct VBData {
    float3 pos;
    float u;
    float v;
    float3 normal;
    float3 tangent;
};

struct VertexData {
    float3 pos;
    float3 normal; // normalized
    float3 albedo;
    float roughness;
    float metalness;
};

StructuredBuffer<VBData> gVertexBuffer : register(t0, space2);
Buffer<uint> gIndexBuffer : register(t1, space2);
Texture2D<float3> gAlbedo : register(t2, space2);
Texture2D<float> gRoughness : register(t3, space2);
Texture2D<float> gMetalness : register(t4, space2);
Texture2D<float3> gNormal : register(t5, space2);

VertexData GetVertexData(uint primitive_idx, float2 barycentrics) {
    uint idx0 = gIndexBuffer[3 * primitive_idx + 0];
    uint idx1 = gIndexBuffer[3 * primitive_idx + 1];
    uint idx2 = gIndexBuffer[3 * primitive_idx + 2];

    VBData data0 = gVertexBuffer[idx0];
    VBData data1 = gVertexBuffer[idx1];
    VBData data2 = gVertexBuffer[idx2];

    float u = barycentrics.x;
    float v = barycentrics.y;
    float w = 1 - u - v;

    float3 pos = w * data0.pos + u * data1.pos + v * data2.pos;
    float3 normal = normalize(w * data0.normal + u * data1.normal + v * data2.normal);
    float2 texc = w * float2(data0.u, data0.v) + u * float2(data1.u, data1.v) + v * float2(data2.u, data2.v);
    
    float3 albedo = gAlbedo.SampleLevel(gSamplerLinearWrap, texc, 0).rgb;
    float roughness = gRoughness.SampleLevel(gSamplerLinearWrap, texc, 0).r;
    float metalness = gMetalness.SampleLevel(gSamplerLinearWrap, texc, 0).r;
    // TODO: texture normal
    
    VertexData vd;
    vd.pos = pos;
    vd.normal = normal;
    vd.albedo = albedo;
    vd.roughness = lerp(0.01f, 1.f, roughness);
    vd.metalness = metalness;

    return vd;
}

[shader("closesthit")]
 void IndirectCHS(inout IndirectPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.color = float3(0.f, 0.f, 0.f);
    
    VertexData data = GetVertexData(PrimitiveIndex(), attribs.barycentrics);
    float roughness = data.roughness;
    float3 albedo = data.albedo;
    float metalness = data.metalness;
    
    float3 posW = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    float3x3 normalMatrix = transpose((float3x3)WorldToObject3x4());
    float3 N = normalize(mul(normalMatrix, data.normal));
    float alpha = Pow2(roughness);
    float3 V = -normalize(WorldRayDirection());
    float3 F0 = MetalWorkflow_F0(albedo, metalness);

    ////////////
    // direct //
    ////////////
    uint lightCnt = gDirectionalLightNum + gPointLightNum + gSpotLightNum + gRectLightNum;
    if(lightCnt > 0) {
        uint lightIdx = min(uint(nextRand(payload.randSeed) * lightCnt), lightCnt - 1);
        float3 L;
        float dist;
        float3 intensity;
        EvaluateLight(payload.randSeed, lightIdx, posW, L, dist, intensity);

        float3 H = normalize(L + V);

        float NdotH = dot(N, H);
        float NdotV = max(dot(N, V), EPSILON);
        float NdotL = dot(N, L);
        float HdotV = dot(H, V);
        float HdotL = dot(H, L); // == HdotV
        if(NdotH>0&&NdotV>0&&NdotL>0&&HdotV>0&&HdotL>0) {
            float visibility = ShadowVisibility(posW, L, dist);
            float shadowMult = lightCnt * visibility;

            float D = GGX_D(alpha, NdotH);
            float G = GGX_G(alpha, NdotL, NdotV);
            float3 fr = Fresnel(F0, HdotL);

            float3 diffuse = (1-data.metalness) * data.albedo * NdotL / PI;
            float3 specular = fr * D * G / (4 * NdotV);

            float3 brdf = diffuse + specular;
            float3 color = shadowMult * brdf * intensity;

            payload.color += color;
        }
    }
    
    //////////////
    // Indirect //
    //////////////
    if(payload.depth + 2 /*indirect + shadow*/ <= MAX_RAY_DEPTH) {
        payload.depth += 1;
        
        float3 up = float3(0.0, 1.0, 0.0);
        float3 right = normalize(cross(up, N));
        up = normalize(cross(N, right));

        float expect_diff = (1-metalness) * ComputeLuminance(albedo);
        float expect_spec = ComputeLuminance(F0);
        float p_diff = expect_diff / (expect_diff + expect_spec);
        float p_spec = 1 - p_diff;
        float2 Xi = float2(nextRand(payload.randSeed), nextRand(payload.randSeed));
        float3 L; // to light
        float3 H; // half vector
        bool chooseDiffuse = nextRand(payload.randSeed) < p_diff;
        if(chooseDiffuse) { // sample diff
            float3 sL = CosOnHalfSphere(Xi);
            L = normalize(sL.x * right + sL.y * up + sL.z * N); // normalized
            H = normalize(L + V);
        }
        else { // sample spec
            //float3 sH = SchlickGGX_Sample(Xi, roughness);
            float Vx = dot(V, right);
            float Vy = dot(V, up);
            float Vz = dot(V, N);
            float3 sV = float3(Vx, Vy, Vz);
            float3 sH = SampleGGXVNDF(sV, alpha, Xi.x, Xi.y);
            H = normalize(sH.x * right + sH.y * up + sH.z * N); // normalized
            L = reflect(-V, H); // to light
        }
        float NdotH = dot(N, H);
        float NdotV = max(dot(N, V), EPSILON);
        float NdotL = dot(N, L);
        float HdotV = dot(H, V);
        float HdotL = dot(H, L); // == HdotV
        if(NdotH>0&&NdotV>0&&NdotL>0&&HdotV>0&&HdotL>0) {
            float D = GGX_D(alpha, NdotH);
            float G = GGX_G(alpha, NdotL, NdotV);
            float3 fr = Fresnel(F0, HdotL);

            float pdf_diff = NdotL / PI;
            float pdf_spec = PDF_VNDF(alpha, NdotV, NdotH, HdotV);

            float pdf = p_diff * pdf_diff + p_spec * pdf_spec;

            RayDesc ray;
            ray.Origin = posW;
            ray.Direction = L;

            ray.TMin = RAY_TMIN;
            ray.TMax = RAY_TMAX;

            IndirectPayload nextpayload;
            nextpayload.color = float3(0.f, 0.f, 0.f);
            nextpayload.randSeed = payload.randSeed;
            nextpayload.depth = payload.depth + 1;
            TraceRay(
                gRtScene,
                0 /*flags*/,
                0xFF,
                INDIRECT_RAY_INDEX,
                PER_INSTANCE_HIT_GROUP_COUNT,
                INDIRECT_MISS_SHADER_INDEX,
                ray,
                nextpayload
            );
            payload.randSeed = nextpayload.randSeed;

            float3 diffuse = (1-metalness) * albedo * NdotL / PI;
            float3 specular = fr * D * G / (4 * NdotV);
            
            //float3 kS = fr;

            float3 kD = 1; // 1 - Ks
            
            float3 brdf = diffuse + specular;

            float3 wbrdf = brdf / pdf;
            float3 color = wbrdf * nextpayload.color;
            
            payload.color += color;
        }
    }
}
