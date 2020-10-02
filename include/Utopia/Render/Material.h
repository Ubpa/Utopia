#pragma once

#include "ShaderProperty.h"

#include <map>
#include <string>

namespace Ubpa::Utopia {
	struct Shader;

	struct Material {
		const Shader* shader;
		std::map<std::string, ShaderProperty, std::less<>> properties;
	};
}

#include "details/Material_AutoRefl.inl"
