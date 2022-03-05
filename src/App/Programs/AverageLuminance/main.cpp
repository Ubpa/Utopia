#include <Utopia/Core/Image.h>

#include <iostream>

using namespace Ubpa::Utopia;
using namespace Ubpa;
using namespace std;

int main(int argc, const char* argv[]) {
	if (argc != 2) {
		cerr << "[Error] Command line arguments error." << endl
			<< "Usage: <input-file-name>" << endl;
		return -1;
	}
	
	std::string filename(argv[1]);

	Image img(filename);

	const auto w = img.GetWidth();
	const auto h = img.GetHeight();
	const auto c = img.GetChannel();

	if (c != 3) {
		cerr << "[Error] image must have 3 channels." << endl;
		return -1;
	}

	rgbd sumColor(0.);

	for (size_t y = 0; y < h; y++) {
		for (size_t x = 0; x < w; x++)
			sumColor += img.At<rgbf>(x, y).cast_to<rgbd>();
	}
	sumColor /= w * h;
	double luminance = sumColor.illumination();

	cout << sumColor << endl;
	cout << luminance << endl;

	return 0;
}
