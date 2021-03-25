#pragma once

#include "Texture.h"
#include "../Core/Image.h"

namespace Ubpa::Utopia {
	struct Texture2D : Texture {
		Image image;
	};
}
