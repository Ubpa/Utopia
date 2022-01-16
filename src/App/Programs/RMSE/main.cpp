#include <Utopia/Core/Image.h>

#include <iostream>
#include <thread>

using namespace Ubpa::Utopia;
using namespace Ubpa;
using namespace std;

double MSE(const Image& a, const Image& b, size_t i) {
	const auto w = a.GetWidth();
	const auto h = a.GetHeight();

	assert(w == b.GetWidth());
	assert(h == b.GetHeight());

	assert(i < a.GetChannel());
	assert(i < b.GetChannel());

	double se = 0.;

	for (size_t y = 0; y < h; y++) {
		for (size_t x = 0; x < w; x++)
			se += pow2(a.At(x, y, i) - b.At(x, y, i));
	}

	double mse = se / (w * h);

	return mse;
}

double RMSE(const Image& a, const Image& b, size_t i) {
	return sqrt(MSE(a, b, i));
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

	double acc_rmse = 0.;
	for(size_t i=0;i<c;i++)
		acc_rmse += RMSE(ref, comp, i);
	double rmse = acc_rmse / c;

	cout << "rmse : " << rmse << endl;

	return 0;
}
