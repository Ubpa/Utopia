#pragma once

#include "Texture.h"
#include "../Core/Image.h"

#include <memory>
#include <array>

namespace Ubpa::Utopia {
	class TextureCube : public Texture {
	public:
		enum class SourceMode {
			SixSidedImages,
			EquirectangularMap
		};

		TextureCube(const std::array<Image, 6>& images);
		TextureCube(const Image& equirectangularMap);

		void Init(const std::array<Image, 6>& images);
		void Init(const Image& equirectangularMap);
		void Clear();

		SourceMode GetSourceMode() const noexcept { return mode; }
		const auto& GetSixSideImages() const noexcept { return images; }
		const auto& GetEquiRectangularMap() const noexcept { return equirectangularMap; }

	private:
		SourceMode mode;
		std::array<Image, 6> images;
		Image equirectangularMap;
	};
}
