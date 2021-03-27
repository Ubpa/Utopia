#pragma once

#include <UGM/rgba.hpp>
#include <UGM/point.hpp>

#include <string>

namespace Ubpa::Utopia {
	class Image {
	public:
		Image() = default;
		~Image();
		Image(const std::string& path, bool flip = false);
		Image(size_t width, size_t height, size_t channel);
		Image(size_t width, size_t height, size_t channel, const float* data);
		Image(Image&& image) noexcept;
		Image(const Image& image);
		Image& operator=(Image&& image) noexcept;
		Image& operator=(const Image& image);

		bool Init(const std::string& path, bool flip = false);
		void Init(size_t width, size_t height, size_t channel);
		void Init(size_t width, size_t height, size_t channel, const float* data);
		
		template<typename T, // float, rgbf or rgbaf
			typename = std::enable_if_t<
			std::is_same_v<T, float>
			|| std::is_same_v<T, rgbf>
			|| std::is_same_v<T, rgbaf>>>
		void SetAll(const T& color);
		// png, bmp, tga, jpg, hdr
		bool Save(const std::string& path, bool flip = false) const;
		void Clear();

		bool IsValid() const noexcept;

		float& At(size_t x, size_t y, size_t c);
		const float& At(size_t x, size_t y, size_t c) const;
		const rgbaf At(size_t x, size_t y) const;
		template<typename T, // float, rgbf or rgbaf
			typename = std::enable_if_t<
			std::is_same_v<T, float>
			|| std::is_same_v<T, rgbf>
			|| std::is_same_v<T, rgbaf>>>
		T& At(size_t x, size_t y);
		template<typename T, // float, rgbf or rgbaf
			typename = std::enable_if_t<
			std::is_same_v<T, float>
			|| std::is_same_v<T, rgbf>
			|| std::is_same_v<T, rgbaf>>>
		const T& At(size_t x, size_t y) const;

		const rgbaf SampleNearest(const pointf2& uv) const;
		const rgbaf SampleLinear(const pointf2& uv) const;

		float* GetData() noexcept { return data; }
		const float* GetData() const noexcept { return data; }
		size_t GetWidth() const noexcept { return width; }
		size_t GetHeight() const noexcept { return height; }
		size_t GetChannel() const noexcept { return channel; }

		friend bool operator==(const Image& lhs, const Image& rhs) noexcept;
		friend bool operator!=(const Image& lhs, const Image& rhs) noexcept;
	private:
		float* data{ nullptr };
		size_t width{ static_cast<size_t>(0) };
		size_t height{ static_cast<size_t>(0) };
		size_t channel{ static_cast<size_t>(0) };
	};
}

#include "details/Image.inl"
