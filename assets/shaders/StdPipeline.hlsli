#ifndef STD_PIEPELINE_HLSLI
#define STD_PIEPELINE_HLSLI

// [ configuration ] define before include
// 1. STD_PIPELINE_ENABLE_LTC
// - 1. STD_PIPELINE_LTC_RECT_SPHEXE_APPROX

#include "StdPipeline_details.hlsli"

// standard pipeline static samplers
SamplerState gSamplerPointWrap   : register(s0);
SamplerState gSamplerPointClamp  : register(s1);
SamplerState gSamplerLinearWrap  : register(s2);
SamplerState gSamplerLinearClamp : register(s3);

#define STD_PIPELINE_CB_PER_OBJECT(x)            \
cbuffer StdPipeline_cbPerObject : register(b##x) \
{                                                \
    float4x4 gWorld;                             \
    float4x4 gInvWorld;                          \
    float4x4 gPrevWorld;                         \
}

#define STD_PIPELINE_CB_PER_CAMERA(x)            \
cbuffer StdPipeline_cbPerCamera : register(b##x) \
{                                                \
    float4x4 gView;                              \
    float4x4 gInvView;                           \
    float4x4 gProj;                              \
    float4x4 gInvProj;                           \
    float4x4 gViewProj;                          \
    float4x4 gUnjitteredViewProj;                \
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
    float2 gJitter;                              \
    uint _padding0;                              \
    uint _padding1;                              \
}

#define STD_PIPELINE_CB_DIRECTIONAL_SHADOW(x)            \
cbuffer StdPipeline_cbDirectionalShadow : register(b##x) \
{                                                        \
    float4x4 gDirectionalShadowViewProj;                 \
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

#define STD_PIPELINE_SR3_IBL(x)                           \
TextureCube StdPipeline_IrradianceMap : register(t[x  ]); \
TextureCube StdPipeline_PreFilterMap  : register(t[x+1]); \
Texture2D   StdPipeline_BRDFLUT       : register(t[x+2]); \
static const uint StdPipeline_MaxMipLevel = 4

#define STD_PIPELINE_SAMPLE_PREFILTER(R, roughness) \
StdPipeline_PreFilterMap.SampleLevel(gSamplerLinearClamp, R, roughness * StdPipeline_MaxMipLevel).rgb

#define STD_PIPELINE_SAMPLE_BRDFLUT(NdotV, roughness) \
StdPipeline_BRDFLUT.Sample(gSamplerLinearClamp, float2(NdotV, roughness)).rg

#define STD_PIPELINE_SAMPLE_LEVEL_BRDFLUT(NdotV, roughness) \
StdPipeline_BRDFLUT.SampleLevel(gSamplerLinearClamp, float2(NdotV, roughness), 0).rg

#define STD_PIPELINE_SAMPLE_IRRADIANCE(N) \
StdPipeline_IrradianceMap.Sample(gSamplerLinearClamp, N).rgb

#ifdef STD_PIPELINE_ENABLE_LTC
#define STD_PIPELINE_SR2_LTC(x)                  \
/* GGX m(0,0) m(2,0) m(0,2) m(2,2) */            \
Texture2D StdPipeline_LTC0 : register(t[x    ]); \
/* GGX norm, fresnel, 0(unused), sphere */       \
Texture2D StdPipeline_LTC1 : register(t[x + 1]); \
STD_PIPELINE_LTC_SAMPLE_DEFINE
#endif // STD_PIPELINE_ENABLE_LTC

#endif // STD_PIEPELINE_HLSLI
