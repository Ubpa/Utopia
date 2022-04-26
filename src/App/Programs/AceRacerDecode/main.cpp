#include <Utopia/Core/Image.h>

#include <iostream>

using namespace Ubpa::Utopia;
using namespace Ubpa;
using namespace std;

template<size_t N>
void ReadVal(valf<N>& val, const char* argv[], size_t& idx) {
	for (size_t i = 0; i < N; i++)
		val[i] = atof(argv[idx++]);
}

int main(int argc, const char* argv[]) {
	if (argc != 1 + 40) {
		cerr << "[Error] Command line arguments error." << endl
			<< "Usage: <input-file-name>" << endl   // 1
			<< "       <ColorBasisR/G/B>" << endl   // 12
			<< "       <BaseColor>" << endl         // 3
			<< "       <NormalBasisX/Y>" << endl    // 8
			<< "       <RoughnessBasis>" << endl    // 4
			<< "       <MetalnessBasis>" << endl    // 4
			<< "       <BasisOffset>" << endl       // 4
			<< "       <AOBasisAndOffset>" << endl; // 4
													// = 40
		return -1;
	}

	std::string filename(argv[1]);

	Image inputImg(filename);

	const auto w = inputImg.GetWidth();
	const auto h = inputImg.GetHeight();
	const auto c = inputImg.GetChannel();

	valf4 ColorBasisR, ColorBasisG, ColorBasisB;
	valf3 BaseColor;
	valf4 NormalBasisX, NormalBasisY;
	valf4 RoughnessBasis, MetalnessBasis;
	valf4 BasisOffset; // x: NormalBasisX Offset, y: NormalBasisY Offset, z: RoughnessBasis Offset, w: MetalnessBasis Offset
	valf4 AOBasisAndOffset; // xyz: AO Basis, w: AO Offset

	size_t idx = 2;
	ReadVal(ColorBasisR, argv, idx);
	ReadVal(ColorBasisG, argv, idx);
	ReadVal(ColorBasisB, argv, idx);
	ReadVal(BaseColor, argv, idx);
	ReadVal(NormalBasisX, argv, idx);
	ReadVal(NormalBasisY, argv, idx);
	ReadVal(RoughnessBasis, argv, idx);
	ReadVal(MetalnessBasis, argv, idx);
	ReadVal(BasisOffset, argv, idx);
	ReadVal(AOBasisAndOffset, argv, idx);

	if (c != 4) {
		cerr << "[Error] image must have 4 channels." << endl;
		return -1;
	}

	Image basecolorImg(w, h, 3);
	Image normalImg(w, h, 3);
	Image roughnessImg(w, h, 1);
	Image metalnessImg(w, h, 1);
	Image aoImg(w, h, 1);

	for (size_t y = 0; y < h; y++) {
		for (size_t x = 0; x < w; x++) {
			const valf4 coeffs = 2.f * inputImg.At(x, y).cast_to<valf4>() - 1.f;
			valf3 color;
			valf2 normalXY;
			valf3 normal;
			float roughness;
			float metalness;
			float ao;

			color = BaseColor;
			color.r += coeffs.dot(ColorBasisR);
			color.g += coeffs.dot(ColorBasisG);
			color.b += coeffs.dot(ColorBasisB);
			normalXY.x = coeffs.dot(NormalBasisX) + BasisOffset.x;
			normalXY.y = coeffs.dot(NormalBasisY) + BasisOffset.y;
			roughness = coeffs.dot(RoughnessBasis) + BasisOffset.z;
			metalness = coeffs.dot(MetalnessBasis) + BasisOffset.w;
			ao = coeffs.wxy.to_impl().dot(AOBasisAndOffset.xyz) + AOBasisAndOffset.w;

			normal.xy = 2.f * normalXY - 1.f;
			normal.z = 1.f - std::clamp(std::sqrt(normal.xy.to_impl().norm2()), 0.f, 1.f);

			basecolorImg.At<rgbf>(x, y) = color;
			normalImg.At<rgbf>(x, y) = normal * 0.5f + 0.5f;
			roughnessImg.At<float>(x, y) = roughness;
			metalnessImg.At<float>(x, y) = metalness;
			aoImg.At<float>(x, y) = ao;
		}
	}

	basecolorImg.Save(filename + ".basecolor.png");
	normalImg.Save(filename + ".normal.png");
	roughnessImg.Save(filename + ".roughness.png");
	metalnessImg.Save(filename + ".metalness.png");
	aoImg.Save(filename + ".ao.png");

	return 0;
}
