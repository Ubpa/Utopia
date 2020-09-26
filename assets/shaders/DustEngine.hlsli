#ifndef DUST_ENGINE_HLSLI
#define DUST_ENGINE_HLSLI

#define DUST_ENGINE_CB_PER_OBJECT(x)            \
cbuffer DustEngine_cbPerObject : register(b##x) \
{                                               \
    float4x4 gWorld;                            \
    float4x4 gInvWorld;                         \
}

#define DUST_ENGINE_CB_PER_CAMERA(x)            \
cbuffer DustEngine_cbPerCamera : register(b##x) \
{                                               \
    float4x4 gView;                             \
    float4x4 gInvView;                          \
    float4x4 gProj;                             \
    float4x4 gInvProj;                          \
    float4x4 gViewProj;                         \
    float4x4 gInvViewProj;                      \
                                                \
    float3 gEyePosW;                            \
    float _g_cbPerFrame_pad0;                   \
                                                \
    float2 gRenderTargetSize;                   \
    float2 gInvRenderTargetSize;                \
                                                \
    float gNearZ;                               \
    float gFarZ;                                \
    float gTotalTime;                           \
    float gDeltaTime;                           \
}

#endif // DUST_ENGINE_HLSLI
