#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS 1
#endif // WIN32

#include <Utopia/Core/Image.h>

#define STB_IMAGE_IMPLEMENTATION
#include "_deps/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "_deps/stb_image_write.h"

#include <UGM/point.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa;
using namespace std;

Image::~Image() {
	delete data;
}

Image::Image(const std::string& path, bool flip) {
	Init(path, flip);
}

Image::Image(size_t width, size_t height, size_t channel) {
	Init(width, height, channel);
}

Image::Image(size_t width, size_t height, size_t channel, const float* data) {
	Init(width, height, channel, data);
}

Image::Image(Image&& image) noexcept :
	data{ image.data },
	width{ image.width },
	height{ image.height },
	channel{ image.channel }
{
	image.data = nullptr;
	image.Clear();
}

Image::Image(const Image& image) :
	width{ image.width },
	height{ image.height },
	channel{ image.channel }
{
	data = new float[width * height * channel];
	memcpy(data, image.data, width* height* channel * sizeof(float));
}

Image& Image::operator=(Image&& image) noexcept {
	if (image.data != nullptr && image.data == data)
		return *this;
	delete data;
	data = image.data;
	width = image.width;
	height = image.height;
	channel = image.channel;
	image.data = nullptr;
	return *this;
}

Image& Image::operator=(const Image& image) {
	if (image.data != nullptr && image.data == data)
		return *this;
	delete data;
	width = image.width;
	height = image.height;
	channel = image.channel;
	data = new float[width * height * channel];
	memcpy(data, image.data, width * height * channel * sizeof(float));
	return *this;
}

bool Image::Init(const std::string& path, bool flip) {
	int w, h, c;

	stbi_set_flip_vertically_on_load(static_cast<int>(flip));

	if (path.size() > 4 && path.substr(path.size() - 4, 4) == ".hdr") {
		float* stbi_data = stbi_loadf(path.data(), &w, &h, &c, 0);
		if (!stbi_data)
			return false;
		Init(w, h, c, stbi_data);
		stbi_image_free(stbi_data);
	}
	else {
		auto stbi_data = stbi_load(path.c_str(), &w, &h, &c, 0);
		if (!stbi_data)
			return false;

		width = static_cast<size_t>(w);
		height = static_cast<size_t>(h);
		channel = static_cast<size_t>(c);;
		data = new float[width * height * channel];
		for (size_t i = 0; i < width * height * channel; i++)
			data[i] = static_cast<float>(stbi_data[i]) / 255.f;

		stbi_image_free(stbi_data);
	}

	return true;
}

void Image::Init(size_t width, size_t height, size_t channel) {
	Clear();
	this->width = width;
	this->height = height;
	this->channel = channel;
	data = new float[width * height * channel]{ 0.f };
}

void Image::Init(size_t width, size_t height, size_t channel, const float* data) {
	Clear();
	this->width = width;
	this->height = height;
	this->channel = channel;
	this->data = new float[width * height * channel];
	memcpy(this->data, data, width * height * channel * sizeof(float));
}

bool Image::Save(const std::string& path, bool flip) const {
	assert(IsValid());
	assert(!path.empty());

	string supports[5] = { "png", "bmp", "tga", "jpg", "hdr" };

	size_t k = static_cast<size_t>(-1);
	for (size_t i = 0; i < 5; i++) {
		const auto& postfix = supports[i];
		if (path.size() < postfix.size())
			continue;
		if (path.substr(path.size() - postfix.size(), postfix.size()) == postfix) {
			k = i;
			break;
		}
	}

	int w = static_cast<int>(width);
	int h = static_cast<int>(height);
	int c = static_cast<int>(channel);
	stbi_flip_vertically_on_write(static_cast<int>(flip));
	if (k < 4) {
		size_t num = width * height * channel;
		stbi_uc* stbi_data = new stbi_uc[num];
		for (size_t i = 0; i < num; i++)
			stbi_data[i] = static_cast<stbi_uc>(std::clamp(data[i] * 255.f, 0.f, 255.f));

		int rst;
		if (k == 0)
			rst = stbi_write_png(path.c_str(), w, h, c, stbi_data, w * c);
		else if (k == 1)
			rst = stbi_write_bmp(path.c_str(), w, h, c, stbi_data);
		else if (k == 2)
			rst = stbi_write_tga(path.c_str(), w, h, c, stbi_data);
		else
			rst = stbi_write_jpg(path.c_str(), w, h, c, stbi_data, 75);

		delete[] stbi_data;

		if (rst == 0)
			return false;
	}
	else if (k == 4) {
		int rst = stbi_write_hdr(path.c_str(), w, h, c, data);
		if (rst == 0)
			return false;
	}
	else
		return false;

	return true;
}

void Image::Clear() {
	delete data;
	data = nullptr;
	width = static_cast<size_t>(0);
	height = static_cast<size_t>(0);
	channel = static_cast<size_t>(0);
}

bool Image::IsValid() const noexcept {
	return data != nullptr;
}

float& Image::At(size_t x, size_t y, size_t c) {
	assert(IsValid());
	assert(x < width&& y < height&& c < channel);
	return data[(y * width + x) * channel + c];
}

const float& Image::At(size_t x, size_t y, size_t c) const {
	assert(IsValid());
	assert(x < width&& y < height&& c < channel);
	return data[(y * width + x) * channel + c];
}

const rgbaf Image::At(size_t x, size_t y) const {
	assert(IsValid());
	assert(x < width&& y < height);

	rgbaf rst{ 0.f,0.f,0.f,1.f };

	size_t offset = (y * width + x) * channel;
	for (size_t i = 0; i < channel; i++)
		rst[i] = data[offset + i];

	return rst;
}

const rgbaf Image::SampleNearest(const pointf2& uv) const {
	const float u = uv[0];
	const float v = uv[1];
	float xf = width * u - 0.5f; // [-0.5f, width - 0.5f]
	float yf = height * v - 0.5f; // [-0.5f, height - 0.5f]
	auto x = static_cast<size_t>(std::round(xf));
	auto y = static_cast<size_t>(std::round(yf));
	return At(x, y);
}

const rgbaf Image::SampleLinear(const pointf2& uv) const {
	const float u = uv[0];
	const float v = uv[1];

	assert(IsValid());
	assert(0 <= u && u <= 1);
	assert(0 <= v && v <= 1);

	float xf = width * u - 0.5f; // [-0.5f, width - 0.5f]
	float yf = height * v - 0.5f; // [-0.5f, height - 0.5f]

	size_t x0 = static_cast<size_t>(std::max(xf, 0.f)); // [0, width - 1]
	size_t y0 = static_cast<size_t>(std::max(yf, 0.f)); // [0, height - 1]

	size_t x1 = std::min(x0 + 1, width - 1);
	size_t y1 = std::min(y0 + 1, height - 1);

	float tx = xf - x0;
	float ty = yf - y0;

	const rgbaf c00 = At(x0, y0);
	const rgbaf c01 = At(x1, y0);
	const rgbaf c10 = At(x0, y1);
	const rgbaf c11 = At(x1, y1);

	rgbaf c0x = rgbaf::lerp(c00, c01, tx);
	rgbaf c1x = rgbaf::lerp(c10, c10, tx);
	rgbaf cyx = rgbaf::lerp(c0x, c1x, ty);

	return cyx;
}

namespace Ubpa::Utopia {
	bool operator==(const Image& lhs, const Image& rhs) noexcept {
		if (lhs.width != rhs.width
			|| lhs.height != rhs.height
			|| lhs.channel != rhs.channel)
			return false;

		std::size_t cnt = lhs.width * lhs.height * lhs.channel;
		for (std::size_t i = 0; i < cnt; i++) {
			if (lhs.data[i] != rhs.data[i])
				return false;
		}

		return true;
	}
	bool operator!=(const Image& lhs, const Image& rhs) noexcept {
		return !(lhs == rhs);
	}
}
