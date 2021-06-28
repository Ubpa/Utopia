#include "StdPipeline.hlsli"
#include "PBR.hlsli"

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

RWTexture2D<float4> gOutput  : register(u0); // rt result
RWTexture2D<float4> gOutput2 : register(u1); // rt u result

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
	
	float2 scale_bias = STD_PIPELINE_SAMPLE_LEVEL_BRDFLUT(roughness, NdotV);
	float3 specular_int = scale_bias.x * F0 + scale_bias.y;

    uint Num = 1;
    float3 sum_diffuse = float3(0,0,0);
    float3 sum_specular = float3(0,0,0);
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

                float3 diffuse = shadowMult * intensity * NdotL / PI;
                float3 specular = shadowMult * intensity * fr * D * G / (4 * NdotV);

                bool diffuseNan = any(isnan(diffuse));
                bool specularNan = any(isnan(specular));
                if(!diffuseNan && !specularNan) {
					sum_diffuse += diffuse;
					sum_specular += specular;
				}
            }
        }

        //////////////
        // Indirect //
        //////////////

        float expect_diff = (1-metalness) * ComputeLuminance(albedo);
        float expect_spec = ComputeLuminance(specular_int);
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

        float3 diffuse = payload.color * NdotL / PI / pdf;
        float3 specular = payload.color * fr * D * G / (4 * NdotV * pdf);

        bool diffuseNan = any(isnan(diffuse));
        bool specularNan = any(isnan(specular));
        if(!diffuseNan && !specularNan) {
            sum_diffuse += diffuse;
            sum_specular += specular;
        }
    }

    sum_diffuse /= Num;
    sum_specular /= Num;
    
    gOutput [launchIndex.xy] = float4(sum_diffuse, 0);
    gOutput2[launchIndex.xy] = float4(sum_specular / specular_int, 0);
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
    float4 albedo;
    float roughness;
    float metalness;
};

StructuredBuffer<VBData> gVertexBuffer : register(t0, space2);
Buffer<uint> gIndexBuffer : register(t1, space2);
Texture2D<float4> gAlbedo : register(t2, space2);
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
    
    float4 albedo = gAlbedo.SampleLevel(gSamplerLinearWrap, texc, 0).rgba;
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


[shader("anyhit")]
void IndirectAHS(inout IndirectPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
    VertexData data = GetVertexData(PrimitiveIndex(), attribs.barycentrics);
	if(data.albedo.a < 0.5)
		IgnoreHit();
}

[shader("closesthit")]
void IndirectCHS(inout IndirectPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.color = float3(0.f, 0.f, 0.f);
    
    VertexData data = GetVertexData(PrimitiveIndex(), attribs.barycentrics);
    float roughness = data.roughness;
    float3 albedo = data.albedo.rgb;
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

            float3 diffuse = (1-data.metalness) * data.albedo.rgb * NdotL / PI;
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
