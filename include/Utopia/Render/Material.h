#pragma once

#include "ShaderProperty.h"
#include "../Core/Object.h"

#include <map>
#include <string>

namespace Ubpa::Utopia {
	struct Shader;

	struct Material : Object {
		std::shared_ptr<const Shader> shader;
		std::map<std::string, ShaderProperty, std::less<>> properties;
	};
}

#include "../Core/details/Object_AutoRefl.inl"
#include "details/Material_AutoRefl.inl"
