#include <Utopia/Core/Image.h>

#include <iostream>
#include <thread>

using namespace Ubpa::Utopia;
using namespace Ubpa;
using namespace std;

int main(int argc, const char* argv[]) {
	if (argc != 3) {
		cerr << "[Error] Command line arguments error." << endl
			<< "Usage: <base-file-name> <number: from 0 - N-1>" << endl;
		return -1;
	}

	std::string base{ argv[1] };
	size_t N = atoi(argv[2]);

	if (N > 60) {
		cerr << "[Error] image number must <= 60." << endl;
		return -1;
	}

	if (N == 0)
		return 0;

	std::vector<Image> images;
	images.reserve(N);

	auto first_path = base + "_" + std::to_string(0) + ".png";
	Image first(base + "_" + std::to_string(0) + ".png");
	if (!first.IsValid()) {
		cerr << "[Error] open image (" << first_path << ") failed." << endl;
		return -1;
	}

	const auto w = first.GetWidth();
	const auto h = first.GetHeight();

	if (first.GetChannel() != 3 && first.GetChannel() != 4) {
		cerr << "[Error] image 0's channel must be 3/4." << endl;
		return -1;
	}

	images.push_back(std::move(first));

	for (size_t i = 1; i < N; i++) {
		auto path = base + "_" + std::to_string(i) + ".png";
		Image img(base + "_" + std::to_string(i) + ".png");
		if (!img.IsValid()) {
			cerr << "[Error] open image (" << path << ") failed." << endl;
			return -1;
		}
		if (w != img.GetWidth() || h != img.GetHeight() || img.GetChannel() != 3 && img.GetChannel() != 4) {
			cerr << "[Error] image " << i << "isn't compatible." << endl
				<< "w: " << w << endl
				<< "h: " << h << endl
				<< "c: 3 / 4" << endl;
			return -1;
		}

		images.push_back(std::move(img));
	}


	std::vector<double> errors(N, 0.);
	errors[0] = 0.;
	for (size_t i = 0; i < N - 1; i++) {
		double sum_diff = 0.;
		for (size_t y = 0; y < h; y++) {
			for (size_t x = 0; x < w; x++) {
				double diff = std::abs(images[i].At(x, y).to_rgb().illumination() - images[i + 1].At(x, y).to_rgb().illumination());
				sum_diff += diff;
			}
		}
		double avg_diff = sum_diff / (w * h);
		errors[i + 1] = avg_diff;
		cout << i + 1 << ":" << avg_diff << endl;
	}
	cout << "---------------" << endl;
	for (double e: errors)
		cout << e << endl;

	return 0;
}
