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

	img.Save(filename + ".png");

	return 0;
}
