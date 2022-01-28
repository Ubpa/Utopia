#include <Utopia/Core/Image.h>

#include <UGM/UGM.hpp>

#include <iostream>
#include <thread>

using namespace Ubpa::Utopia;
using namespace Ubpa;
using namespace std;

using svecd = svec<double>;

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

svecd SchlickGGX_Sample(valf2 Xi, double roughness) {
	double a = roughness * roughness;

	double phi = 2 * Pi<double> * Xi[0];
	double cosTheta = sqrt((1.f - Xi[1]) / (1.f + (a * a - 1.f) * Xi[1]));
	double sinTheta = sqrt(1.f - cosTheta * cosTheta);

	// from spherical coordinates to cartesian coordinates - halfway vector
	svecd H{
		cos(phi) * sinTheta,
		sin(phi) * sinTheta,
		cosTheta
	};

	return H;
}
// ref: Sampling the GGX Distribution of Visible Normals
//      http://jcgt.org/published/0007/04/01/paper.pdf
// ---------------------------------------------------------------
// Input Ve: view direction
// Input alpha: roughness parameters
// Input U1, U2: uniform random numbers
// Output Ne: normal sampled with PDF D_Ve(Ne) = G1(Ve) * max(0, dot(Ve, Ne)) * D(Ne) / Ve.z
svecd SampleGGXVNDF(svecd Ve, double alpha, float U1, float U2)
{
	// Section 3.2: transforming the view direction to the hemisphere configuration
	svecd Vh = svecd(alpha * Ve.x, alpha * Ve.y, Ve.z).normalize();
	// Section 4.1: orthonormal basis (with special case if cross product is zero)
	double lensq = Vh.x * Vh.x + Vh.y * Vh.y;
	svecd T1 = lensq > 0 ? svecd(-Vh.y, Vh.x, 0) * (1/std::sqrt(lensq)) : svecd(1, 0, 0);
	svecd T2 = Vh.cross(T1);
	// Section 4.2: parameterization of the projected area
	double r = sqrt(U1);
	double phi = 2.0 * Pi<double> * U2;
	double t1 = r * cos(phi);
	double t2 = r * sin(phi);
	double s = 0.5 * (1.0 + Vh.z);
	t2 = (1.0 - s) * sqrt(1.0 - t1 * t1) + s * t2;
	// Section 4.3: reprojection onto hemisphere
	svecd Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2)) * Vh;
	// Section 3.4: transforming the normal back to the ellipsoid configuration
	svecd Ne = svecd(alpha * Nh.x, alpha * Nh.y, std::max(0., Nh.z)).normalize();
	return Ne;
}

double GGX_G1(double alpha, double NoV) {
	double tan2_sthetao = 1 / pow2(NoV) - 1;
	return 2 / (1 + sqrt(1 + pow2(alpha) * tan2_sthetao));
}
double GGX_D(double alpha, double NoH) {
	double alpha2 = alpha * alpha;
	double x = 1 + (alpha2 - 1) * NoH * NoH;
	double denominator = Pi<double> * x * x;
	return alpha2 / denominator;
}
double PDF_VNDF(double alpha, double NoV, double NoH, double HoV) {
	return GGX_G1(alpha, NoV) * GGX_D(alpha, NoH) / (4 * NoV);
	//double cos2_sthetao = Pow2(HoV);
	//double sin2_sthetao = 1 - cos2_sthetao;
	//double alpha2 = Pow2(alpha);
	//return GGX_D(alpha, NoH) / (2 * (HoV + sqrt(cos2_sthetao + alpha2 * sin2_sthetao)));
}

double GGX_G(double alpha, double cos_sthetai, double cos_sthetao) {
	double alpha2 = alpha * alpha;

	if (cos_sthetai < 0 || cos_sthetao < 0)
		return 0.f;

	double tan2_sthetai = 1.f / (cos_sthetai * cos_sthetai) - 1.f;
	double tan2_sthetao = 1.f / (cos_sthetao * cos_sthetao) - 1.f;

	return 2.f / (sqrt(1.f + alpha2 * tan2_sthetai) + sqrt(1.f + alpha2 * tan2_sthetao));
}

double IntegrateBRDF(double NdotV, double roughness) {
	double alpha = pow2(roughness);
	// 由于各向同性，随意取一个 V 即可
	svecd V;
	V[0] = sqrt(1. - NdotV * NdotV);
	V[1] = 0.;
	V[2] = NdotV;

	double A = 0.f;
	double B = 0.f;
	double C = 0.f;

	svecd N{ 0,0,1 };
	svecd right{ 1,0,0 };
	svecd up{ 0,1,0 };

	const unsigned int SAMPLE_COUNT = 16384u;
	for (unsigned int i = 0u; i < SAMPLE_COUNT; ++i) {
		valf2 Xi = Hammersley(i, SAMPLE_COUNT);
		svecd sH = SampleGGXVNDF(V, alpha, Xi.x, Xi.y);
		svecd H = svecd(sH[0] * right + sH[1] * up + sH[2] * N).normalize();
		svecd L = (2.f * V.dot(H) * H - V).normalize();

		double NdotL = L.cos_stheta();
		double NdotV = V.cos_stheta();
		double NdotH = H.cos_stheta();
		double VdotH = std::max(V.dot(H), 0.);

		if (NdotL > 0.f) {
			double G = GGX_G(alpha, NdotL, NdotV);
			double G_Vis = G / GGX_G1(alpha, NdotV);
			double Fc = pow5(1.f - VdotH);

			//A += (1.f - Fc) * G_Vis;
			//B += Fc * G_Vis;
			C += G_Vis;
		}

	}

	//return valf2{ A / double(SAMPLE_COUNT), B / double(SAMPLE_COUNT) };
	return 1.0 - C / double(SAMPLE_COUNT);
}

int main() {
	constexpr size_t s = 128;
	Image img(s, s, 1);

	const size_t N = std::thread::hardware_concurrency();
	auto work = [&](size_t id) {
		for (size_t j = id; j < s; j += N) {
			double v = (j + 0.5) / s;
			for (size_t i = 0; i < s; i++) {
				double u = (i + 0.5) / s;
				auto rst = IntegrateBRDF(u, v);
				img.At(i, j, 0) = static_cast<float>(rst);
			}
			cout << ((1 + j) / double(s)) << endl;
		}
	};
	std::vector<std::thread> workers;
	for (size_t i = 0; i < N; i++)
		workers.emplace_back(work, i);

	for (auto& worker : workers)
		worker.join();

	img.Save("BRDFCLUT.png");
	
	return 0;
}
