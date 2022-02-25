#include <Utopia/Core/Image.h>

#include <iostream>

using namespace Ubpa::Utopia;
using namespace Ubpa;
using namespace std;

int main(int argc, const char* argv[]) {
	if (argc != 3) {
		cerr << "[Error] Command line arguments error." << endl
			<< "Usage: <input-file-name> <max-value>" << endl;
		return -1;
	}
	
	std::string filename(argv[1]);

	Image img(filename);

	float max_value = std::atof(argv[2]);

	const auto w = img.GetWidth();
	const auto h = img.GetHeight();
	const auto c = img.GetChannel();

	if (c != 4) {
		cerr << "[Error] RGBM image must have 4 channels." << endl;
		return -1;
	}

	Image result(w, h, 3);

	for (size_t y = 0; y < h; y++) {
		for (size_t x = 0; x < w; x++)
			result.At<rgbf>(x, y) = img.At<rgbaf>(x, y).to_rgb() * max_value;
	}
	result.Save(filename + ".decode.png");

	return 0;
}
