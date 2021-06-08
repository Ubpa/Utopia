#include <Utopia/Core/Image.h>

#include <iostream>
#include <thread>

using namespace Ubpa::Utopia;
using namespace Ubpa;
using namespace std;

int main(int argc, const char* argv[]) {
	if (argc != 2) {
		cerr << "[Error] Command line arguments error." << endl
			<< "Usage: <input-file-name>" << endl;
		return -1;
	}
	
	Image img(argv[1]);

	const auto w = img.GetWidth();
	const auto h = img.GetHeight();
	const auto c = img.GetChannel();

	const char* names[] = {
		"r","g","b","a"
	};

	for (size_t i = 0; i < c; i++) {
		Image rst(w, h, 1);
		for (size_t y = 0; y < h; y++) {
			for (size_t x = 0; x < w; x++)
				rst.At(x, y, 0) = img.At(x, y, i);
		}

		string path = string(argv[1]) + "." + string(names[i]) + ".png";
		rst.Save(path);
		cout << names[i] << " : " << path << endl;
	}

	return 0;
}
