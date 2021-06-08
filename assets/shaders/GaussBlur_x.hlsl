#include "StdPipeline.hlsli"

STD_PIPELINE_CB_PER_CAMERA(0);

Texture2D    gImage             : register(t0);

static const float2 gTexCoords[6] =
{
    float2(0.0f, 1.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 1.0f)
};

static const float gWeight[5] = {
  0.2270270270, 0.1945945946, 0.1216216216,
  0.0540540541, 0.0162162162
};

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

float4 PS(VertexOut pin) : SV_Target
{
    float4 color = gWeight[0] * gImage.Sample(gSamplerPointWrap, pin.TexC);
	
	float step = 1.f / gRenderTargetSize.x;

	[unroll]
	for(uint i=1; i<5; i++) {
		color += gWeight[i] * (gImage.Sample(gSamplerPointWrap, float2(pin.TexC.x + i * step, pin.TexC.y))
			+ gImage.Sample(gSamplerPointWrap, float2(pin.TexC.x - i * step, pin.TexC.y)));
	}
	
	return color;
}
