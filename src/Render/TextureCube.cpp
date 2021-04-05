#include <Utopia/Render/TextureCube.h>

#include <Utopia/Core/Image.h>

#include <UGM/vec.hpp>

#include <cassert>
#include <thread>

using namespace Ubpa::Utopia;
using namespace Ubpa;

TextureCube::TextureCube(const std::array<Image, 6>& images) {
	Init(images);
}

TextureCube::TextureCube(const Image& equirectangularMap) {
	Init(equirectangularMap);
}

void TextureCube::Init(const std::array<Image, 6>& images) {
	Clear();
	mode = SourceMode::SixSidedImages;
	for (size_t i = 0; i < 6; i++)
		this->images[i] = images[i];
}

void TextureCube::Init(const Image& equirectangularMap) {
	Clear();

	this->equirectangularMap = equirectangularMap;

	mode = SourceMode::EquirectangularMap;
#ifdef _DEBUG
	size_t s = equirectangularMap.GetHeight() <= 512 ? equirectangularMap.GetHeight() : equirectangularMap.GetHeight() / 2;
#else
	size_t s = equirectangularMap.GetHeight();
#endif
	size_t c = equirectangularMap.GetChannel();

	// you face to the box
	// left is -x, right is +x
	// -----------------------------
	//    +y
	// -x +z +x -z
	//    -y

	vecf3 origin[6] = {
		{ 1,-1, 1}, // +x
		{-1,-1,-1}, // -x
		{-1, 1, 1}, // +y
		{-1,-1,-1}, // -y
		{-1,-1, 1}, // +z
		{ 1,-1,-1}, // -z
	};

	vecf3 right[6] = {
		{ 0, 0,-2}, // +x
		{ 0, 0, 2}, // -x
		{ 2, 0, 0}, // +y
		{ 2, 0, 0}, // -y
		{ 2, 0, 0}, // +z
		{-2, 0, 0}, // -z
	};

	vecf3 up[6] = {
		{ 0, 2, 0}, // +x
		{ 0, 2, 0}, // -x
		{ 0, 0,-2}, // +y
		{ 0, 0, 2}, // -y
		{ 0, 2, 0}, // +z
		{ 0, 2, 0}, // -z
	};

	std::array<Image, 6> imgs;
	for (size_t i = 0; i < 6; i++)
		imgs[i] = Image(s, s, c);

	const size_t N = std::thread::hardware_concurrency();
	auto work = [&](size_t id) {
		vecf2 invAtan = { 0.1591f, 0.3183f };
		for (size_t i = 0; i < 6; i++) {
			auto& img = imgs[i];
			for (size_t y = id; y < s; y += N) {
				for (size_t x = 0; x < s; x++) {
					vecf3 p = origin[i] + (x / float(s)) * right[i] + (y / float(s)) * up[i];
					p.normalize_self();
					pointf2 uv = { std::atan2(p[2], p[0]), std::asin(p[1]) };
					uv[0] = 0.5f + uv[0] * invAtan[0];
					uv[1] = 1 - (0.5f + uv[1] * invAtan[1]); // filp
					auto color = equirectangularMap.SampleLinear(uv);
					for (size_t k = 0; k < c; k++)
						img.At(x, s - 1 - y, k) = color[k]; // flip
				}
			}
		}
	};

	std::vector<std::thread> workers;
	for (size_t i = 0; i < std::thread::hardware_concurrency(); i++)
		workers.emplace_back(work, i);

	for (auto& worker : workers)
		worker.join();

	/*imgs[0].Save("pos_x.png");
	imgs[1].Save("neg_x.png");
	imgs[2].Save("pos_y.png");
	imgs[3].Save("neg_y.png");
	imgs[4].Save("pos_z.png");
	imgs[5].Save("neg_z.png");*/

	images = std::move(imgs);
}

void TextureCube::Clear() {
	for (auto& img : images)
		img.Clear();
	equirectangularMap.Clear();
}
