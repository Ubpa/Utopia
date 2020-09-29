#ifndef STD_PIEPELINE_HLSLI
#define STD_PIEPELINE_HLSLI

// standard pipeline static samplers
SamplerState gSamplerPointWrap  : register(s0);
SamplerState gSamplerLinearWrap : register(s2);

#define STD_PIPELINE_CB_PER_OBJECT(x)            \
cbuffer StdPipeline_cbPerObject : register(b##x) \
{                                                \
    float4x4 gWorld;                             \
    float4x4 gInvWorld;                          \
}

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
    float3 gEyePosW;                             \
    float _g_cbPerFrame_pad0;                    \
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
// range
// radius : f0

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

// cover 3 register
#define STD_PIPELINE_SR_IBL(x)                            \
TextureCube StdPipeline_IrradianceMap : register(t[x  ]); \
TextureCube StdPipeline_PreFilterMap  : register(t[x+1]); \
Texture2D   StdPipeline_BRDFLUT       : register(t[x+2])

#define STD_PIPELINE_PREFILTER_MIP_LEVEL 4

#define STD_PIPELINE_SAMPLE_PREFILTER(R, roughness) \
StdPipeline_PreFilterMap.SampleLevel(gSamplerLinearWrap, R, roughness * STD_PIPELINE_PREFILTER_MIP_LEVEL).rgb

#define STD_PIPELINE_SAMPLE_BRDFLUT(NdotV, roughness) \
StdPipeline_BRDFLUT.Sample(gSamplerLinearWrap, float2(NdotV, roughness)).rg

#define STD_PIPELINE_SAMPLE_IRRADIANCE(N) \
StdPipeline_IrradianceMap.Sample(gSamplerLinearWrap, N).rgb

#endif // STD_PIEPELINE_HLSLI
