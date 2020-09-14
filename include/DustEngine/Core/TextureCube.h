#pragma once

#include "Texture.h"

#include <array>

namespace Ubpa::DustEngine {
	class Image;

	struct TextureCube : Texture {
		std::array<const Image*, 6> images;
	};
}
