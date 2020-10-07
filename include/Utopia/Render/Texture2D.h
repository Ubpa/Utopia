#pragma once

#include "Texture.h"
#include <memory>

namespace Ubpa::Utopia {
	class Image;

	struct Texture2D : Texture {
		std::shared_ptr<const Image> image;
	};
}
