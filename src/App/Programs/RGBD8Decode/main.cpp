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

	if (c != 4) {
		cerr << "[Error] RGBD8 image must have 4 channels." << endl;
		return -1;
	}

	Image result(w, h, 3);

	for (size_t y = 0; y < h; y++) {
		for (size_t x = 0; x < w; x++) {
			rgbaf src = img.At<rgbaf>(x, y);
			rgbf decodeColor = src.rgb.to_impl() / ((1.f - (src.a)) * 0.928571403f + 0.071428575f);
			result.At<rgbf>(x, y) = decodeColor;
		}
	}
	result.Save(filename + ".decode.hdr");

	return 0;
}
