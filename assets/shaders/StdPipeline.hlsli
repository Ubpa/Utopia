#ifndef STD_PIEPELINE_HLSLI
#define STD_PIEPELINE_HLSLI

#define STD_PIPELINE_CB_PER_OBJECT(x)           \
cbuffer StdPipeline_cbPerObject : register(b##x) \
{                                                \
    float4x4 gWorld;                             \
    float4x4 gInvWorld;                          \
}

#define STD_PIPELINE_CB_PER_CAMERA(x)           \
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

#endif // STD_PIEPELINE_HLSLI
