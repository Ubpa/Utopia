#pragma once

#include "Texture.h"
#include <memory>

namespace Ubpa::Utopia {
	class Image;

	struct Texture2D : Texture {
		enum class WrapMode { Clamp, Repeat, Mirror };
		enum class FilterMode { Point, Bilinear };

		std::shared_ptr<const Image> image;
		WrapMode wrapMode{ WrapMode::Repeat };
		FilterMode filterMode{ FilterMode::Bilinear };
	};
}
