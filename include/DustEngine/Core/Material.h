#pragma once

#include <map>
#include <string>

namespace Ubpa::DustEngine {
	struct Shader;
	struct Texture2D;
	struct TextureCube;

	struct Material {
		const Shader* shader;
		std::map<std::string, const Texture2D*> texture2Ds;
		std::map<std::string, const TextureCube*> textureCubes;
	};
}
