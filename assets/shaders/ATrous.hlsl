#include "StdPipeline.hlsli"
#include "SVGFConfig.hlsli"

STD_PIPELINE_CB_PER_CAMERA(0);

cbuffer gATrousConfig : register(b1) {
	int gKernelStep;
}

// rgb: color, a: var
Texture2D gRTS : register(t0);
Texture2D gRTU : register(t1);

//	  [rgb    a        ]
// 0  [albedo roughness]
// 1  [N      metalness]
// 2  [0      matID    ]
Texture2D gGBuffer1 : register(t2);

// x: Z, y: maxChangeZ, z: prevZ, w: objN
Texture2D gLinearZ : register(t3);

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

struct PixelOut {
	float4 avg_RTS : SV_Target0;
	float4 avg_RTU : SV_Target1;
};

float ComputeLuminance(float3 color) {
	return dot(color, float3(0.299f, 0.587f, 0.114f));
}

float2 ComputeWeight(
	float phi_rts, float center_luminance_rts, float luminance_rts,
	float phi_rtu, float center_luminance_rtu, float luminance_rtu,
	float phi_normal, float3 center_normal, float3 normal,
	float phi_z, float center_z, float z)
{
	float exponent_rts = abs(center_luminance_rts - luminance_rts) / max(1e-6f, phi_rts);
	float exponent_rtu = abs(center_luminance_rtu - luminance_rtu) / max(1e-6f, phi_rtu);
	float exponent_z = abs(center_z - z) / max(1e-6f, phi_z);
	float weight_n = pow(max(0, dot(center_normal, normal)), phi_normal);
	
	float weight_rts = exp(0.0 - max(exponent_rts, 0.0) - max(exponent_z, 0.0)) * weight_n;
	float weight_rtu = exp(0.0 - max(exponent_rtu, 0.0) - max(exponent_z, 0.0)) * weight_n;
	
	return float2(weight_rts, weight_rtu);
}
float2 ComputeVarianceCenter(int2 ipos, Texture2D sA, Texture2D sB)
{
	float2 sum = float2(0.0, 0.0);

	const float kernel[2][2] = {
		{ 1.0 / 4.0, 1.0 / 8.0  },
		{ 1.0 / 8.0, 1.0 / 16.0 }
	};

	const int radius = 1;
	for (int yy = -radius; yy <= radius; yy++)
	{
		for (int xx = -radius; xx <= radius; xx++)
		{
			int2 p = ipos + int2(xx, yy);

			float k = kernel[abs(xx)][abs(yy)];

			sum.r += sA.Load(int3(p, 0)).a * k;
			sum.g += sB.Load(int3(p, 0)).a * k;
		}
	}

	return sum;
}
bool IsNan_float(float x) {
	return !(x < 0.0 || x > 0.0 || x == 0.0);
}
bool IsAnyNan_float3(float3 c) {
	return IsNan_float(c.x) || IsNan_float(c.y) || IsNan_float(c.z);
}
bool IsAnyNan_float4(float4 c) {
	return IsNan_float(c.x) || IsNan_float(c.y) || IsNan_float(c.z) || IsNan_float(c.w);
}
PixelOut PS(VertexOut pin) : SV_Target
{
	int2 itexc = int2(pin.TexC*gRenderTargetSize);
	float4 center_rtS = gRTS[itexc];
	float4 center_rtU = gRTU[itexc];
	float4 center_data1 = gGBuffer1[itexc];
	float4 center_data2 = gLinearZ[itexc];
	float2 var = ComputeVarianceCenter(itexc, gRTS, gRTU);

	float3 center_N	   = center_data1.xyz;
	float  center_linearZ = center_data2.x;
	float  center_changeZ = center_data2.y;

	float phi_rtS = SVGF_CONFIG_COLOR_PHI * sqrt(max(0.0, 1e-10 + var.r));
	float phi_rtU = 1 * sqrt(max(0.0, 1e-10 + var.g));
	float phi_z_factor = center_changeZ * gKernelStep;
	float phi_normal = SVGF_CONFIG_NORMAL_PHI;

	float luminance_center_rtS = ComputeLuminance(center_rtS.xyz);
	float luminance_center_rtU = ComputeLuminance(center_rtU.xyz);

	float2 pixelstep = 1.f / gRenderTargetSize;

	float4 sumS = center_rtS;
	float4 sumU = center_rtU;
	float sumSW = 1.f;
	float sumUW = 1.f;

	for(int yy = -2; yy <= 2; yy++) {
		for(int xx = -2; xx <= 2; xx++) {
			float2 itexc_xy = itexc + gKernelStep * int2(xx, yy);
			float4 data2 = gLinearZ[itexc_xy];
			bool inside = itexc_xy.x >= 0
				&& itexc_xy.x < gRenderTargetSize.x
				&& itexc_xy.y >= 0
				&& itexc_xy.y < gRenderTargetSize.y
				&& data2.x > 0.f;
			
			if(inside && (xx != 0 || yy != 0)) {
				float kernel = gKernelWeights[abs(xx)] * gKernelWeights[abs(yy)];

				float4 rtS = gRTS[itexc_xy];
				float4 rtU = gRTU[itexc_xy];

				float4 data1   = gGBuffer1[itexc_xy];
				float3 N	   = data1.xyz;
				float  linearZ = data2.x;
				
				float phi_z = phi_z_factor * length(float2(xx,yy));
				float2 weight = kernel * ComputeWeight(
					phi_rtS, luminance_center_rtS, ComputeLuminance(rtS.xyz), 
					phi_rtU, luminance_center_rtU, ComputeLuminance(rtU.xyz), 
					phi_normal, center_N, N, 
					phi_z, center_linearZ, linearZ
				);
				
				sumS += float4(weight.xxx, weight.x*weight.x) * gRTS[itexc_xy];
				sumU += float4(weight.yyy, weight.y*weight.y) * gRTU[itexc_xy];

				sumSW += weight.x;
				sumUW += weight.y;
			}
		}
	}
	
	PixelOut pout;
	if(IsAnyNan_float4(sumS) || IsAnyNan_float4(sumU)) {
		pout.avg_RTS = center_rtS;
		pout.avg_RTU = center_rtU;
	}
	else {
		pout.avg_RTS = sumS / float4(sumSW.xxx, sumSW*sumSW);
		pout.avg_RTU = sumU / float4(sumUW.xxx, sumUW*sumUW);
	}

	return pout;
}
