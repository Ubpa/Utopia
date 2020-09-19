#pragma once

#include <map>
#include <string>

namespace Ubpa::DustEngine {
	struct Shader;
	struct Texture2D;
	class TextureCube;

	struct Material {
		const Shader* shader;
		std::map<std::string, const Texture2D*> texture2Ds;
		std::map<std::string, const TextureCube*> textureCubes;
	};
}

#include "details/Material_AutoRefl.inl"
