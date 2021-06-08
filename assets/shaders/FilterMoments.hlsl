#include "StdPipeline.hlsli"
#include "SVGFConfig.hlsli"

STD_PIPELINE_CB_PER_CAMERA(0);

// rgb: color, a: var
Texture2D gRTS : register(t0);
Texture2D gRTU : register(t1);

Texture2D gMoments : register(t2);

Texture2D<uint> gHistoryLength : register(t3);

//	  [rgb    a        ]
// 0  [albedo roughness]
// 1  [N      metalness]
// 2  [0      matID    ]
Texture2D gGBuffer1 : register(t4);

// x: Z, y: maxChangeZ, z: prevZ, w: objN
Texture2D gLinearZ : register(t5);

static const float2 gTexCoords[6] =
{
	float2(0.0f, 1.0f),
	float2(1.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 1.0f)
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

struct PixelOut {
	float4 out_RTS : SV_Target0;
	float4 out_RTU : SV_Target1;
};

float ComputeLuminance(float3 color) {
	return dot(color, float3(0.299f, 0.587f, 0.114f));
}

float2 ComputeWeight(
	float phi_rt, float center_luminance_rt, float luminance_rt,
	float phi_brdf, float center_brdf, float brdf,
	float phi_normal, float3 center_normal, float3 normal,
	float phi_z, float center_z, float z)
{
	float exponent_rt = abs(center_luminance_rt - luminance_rt) / max(1e-6f, phi_rt);
	float exponent_brdf = abs(center_brdf - brdf) / max(1e-6f, phi_brdf);
	float exponent_z = abs(center_z - z) / max(1e-6f, phi_z);
	float weight_n = pow(max(0, dot(center_normal, normal)), phi_normal);
	
	float weight_rt = exp(0.0 - max(exponent_rt, 0.0) /*- max(exponent_brdf, 0.0)*/ - max(exponent_z, 0.0)) * weight_n;
	float weight_brdf = exp(0.0 - max(exponent_brdf, 0.0)   - max(exponent_z, 0.0)) * weight_n;
	
	return float2(weight_rt, weight_brdf);
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
	int2 itexc = int2(pin.TexC * gRenderTargetSize);
	uint h = gHistoryLength[itexc].r;

	PixelOut pout;

	if(h < 4u) {
		// spatio filter

		float4 center_rtS = gRTS[itexc];
		float4 center_rtU = gRTU[itexc];
		float4 center_data1 = gGBuffer1[itexc];
		float4 center_data2 = gLinearZ[itexc];

		float3 center_N	      = center_data1.xyz;
		float  center_linearZ = center_data2.x;
		float  center_changeZ = center_data2.y;

		if (center_data2.x <= 0) {
			pout.out_RTS = gRTS[itexc];
			pout.out_RTU = gRTU[itexc];
			return pout;
		}

		float phi_rtS = 1;//SVGF_CONFIG_COLOR_PHI;
		float phi_rtU = 1;
		float phi_z_factor = max(center_changeZ, 1e-8) * 3;
		float phi_n = 0;//SVGF_CONFIG_NORMAL_PHI;

		float luminance_center_rtS = ComputeLuminance(center_rtS.xyz);
		float luminance_center_rtU = ComputeLuminance(center_rtU.xyz);

		float3 sumS = float3(0.f, 0.f, 0.f);
		float3 sumU = float3(0.f, 0.f, 0.f);
		float  sumSW = 0.f;
		float  sumUW = 0.f;
		float4 sumMoments = float4(0.f, 0.f, 0.f, 0.f);

		// compute first and second moment spatially. This code also applies cross-bilateral
        // filtering on the input color samples
        const int radius = 4;

		for(int yy = -radius; yy <= radius; yy++) {
			for(int xx = -radius; xx <= radius; xx++) {
				int2 itexc_xy = itexc + int2(xx, yy);
				float4 data2 = gLinearZ[itexc_xy];
				bool inside = itexc_xy.x >= 0
					&& itexc_xy.x < gRenderTargetSize.x
					&& itexc_xy.y >= 0
					&& itexc_xy.y < gRenderTargetSize.y
					&& data2.x > 0;
				
				if(inside) {
					float4 rtS = gRTS[itexc_xy];
					float4 rtU = gRTU[itexc_xy];
					float4 moments = gMoments[itexc_xy];

					float4 data1   = gGBuffer1[itexc_xy];
					float3 N	   = data1.xyz;
					float  linearZ = data2.x;
					
					float phi_z = phi_z_factor * length(float2(xx,yy));
					float2 weight = ComputeWeight(
						phi_rtS, luminance_center_rtS, ComputeLuminance(rtS.xyz), 
						phi_rtU, luminance_center_rtU, ComputeLuminance(rtU.xyz), 
						phi_n, center_N, N, 
						phi_z, center_linearZ, linearZ
					);
					
					if(any(isnan(weight)))
						continue;
					
					float weightS = weight.x;
					float weightU = weight.x; // weight.y;
					
					float3 S = weightS * gRTS[itexc_xy].rgb;
					float3 U = weightU * gRTU[itexc_xy].rgb;
					
					if(!IsAnyNan_float3(S) && !IsAnyNan_float3(U) && !IsAnyNan_float4(moments)) {
						sumS += S;
						sumU += U;

						sumSW += weightS;
						sumUW += weightU;

						sumMoments += moments * float4(weightS.xx, weightU.xx);
					}
				}
			}
		}

		// Clamp sums to >0 to avoid NaNs.
		sumSW = max(sumSW, 1e-6f);
		sumUW = max(sumUW, 1e-6f);

        sumS /= sumSW;
        sumU /= sumUW;

        sumMoments /= float4(sumSW.xx, sumUW.xx);

		float2 variance = sumMoments.ga - sumMoments.rb * sumMoments.rb;

        // give the variance a boost
        variance *= 4.0 / h;

        pout.out_RTS = float4(sumS, variance.r);
        pout.out_RTU = float4(sumU, variance.g);
	}
	else {
		pout.out_RTS = gRTS[itexc];
		pout.out_RTU = gRTU[itexc];
	}

	return pout;
}
