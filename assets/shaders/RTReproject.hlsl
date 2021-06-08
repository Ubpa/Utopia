#include "StdPipeline.hlsli"

float3 OctToDir(uint octo) {
	float2 e = float2( f16tof32(octo & 0xFFFF), f16tof32((octo>>16) & 0xFFFF) ); 
	float3 v = float3(e, 1.0 - abs(e.x) - abs(e.y));
	if (v.z < 0.0)
		v.xy = (1.0 - abs(v.yx)) * (step(0.0, v.xy)*2.0 - (float2)(1.0));
	return normalize(v);
}

STD_PIPELINE_CB_PER_CAMERA(0);

Texture2D gPrevRTS          : register(t0);
Texture2D gPrevRTU          : register(t1);
Texture2D gPrevColorMoments : register(t2);
Texture2D gMotion           : register(t3);

// linearZ, maxChangeZ, prevZ
Texture2D gPrevLinearZ : register(t4);
Texture2D gLinearZ     : register(t5);

RWTexture2D<float4> gRTS           : register(u0); // inout
RWTexture2D<float4> gRTU           : register(u1); // inout
RWTexture2D<float4> gColorMoments  : register(u2); // TODO: as render target
RWTexture2D<uint>   gHistoryLength : register(u3); // inout

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

float ComputeLuminance(float3 color) {
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

bool IsReprjValid(int2 coord, float Z, float Zprev, float fwidthZ, float3 normal, float3 normalPrev, float fwidthNormal)
{
    // check whether reprojected pixel is inside of the screen
    if(coord.x < 0 || coord.x >= gRenderTargetSize.x || coord.y < 0 || coord.y >= gRenderTargetSize.y) return false;
    // check if deviation of depths is acceptable
    if(abs(Zprev - Z) / (fwidthZ + 1e-4) > 2.0) return false;
    // check normals for compatibility
    if(distance(normal, normalPrev) / (fwidthNormal + 1e-2) > 16.0) return false;

    return true;
}

bool IsNan_float(float x) {
	return !(x < 0.0 || x > 0.0 || x == 0.0);
}

bool IsAnyNan_float4(float4 c) {
	return IsNan_float(c.x) || IsNan_float(c.y) || IsNan_float(c.z) || IsNan_float(c.w);
}

bool LoadPrevData(float2 texc, out float4 prevRTS, out float4 prevRTU, out float4 prevMoments, out uint historyLength)
{
    const int2 ipos = texc * gRenderTargetSize;

    // motion, 0, normFWidth
    float4 data_gMotion = gMotion[ipos];
    // Z, maxChangeZ, prevZ, objN
	float4 data_gLinearZ = gLinearZ[ipos];
    
    float2 motion = data_gMotion.xy;
    float normFWidth = data_gMotion.w;
    float Z = data_gLinearZ.x;
    float maxChangeZ = data_gLinearZ.y;
    float prevZ = data_gLinearZ.z;
    float3 normal = OctToDir(asuint(data_gLinearZ.w));

    // +0.5 to account for texel center offset
    const int2 prev_ipos = int2(float2(ipos) - motion * gRenderTargetSize + float2(0.5,0.5));

    prevRTS = float4(0,0,0,0);
    prevRTU = float4(0,0,0,0);
    prevMoments = float4(0,0,0,0);

    bool v[4];
	const float2 prev_pos = floor(texc * gRenderTargetSize) - motion * gRenderTargetSize;
    int2 offset[4] = { int2(0, 0), int2(1, 0), int2(0, 1), int2(1, 1) };
    
    // check for all 4 taps of the bilinear filter for validity
	bool valid = false;
    for (int sampleIdx = 0; sampleIdx < 4; sampleIdx++)
    { 
        int2 loc = int2(prev_pos) + offset[sampleIdx];
        float4 depthPrev = gPrevLinearZ[loc];
        float3 normalPrev = OctToDir(asuint(depthPrev.w));

        v[sampleIdx] = IsReprjValid(prev_ipos, prevZ, depthPrev.x, maxChangeZ, normal, normalPrev, normFWidth);

        valid = valid || v[sampleIdx];
    }    

    if (valid) 
    {
        float sumw = 0;
        float x = frac(prev_pos.x);
        float y = frac(prev_pos.y);

        // bilinear weights
        float w[4] = { (1 - x) * (1 - y), 
                            x  * (1 - y), 
                       (1 - x) *      y,
                            x  *      y };

        prevRTS = float4(0,0,0,0);
        prevRTU = float4(0,0,0,0);
        prevMoments = float4(0,0,0,0);

        // perform the actual bilinear interpolation
        for (int sampleIdx = 0; sampleIdx < 4; sampleIdx++)
        {
            int2 loc = int2(prev_pos) + offset[sampleIdx];            
            if (v[sampleIdx])
            {
                prevRTS += w[sampleIdx] * gPrevRTS[loc];
                prevRTU += w[sampleIdx] * gPrevRTU[loc];
                prevMoments += w[sampleIdx] * gPrevColorMoments[loc];
                sumw        += w[sampleIdx];
            }
        }

		// redistribute weights in case not all taps were used
		valid = (sumw >= 0.01);
		prevRTS = valid ? prevRTS / sumw : float4(0, 0, 0, 0);
		prevRTU = valid ? prevRTU / sumw : float4(0, 0, 0, 0);
		prevMoments = valid ? prevMoments / sumw : float4(0, 0, 0, 0);
    }
    if(!valid) // perform cross-bilateral filter in the hope to find some suitable samples somewhere
    {
        float cnt = 0.0;

        // this code performs a binary descision for each tap of the cross-bilateral filter
        const int radius = 1;
        for (int yy = -radius; yy <= radius; yy++)
        {
            for (int xx = -radius; xx <= radius; xx++)
            {
                int2 p = prev_ipos + int2(xx, yy);
                float4 depthFilter = gPrevLinearZ[p];
				float3 normalFilter = OctToDir(asuint(depthFilter.w));

                if ( IsReprjValid(prev_ipos, prevZ, depthFilter.x, maxChangeZ, normal, normalFilter, normFWidth) )
                {
					prevRTS += gPrevRTS[p];
                    prevRTU += gPrevRTU[p];
					prevMoments += gPrevColorMoments[p];
                    cnt += 1.0;
                }
            }
        }
        if (cnt > 0)
        {
            valid = true;
            prevRTS /= cnt;
            prevRTU /= cnt;
            prevMoments /= cnt;
        }

    }

    if (valid)
    {
	
        // crude, fixme
        historyLength = gHistoryLength[prev_ipos];
    }
    else
    {
        prevRTS = float4(0,0,0,0);
        prevRTU = float4(0,0,0,0);
        prevMoments = float4(0,0,0,0);
        historyLength = 0;
    }

    return valid;
}

static const float BlendWeightLowerBound = 0.05f;
static const float BlendWeightUpperBound = 0.1f;
static const float BlendWeightVelocityScale = 1.0f * 100.0f;

void PS(VertexOut pin)
{
    int2 itexc = int2(pin.TexC * gRenderTargetSize);
	
	float3 currRTS = gRTS[itexc].xyz;
	float3 currRTU = gRTU[itexc].xyz;
	
	// nan
    float4 prevRTS, prevRTU, prevMoments;
    uint historyLength;
    bool success = LoadPrevData(pin.TexC, prevRTS, prevRTU, prevMoments, historyLength);
    
    float4 data_gMotion = gMotion[itexc];
    // Z, maxChangeZ, prevZ, objN
	float4 data_gLinearZ = gLinearZ[itexc];
    float2 motion = data_gMotion.xy;
    float len_motion = length(motion);
    float min_alpha = lerp(BlendWeightLowerBound, BlendWeightUpperBound, saturate(len_motion * BlendWeightVelocityScale));

	float alpha = max(min_alpha, 1.f / (1.f + historyLength));
    
    float lum_rts = ComputeLuminance(currRTS);
    float lum_rtu = ComputeLuminance(currRTU);
	
	float4 cur_moments = float4(lum_rts, lum_rts*lum_rts, lum_rtu, lum_rtu*lum_rtu);
	
	if(IsAnyNan_float4(prevRTS) || IsAnyNan_float4(prevRTU) || IsAnyNan_float4(prevMoments)) {
		float2 var = cur_moments.ga - cur_moments.rb * cur_moments.rb;
		gRTS[itexc] = float4(currRTS, max(var.x, 0));
		gRTU[itexc] = float4(currRTU, max(var.y, 0));
		gColorMoments[itexc] = cur_moments;
		gHistoryLength[itexc] = 1;
	}
	else {
		float4 new_moments = lerp(prevMoments, cur_moments, alpha);
		float2 var = new_moments.ga - new_moments.rb * new_moments.rb;
		gRTS[itexc] = float4(lerp(prevRTS.xyz, currRTS, alpha), max(var.x, 0));
		gRTU[itexc] = float4(lerp(prevRTU.xyz, currRTU, alpha), max(var.y, 0));
		gColorMoments[itexc] = new_moments;
		gHistoryLength[itexc] = historyLength + 1;
	}
}
