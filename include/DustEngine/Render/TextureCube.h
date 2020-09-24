#pragma once

#include "Texture.h"
#include <UDP/Basic/Read.h>

#include <array>

namespace Ubpa::DustEngine {
	class Image;

	class TextureCube : public Texture {
	public:
		enum class SourceMode {
			SixSidedImages,
			EquirectangularMap
		};
		Read<TextureCube, SourceMode> mode;
		Read<TextureCube, std::array<const Image*, 6>> images{ nullptr };
		Read<TextureCube, const Image*> equirectangularMap{ nullptr };

		TextureCube(std::array<const Image*, 6> images);
		TextureCube(const Image* equirectangularMap);
		~TextureCube();

		void Init(std::array<const Image*, 6> images);
		void Init(const Image* equirectangularMap);
		void Clear();
	};
}
