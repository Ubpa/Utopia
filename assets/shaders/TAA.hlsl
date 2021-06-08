#include "StdPipeline.hlsli"

STD_PIPELINE_CB_PER_CAMERA(0);

float3 RGB2YCoCgR(float3 rgbColor)
{
    float3 YCoCgRColor;

    YCoCgRColor.y = rgbColor.r - rgbColor.b;
    float temp = rgbColor.b + YCoCgRColor.y / 2;
    YCoCgRColor.z = rgbColor.g - temp;
    YCoCgRColor.x = temp + YCoCgRColor.z / 2;

    return YCoCgRColor;
}

float3 YCoCgR2RGB(float3 YCoCgRColor)
{
    float3 rgbColor;

    float temp = YCoCgRColor.x - YCoCgRColor.z / 2;
    rgbColor.g = YCoCgRColor.z + temp;
    rgbColor.b = temp - YCoCgRColor.y / 2;
    rgbColor.r = rgbColor.b + YCoCgRColor.y;

    return rgbColor;
}

float3 ClipAABB(float3 aabbMin, float3 aabbMax, float3 prevSample, float3 avg)
{
    float3 r = prevSample - avg;
    float3 rmax = aabbMax - avg.xyz;
    float3 rmin = aabbMin - avg.xyz;

    const float eps = 0.000001f;

    if (r.x > rmax.x + eps)
        r *= (rmax.x / r.x);
    if (r.y > rmax.y + eps)
        r *= (rmax.y / r.y);
    if (r.z > rmax.z + eps)
        r *= (rmax.z / r.z);

    if (r.x < rmin.x - eps)
        r *= (rmin.x / r.x);
    if (r.y < rmin.y - eps)
        r *= (rmin.y / r.y);
    if (r.z < rmin.z - eps)
        r *= (rmin.z / r.z);

    return avg + r;
}

float Luma4(float3 Color)
{
    return Color.r;
}

// Optimized HDR weighting function.
float HdrWeight4(float3 Color, float Exposure)
{
    return rcp(Luma4(Color) * Exposure + 4.0);
}

Texture2D gPrevHDRImg : register(t0);
Texture2D gCurrHDRImg : register(t1);
Texture2D gMotion     : register(t2);

static const float2 gTexCoords[6] =
{
    float2(0.0f, 1.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 1.0f)
};

// {3/8, 1/4, 1/16} * 8/3
static const float gKernelWeights[3] = { 1.0,2.0/3.0,1.0/6.0 };

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float2 TexC : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vout;

    vout.TexC = gTexCoords[vid];

    // Quad covering screen in NDC space.
    vout.PosH = float4(2.0f*vout.TexC.x - 1.0f, 1.0f - 2.0f*vout.TexC.y, 0.0f, 1.0f);

    return vout;
}

static const float Exposure = 10;
static const float VarianceClipGamma = 1.0f;
static const float BlendWeightLowerBound = 0.05f;
static const float BlendWeightUpperBound = 0.2f;
static const float BlendWeightVelocityScale = 100.0f * 200.0f;

bool IsNan_float(float x) {
	return !(x < 0.0 || x > 0.0 || x == 0.0);
}
bool IsAnyNan_float3(float3 c) {
	return IsNan_float(c.x) || IsNan_float(c.y) || IsNan_float(c.z);
}
bool IsAnyNan_float4(float4 c) {
	return IsNan_float(c.x) || IsNan_float(c.y) || IsNan_float(c.z) || IsNan_float(c.w);
}

float4 PS(VertexOut pin) : SV_Target
{
    int x, y, i;
    float2 velocity;
    float lenVelocity;
    velocity = gMotion.Sample(gSamplerLinearClamp, pin.TexC).rg;
    lenVelocity = length(velocity);
    
    float2 InputSampleUV = pin.TexC;
    
    float3 currColor = gCurrHDRImg.Sample(gSamplerLinearClamp, InputSampleUV).rgb;
	//return float4(currColor,1);
    currColor = RGB2YCoCgR(currColor);
    
    float3 prevColor = gPrevHDRImg.Sample(gSamplerLinearClamp, pin.TexC - velocity).rgb;
    prevColor = RGB2YCoCgR(prevColor);

    float3 History;
    uint N = 9;
    float3 m1 = 0.0f;
    float3 m2 = 0.0f;
    float3 neighborMin = float3(9999999.0f, 9999999.0f, 9999999.0f);
    float3 neighborMax = float3(-99999999.0f, -99999999.0f, -99999999.0f);
    float neighborhoodFinalWeight = 0.0f;
    // 3x3 Sampling
    for (y = -1; y <= 1; ++y)
    {
        for (x = -1; x <= 1; ++x)
        {
            i = (y + 1) * 3 + x + 1;
            float2 sampleOffset = float2(x, y) * gInvRenderTargetSize;
            float2 sampleUV = InputSampleUV + sampleOffset;
            sampleUV = saturate(sampleUV);

            float3 NeighborhoodSamp = gCurrHDRImg.Sample(gSamplerLinearClamp, sampleUV).rgb;
            NeighborhoodSamp = max(NeighborhoodSamp, 0.0f);
			NeighborhoodSamp = RGB2YCoCgR(NeighborhoodSamp);

            neighborMin = min(neighborMin, NeighborhoodSamp);
            neighborMax = max(neighborMax, NeighborhoodSamp);
            neighborhoodFinalWeight = HdrWeight4(NeighborhoodSamp, Exposure);
            m1 += NeighborhoodSamp;
            m2 += NeighborhoodSamp * NeighborhoodSamp;
        }
    }
    
    // Variance clip.
    float3 mu = m1 / N;
    float3 sigma = sqrt(abs(m2 / N - mu * mu));
    float3 minc = mu - VarianceClipGamma * sigma;
    float3 maxc = mu + VarianceClipGamma * sigma;

    prevColor = ClipAABB(minc, maxc, prevColor, mu);

    float weightCurr = lerp(BlendWeightLowerBound, BlendWeightUpperBound, saturate(lenVelocity * BlendWeightVelocityScale));
    float weightPrev = 1.0f - weightCurr;

    History = currColor * weightCurr + prevColor * weightPrev;
    History.xyz = YCoCgR2RGB(History.xyz);

	if(IsAnyNan_float3(History))
	    return float4(currColor,1);
	
    return float4(History, 1.f);
}
