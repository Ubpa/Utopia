#pragma once

#include "Texture.h"
#include "../Core/Image.h"

namespace Ubpa::Utopia {
	struct Texture2D : Texture {
		Texture2D() = default;
		Texture2D(Image image) : image{ std::move(image) } {}
		Image image;
	};
}
