#ifndef STD_PIEPELINE_DETAILS_HLSLI
#define STD_PIEPELINE_DETAILS_HLSLI

#ifdef STD_PIPELINE_ENABLE_LTC

#include "Constants.hlsli"
#include "Basic.hlsli"

static const float _LTC_LUT_SIZE  = 64.0;
static const float _LTC_LUT_SCALE = (_LTC_LUT_SIZE - 1.0) / _LTC_LUT_SIZE;
static const float _LTC_LUT_BIAS  = 0.5 / _LTC_LUT_SIZE;

float2 _LTC_CorrectUV(float2 uv) {
    return uv * _LTC_LUT_SCALE + _LTC_LUT_BIAS;
}

// _LTC_GetInvM_GGX, _LTC_GetNF_GGX
float2 _LTC_GetUV(float roughness, float NoV) {
    float2 uv = float2(roughness, sqrt(1.0 - NoV));
    return _LTC_CorrectUV(uv);
}

// forward declare
float3x3 _LTC_GetInvM_GGX(float2 uv);
float2 _LTC_GetNF_GGX(float2 uv);
float _LTC_GetSphere(float NoF, float lenF);

#define STD_PIPELINE_LTC_SAMPLE_DEFINE                               \
/* GGX inv M */                                                      \
float3x3 _LTC_GetInvM_GGX(float2 uv) {                               \
    float4 t1 = StdPipeline_LTC0.Sample(gSamplerLinearWrap, uv);     \
    return float3x3(                                                 \
        t1.x, 0, t1.z,                                               \
           0, 1,    0,                                               \
        t1.y, 0, t1.w                                                \
    );                                                               \
}                                                                    \
                                                                     \
/* GGX norm, Fresnel */                                              \
float2 _LTC_GetNF_GGX(float2 uv) {                                   \
    float4 t2 = StdPipeline_LTC1.Sample(gSamplerLinearWrap, uv);     \
    return float2(t2.x, t2.y);                                       \
}                                                                    \
                                                                     \
/* projected solid angle of a spherical cap */                       \
/* clipped to the horizon */                                         \
float _LTC_GetSphere(float NoF, float lenF) {                        \
    float2 uv = float2(NoF * 0.5 + 0.5, lenF);                       \
    uv = _LTC_CorrectUV(uv);                                         \
    return lenF * StdPipeline_LTC1.Sample(gSamplerLinearWrap, uv).w; \
}

// Rect
////////////////////

float _LTC_FitSphereIntegral(float NoF, float lenF) {
    return max(0, (lenF * lenF + NoF) / (lenF + 1));
}

float3 _LTC_IntegrateEdgefloat(float3 v1, float3 v2) {
    float x = dot(v1, v2);
    float y = abs(x);
    float a = 0.8543985 + (0.4965155 + 0.0145206 * y) * y;
    float b = 3.4175940 + (4.1616724 + y) * y;
    float v = a / b;
    float theta_sintheta = (x > 0.0) ? v : 0.5 * rsqrt(max(1.0 - x * x, 1e-7)) - v;
    return cross(v1, v2) * theta_sintheta;
}

float _LTC_IntegrateEdge(float3 v1, float3 v2) {
    return _LTC_IntegrateEdgefloat(v1, v2).z;
}

uint _LTC_ClipQuadToHorizon(inout float3 L[5]) {
    // detect clipping config
    int config = 0;
    if (L[0].z > 0.0) config += 1;
    if (L[1].z > 0.0) config += 2;
    if (L[2].z > 0.0) config += 4;
    if (L[3].z > 0.0) config += 8;
    // clip
    uint n = 0;
    if (config == 0)
    {
        // clip all
    }
    else if (config == 1) // V1 clip V2 V3 V4
    {
        n = 3;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[3].z * L[0] + L[0].z * L[3];
    }
    else if (config == 2) // V2 clip V1 V3 V4
    {
        n = 3;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
    }
    else if (config == 3) // V1 V2 clip V3 V4
    {
        n = 4;
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
        L[3] = -L[3].z * L[0] + L[0].z * L[3];
    }
    else if (config == 4) // V3 clip V1 V2 V4
    {
        n = 3;
        L[0] = -L[3].z * L[2] + L[2].z * L[3];
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
    }
    else if (config == 5) // V1 V3 clip V2 V4) impossible
    {
        n = 0;
    }
    else if (config == 6) // V2 V3 clip V1 V4
    {
        n = 4;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
    }
    else if (config == 7) // V1 V2 V3 clip V4
    {
        n = 5;
        L[4] = -L[3].z * L[0] + L[0].z * L[3];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
    }
    else if (config == 8) // V4 clip V1 V2 V3
    {
        n = 3;
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
        L[1] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] = L[3];
    }
    else if (config == 9) // V1 V4 clip V2 V3
    {
        n = 4;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[2].z * L[3] + L[3].z * L[2];
    }
    else if (config == 10) // V2 V4 clip V1 V3) impossible
    {
        n = 0;
    }
    else if (config == 11) // V1 V2 V4 clip V3
    {
        n = 5;
        L[4] = L[3];
        L[3] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
    }
    else if (config == 12) // V3 V4 clip V1 V2
    {
        n = 4;
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
    }
    else if (config == 13) // V1 V3 V4 clip V2
    {
        n = 5;
        L[4] = L[3];
        L[3] = L[2];
        L[2] = -L[1].z * L[2] + L[2].z * L[1];
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
    }
    else if (config == 14) // V2 V3 V4 clip V1
    {
        n = 5;
        L[4] = -L[0].z * L[3] + L[3].z * L[0];
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
    }
    else if (config == 15) // V1 V2 V3 V4
    {
        n = 4;
    }
    if (n == 3)
        L[3] = L[0];
    if (n == 4)
        L[4] = L[0];

    return n;
}

float _LTC_Rect_Evaluate(float3 N, float3 V, float3 P, float3x3 invM,
    float3 p0, float3 p1, float3 p2, float3 p3
) {
    // construct orthonormal basis around N
    float3 T1, T2;
    T1 = normalize(V - N * dot(V, N));
    T2 = cross(N, T1);
    // rotate area light in (T1, T2, N) basis
    invM = mul(invM, float3x3(T1, T2, N));
    // polygon
    float3 L[5];
    L[0] = mul(invM, p0 - P);
    L[1] = mul(invM, p1 - P);
    L[2] = mul(invM, p2 - P);
    L[3] = mul(invM, p3 - P);
	L[4] = float3(0,0,0);
    // integrate
    float sum = 0.0;
#ifdef STD_PIPELINE_LTC_RECT_SPHEXE_APPROX // sphere approximation
    float3 dir = p0.xyz - P;
    float3 lightNormal = cross(p1 - p0, p3 - p0);
    bool behind = dot(dir, lightNormal) > 0.0;
    L[0] = normalize(L[0]);
    L[1] = normalize(L[1]);
    L[2] = normalize(L[2]);
    L[3] = normalize(L[3]);
    float3 vsum = float3(0, 0, 0);
    vsum += _LTC_IntegrateEdgefloat(L[1], L[0]);
    vsum += _LTC_IntegrateEdgefloat(L[2], L[1]);
    vsum += _LTC_IntegrateEdgefloat(L[3], L[2]);
    vsum += _LTC_IntegrateEdgefloat(L[0], L[3]);
    float len = length(vsum);
    float z = vsum.z / len;
    if (behind)
        z = -z;
    sum = _LTC_GetSphere(z, len);
    if (behind)
        sum = 0.0;
#else
    uint n = _LTC_ClipQuadToHorizon(L);
    if (n > 0) {
        // project onto sphere
        L[0] = normalize(L[0]);
        L[1] = normalize(L[1]);
        L[2] = normalize(L[2]);
        L[3] = normalize(L[3]);
        L[4] = normalize(L[4]);
        // integrate
        sum += _LTC_IntegrateEdge(L[1], L[0]);
        sum += _LTC_IntegrateEdge(L[2], L[1]);
        sum += _LTC_IntegrateEdge(L[3], L[2]);
        if (n >= 4)
            sum += _LTC_IntegrateEdge(L[4], L[3]);
        if (n == 5)
            sum += _LTC_IntegrateEdge(L[0], L[4]);
        sum = max(0.0, sum);
    }
#endif
    return sum;
}

float3 LTC_Rect_Spec(float3 N, float3 V, float3 P, float3 F0, float roughness,
    float3 p0, float3 p1, float3 p2, float3 p3
) {
    float NoV = saturate(dot(N, V));
    float2 uv = _LTC_GetUV(roughness, NoV);
    float3x3 invM = _LTC_GetInvM_GGX(uv);
    float2 nf = _LTC_GetNF_GGX(uv);
    float3 Fr = F0 * nf.x + (1 - F0) * nf.y;
    float spec = _LTC_Rect_Evaluate(N, V, P, invM, p0, p1, p2, p3);
    return Fr * spec;
}

float3 LTC_Rect_Diffuse(float3 N, float3 V, float3 P, float roughness,
    float3 p0, float3 p1, float3 p2, float3 p3
) {
    float3x3 invM = float3x3(
        1, 0, 0,
        0, 1, 0,
        0, 0, 1
    );
    return _LTC_Rect_Evaluate(N, V, P, invM, p0, p1, p2, p3);
}

// Disk
////////////////////

// An extended version of the implementation from
// "How to solve a cubic equation, revisited"
// http://momentsingraphics.de/?p=105
float3 _LTC_SolveCubic(float4 Coefficient) {
    // Normalize the polynomial
    Coefficient.xyz /= Coefficient.w;
    // Divide middle coefficients by three
    Coefficient.yz /= 3.0;

    float A = Coefficient.w;
    float B = Coefficient.z;
    float C = Coefficient.y;
    float D = Coefficient.x;

    // Compute the Hessian and the discriminant
    float3 Delta = float3(
        -Coefficient.z*Coefficient.z + Coefficient.y,
        -Coefficient.y*Coefficient.z + Coefficient.x,
        dot(float2(Coefficient.z, -Coefficient.y), Coefficient.xy)
    );

    float Discriminant = dot(float2(4.0*Delta.x, -Delta.y), Delta.zy);

    float3 RootsA, RootsD;

    float2 xlc, xsc;

    // Algorithm A
    {
        float A_a = 1.0;
        float C_a = Delta.x;
        float D_a = -2.0*B*Delta.x + Delta.y;

        // Take the cubic root of a normalized complex number
        float Theta = atan2(sqrt(Discriminant), -D_a)/3.0;

        float x_1a = 2.0*sqrt(-C_a)*cos(Theta);
        float x_3a = 2.0*sqrt(-C_a)*cos(Theta + (2.0/3.0)*PI);

        float xl;
        if ((x_1a + x_3a) > 2.0*B)
            xl = x_1a;
        else
            xl = x_3a;

        xlc = float2(xl - B, A);
    }

    // Algorithm D
    {
        float A_d = D;
        float C_d = Delta.z;
        float D_d = -D*Delta.y + 2.0*C*Delta.z;

        // Take the cubic root of a normalized complex number
        float Theta = atan2(D*sqrt(Discriminant), -D_d)/3.0;

        float x_1d = 2.0*sqrt(-C_d)*cos(Theta);
        float x_3d = 2.0*sqrt(-C_d)*cos(Theta + (2.0/3.0)*PI);

        float xs;
        if (x_1d + x_3d < 2.0*C)
            xs = x_1d;
        else
            xs = x_3d;

        xsc = float2(-D, xs + C);
    }

    float E =  xlc.y*xsc.y;
    float F = -xlc.x*xsc.y - xlc.y*xsc.x;
    float G =  xlc.x*xsc.x;

    float2 xmc = float2(C*F - B*G, -B*F + C*E);

    float3 Root = float3(xsc.x/xsc.y, xmc.x/xmc.y, xlc.x/xlc.y);

    if (Root.x < Root.y && Root.x < Root.z)
        Root.xyz = Root.yxz;
    else if (Root.z < Root.x && Root.z < Root.y)
        Root.xyz = Root.xzy;

    return Root;
}

float3 _LTC_Disk_Evaluate(
    float3 N, float3 V, float3 P, float3x3 invM,
    float3 p0, float3 p1, float3 p2
) {
    // construct orthonormal basis around N
    float3 T1, T2;
    T1 = normalize(V - N*dot(V, N));
    T2 = cross(N, T1);

    // rotate area light in (T1, T2, N) basis
    float3x3 R = float3x3(T1, T2, N);

    // polygon (allocate 5 vertices for clipping)
    float3 L_[3];
    L_[0] = mul(R, p0 - P);
    L_[1] = mul(R, p1 - P);
    L_[2] = mul(R, p2 - P);

    float3 Lo_i = float3(0, 0, 0);

    // init ellipse
    float3 C  = 0.5 * (L_[0] + L_[2]);
    float3 V1 = 0.5 * (L_[1] - L_[2]);
    float3 V2 = 0.5 * (L_[1] - L_[0]);

    C  = mul(invM, C);
    V1 = mul(invM, V1);
    V2 = mul(invM, V2);

    if(dot(cross(V1, V2), C) > 0.0)
        return float3(0.0, 0.0, 0.0);

    // compute eigenfloattors of ellipse
    float a, b;
    float d11 = dot(V1, V1);
    float d22 = dot(V2, V2);
    float d12 = dot(V1, V2);
    if (abs(d12)/sqrt(d11*d22) > 0.0001)
    {
        float tr = d11 + d22;
        float det = -d12*d12 + d11*d22;

        // use sqrt matrix to solve for eigenvalues
        det = sqrt(det);
        float u = 0.5*sqrt(tr - 2.0*det);
        float v = 0.5*sqrt(tr + 2.0*det);
        float e_max = pow2(u + v);
        float e_min = pow2(u - v);

        float3 V1_, V2_;

        if (d11 > d22)
        {
            V1_ = d12*V1 + (e_max - d11)*V2;
            V2_ = d12*V1 + (e_min - d11)*V2;
        }
        else
        {
            V1_ = d12*V2 + (e_max - d22)*V1;
            V2_ = d12*V2 + (e_min - d22)*V1;
        }

        a = 1.0 / e_max;
        b = 1.0 / e_min;
        V1 = normalize(V1_);
        V2 = normalize(V2_);
    }
    else
    {
        a = 1.0 / dot(V1, V1);
        b = 1.0 / dot(V2, V2);
        V1 *= sqrt(a);
        V2 *= sqrt(b);
    }

    float3 V3 = cross(V1, V2);
    if (dot(C, V3) < 0.0)
        V3 *= -1.0;

    float L  = dot(V3, C);
    float x0 = dot(V1, C) / L;
    float y0 = dot(V2, C) / L;

    float E1 = rsqrt(a);
    float E2 = rsqrt(b);

    a *= L*L;
    b *= L*L;

    float c0 = a*b;
    float c1 = a*b*(1.0 + x0*x0 + y0*y0) - a - b;
    float c2 = 1.0 - a*(1.0 + x0*x0) - b*(1.0 + y0*y0);
    float c3 = 1.0;

    float3 roots = _LTC_SolveCubic(float4(c0, c1, c2, c3));
    float e1 = roots.x;
    float e2 = roots.y;
    float e3 = roots.z;

    float3 avgDir = float3(a*x0/(a - e2), b*y0/(b - e2), 1.0);

    float3x3 rotate = transpose(float3x3(V1, V2, V3));

    avgDir = mul(rotate, avgDir);
    avgDir = normalize(avgDir);

    float L1 = sqrt(-e2/e3);
    float L2 = sqrt(-e2/e1);

    float formFactor = L1*L2*rsqrt((1.0 + L1*L1)*(1.0 + L2*L2));

    // use tabulated horizon-clipped sphere
    float spec = _LTC_GetSphere(avgDir.z, formFactor);

    Lo_i = float3(spec, spec, spec);

    return float3(Lo_i);
}

float3 LTC_Disk_Spec(float3 N, float3 V, float3 P, float3 F0, float roughness,
    float3 p0, float3 p1, float3 p2
) {
    float NoV = saturate(dot(N, V));
    float2 uv = _LTC_GetUV(roughness, NoV);
    float3x3 invM = _LTC_GetInvM_GGX(uv);
    float2 nf = _LTC_GetNF_GGX(uv);
    float3 Fr = F0 * nf.x + (1 - F0) * nf.y;
    float spec = _LTC_Disk_Evaluate(N, V, P, invM, p0, p1, p2);
    return Fr * spec;
}

float3 LTC_Disk_Diffuse(float3 N, float3 V, float3 P, float roughness,
    float3 p0, float3 p1, float3 p2
) {
    float3x3 invM = float3x3(
        1, 0, 0,
        0, 1, 0,
        0, 0, 1
    );
    return _LTC_Disk_Evaluate(N, V, P, invM, p0, p1, p2);
}

#endif // STD_PIPELINE_ENABLE_LTC

#endif // STD_PIEPELINE_DETAILS_HLSLI