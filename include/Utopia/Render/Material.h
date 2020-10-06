#pragma once

#include "ShaderProperty.h"

#include <map>
#include <string>

namespace Ubpa::Utopia {
	struct Shader;

	struct Material {
		const Shader* shader{ nullptr };
		std::map<std::string, ShaderProperty, std::less<>> properties;
	};
}

#include "details/Material_AutoRefl.inl"
