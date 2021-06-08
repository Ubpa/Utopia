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

	const int skirt = 3;

	Image ref(argv[1]);
	Image comp(argv[2]);

	const auto w = ref.GetWidth();
	const auto h = ref.GetHeight();
	const auto c = ref.GetChannel();

	if (w != comp.GetWidth() || h != comp.GetHeight() || c != comp.GetChannel()) {
		cerr << "[Error] images aren't compatible." << endl;
		return -1;
	}

	Image result(w, h, 1);
	Image patch_ref(2 * skirt + 1, 2 * skirt + 1, c);
	Image patch_comp(2 * skirt + 1, 2 * skirt + 1, c);
	float maxRMSE = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			for (int yy = -skirt; yy <= skirt; yy++) {
				for (int xx = -skirt; xx <= skirt; xx++) {
					for (size_t i = 0; i < c; i++) {
						if (x + xx < 0 || x + xx >= (int)w || y + yy < 0 || y + yy >= (int)h) {
							patch_ref.At(xx + skirt, yy + skirt, i) = 0;
							patch_comp.At(xx + skirt, yy + skirt, i) = 0;
						}
						else {
							patch_ref.At(xx + skirt, yy + skirt, i) = ref.At(x+xx,y+yy,i);
							patch_comp.At(xx + skirt, yy + skirt, i) = comp.At(x + xx, y + yy, i);
						}
					}
				}
			}
			double acc_rmse = 0.;
			for (size_t i = 0; i < c; i++)
				acc_rmse += RMSE(patch_ref, patch_comp, i);
			double rmse = acc_rmse / c;
			result.At(x, y, 0) = rmse;
			maxRMSE = std::max<float>(maxRMSE, rmse);
		}
	}
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			result.At(x, y, 0) /= maxRMSE;
		}
	}
	std::cout << "max RMSE: " << maxRMSE << std::endl;

	result.Save("ppRMSE.png");

	return 0;
}
