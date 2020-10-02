#pragma once

namespace Ubpa::Utopia {
	template<typename T, typename>
	T& Image::At(size_t x, size_t y) {
		assert(T::N == channel);
		return reinterpret_cast<T&>(At(x, y, 0));
	}

	template<typename T, typename>
	const T& Image::At(size_t x, size_t y) const {
		return const_cast<Image*>(this)->At<T>(x, y);
	}

	template<typename T, typename>
	void Image::SetAll(const T& color) {
		if constexpr (std::is_same_v<T, float>) {
			for (size_t i = 0; i < width * height * channel; i++)
				data[i] = color;
		}
		else {
			for (size_t j = 0; j < height; j++) {
				for (size_t i = 0; i < width; i++) {
					At<T>(i, j) = color;
				}
			}
		}
	}
}
