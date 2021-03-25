#include <Utopia/Core/Image.h>

#include <UGM/UGM.hpp>

#include <iostream>
#include <thread>

using namespace Ubpa::Utopia;
using namespace Ubpa;
using namespace std;

float _RadicalInverse_VdC(unsigned bits) {
	// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
	// efficient VanDerCorpus calculation.

	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10f; // / 0x100000000
}

valf2 Hammersley(unsigned i, unsigned N) {
	return { float(i) / float(N), _RadicalInverse_VdC(i) };
}

svecf SchlickGGX_Sample(valf2 Xi, float roughness) {
	float a = roughness * roughness;

	float phi = 2 * PI<float> * Xi[0];
	float cosTheta = sqrt((1.f - Xi[1]) / (1.f + (a * a - 1.f) * Xi[1]));
	float sinTheta = sqrt(1.f - cosTheta * cosTheta);

	// from spherical coordinates to cartesian coordinates - halfway vector
	svecf H{
		cos(phi) * sinTheta,
		sin(phi) * sinTheta,
		cosTheta
	};

	return H;
}

float GGX_G(float alpha, vecf3 L, vecf3 V, vecf3 N) {
	float alpha2 = alpha * alpha;

	float cos_sthetai = L.dot(N);
	float cos_sthetao = V.dot(N);

	if (cos_sthetai < 0 || cos_sthetao < 0)
		return 0.f;

	float tan2_sthetai = 1.f / (cos_sthetai * cos_sthetai) - 1.f;
	float tan2_sthetao = 1.f / (cos_sthetao * cos_sthetao) - 1.f;

	return 2.f / (sqrt(1.f + alpha2 * tan2_sthetai) + sqrt(1.f + alpha2 * tan2_sthetao));
}

valf2 IntegrateBRDF(float NdotV, float roughness) {
	// 由于各向同性，随意取一个 V 即可
	vecf3 V;
	V[0] = sqrt(1.f - NdotV * NdotV);
	V[1] = 0.f;
	V[2] = NdotV;

	float A = 0.f;
	float B = 0.f;

	vecf3 N{ 0,0,1 };
	vecf3 right{ 1,0,0 };
	vecf3 up{ 0,1,0 };

	const unsigned int SAMPLE_COUNT = 16384u;
	for (unsigned int i = 0u; i < SAMPLE_COUNT; ++i)
	{
		// generates a sample vector that's biased towards the
		// preferred alignment direction (importance sampling).
		valf2 Xi = Hammersley(i, SAMPLE_COUNT);
		svecf sH = SchlickGGX_Sample(Xi, roughness);
		vecf3 H = sH[0] * right + sH[1] * up + sH[2] * N;
		vecf3 L = (2.f * V.dot(H) * H - V).normalize();

		float NdotL = std::max(L[2], 0.f);
		float NdotH = std::max(H[2], 0.f);
		float VdotH = std::max(V.dot(H), 0.f);

		if (NdotL > 0.f) {
			float G = GGX_G(roughness * roughness, L, V, N);
			float G_Vis = (G * VdotH) / (NdotH * NdotV);
			float Fc = pow5(1.f - VdotH);

			A += (1.f - Fc) * G_Vis;
			B += Fc * G_Vis;
		}
	}

	return valf2{ A, B } / float(SAMPLE_COUNT);
}

int main() {
	constexpr size_t s = 256;
	Image img(s, s, 2);

	const size_t N = std::thread::hardware_concurrency();
	auto work = [&](size_t id) {
		for (size_t j = id; j < s; j += N) {
			float v = (j + 0.5f) / s;
			for (size_t i = 0; i < s; i++) {
				float u = (i + 0.5f) / s;
				auto rst = IntegrateBRDF(u, v);
				img.At(i, j, 0) = rst[0];
				img.At(i, j, 1) = rst[1];
			}
			cout << ((1 + j) / float(s)) << endl;
		}
	};
	std::vector<std::thread> workers;
	for (size_t i = 0; i < N; i++)
		workers.emplace_back(work, i);

	for (auto& worker : workers)
		worker.join();

	img.Save("BRDFLUT.png");
	
	return 0;
}
