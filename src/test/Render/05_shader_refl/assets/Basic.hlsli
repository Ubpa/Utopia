#ifndef BASIC_HLSLI
#define BASIC_HLSLI

float pow2(float x) { return x * x; }
float pow4(float x) { return pow2(pow2(x)); }
float pow5(float x) { return pow4(x) * x; }

#endif // BASIC_HLSLI