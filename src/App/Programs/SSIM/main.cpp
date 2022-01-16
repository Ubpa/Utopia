#include <Utopia/Core/Image.h>

#include <iostream>
#include <thread>

using namespace Ubpa::Utopia;
using namespace Ubpa;
using namespace std;

double Mean(const Image& img, size_t i) {
	const auto w = img.GetWidth();
	const auto h = img.GetHeight();
	assert(i < img.GetChannel());


	double acc = 0.;

	for (size_t y = 0; y < h; y++) {
		for (size_t x = 0; x < w; x++)
			acc += img.At(x, y, i);
	}

	return acc / (w * h);
}

double Variance(const Image& img, size_t i) {
	const auto w = img.GetWidth();
	const auto h = img.GetHeight();
	assert(i < img.GetChannel());

	const double mean = Mean(img, i);
	double se = 0.;
	for (size_t y = 0; y < h; y++) {
		for (size_t x = 0; x < w; x++)
			se += pow2(img.At(x, y, i) - mean);
	}

	return se / (w * h - 1);
}

double StdVariance(const Image& img, size_t i) {
	return sqrt(Variance(img, i));
}

double Covariance(const Image& a, const Image& b, size_t i) {
	const auto w = a.GetWidth();
	const auto h = a.GetHeight();

	assert(w == b.GetWidth());
	assert(h == b.GetHeight());

	assert(i < a.GetChannel());
	assert(i < b.GetChannel());

	const double mean_a = Mean(a, i);
	const double mean_b = Mean(b, i);

	double coe = 0.;
	for (size_t y = 0; y < h; y++) {
		for (size_t x = 0; x < w; x++)
			coe += (a.At(x, y, i) - mean_a) * (b.At(x, y, i) - mean_b);
	}

	return coe / (w * h - 1);
}

double SSIM(const Image& a, const Image& b, size_t i) {
	const double C1 = 0.0001;
	const double C2 = 0.0009;
	const double C3 = C2 / 2;

	const double mu_x = Mean(a, i);
	const double mu_y = Mean(b, i);
	const double sigma_x = StdVariance(a, i);
	const double sigma_y = StdVariance(b, i);
	const double sigma_xy = Covariance(a, b, i);

	double L = (2 * mu_x * mu_y + C1) / (pow2(mu_x) + pow2(mu_y) + C1);
	double C = (2 * sigma_xy + C2) / (pow2(sigma_x) + pow2(sigma_y) + C2);
	double S = (sigma_xy + C3) / (sigma_x * sigma_y + C3);

	double ssim = L * C * S;

	return ssim;
}

int main(int argc, const char* argv[]) {
	if (argc != 3) {
		cerr << "[Error] Command line arguments error." << endl
			<< "Usage: <ref-file-name> <compare-file-name>" << endl;
		return -1;
	}
	
	Image ref(argv[1]);
	Image comp(argv[2]);

	const auto w = ref.GetWidth();
	const auto h = ref.GetHeight();
	const auto c = ref.GetChannel();

	if (w != comp.GetWidth() || h != comp.GetHeight() || c != comp.GetChannel()) {
		cerr << "[Error] images aren't compatible." << endl;
		return -1;
	}

	double acc_ssim = 0.;
	for (size_t i = 0; i < c; i++)
		acc_ssim += SSIM(ref, comp, i);
	double ssim = acc_ssim / c;

	cout << "ssim: " << ssim << endl;

	return 0;
}
