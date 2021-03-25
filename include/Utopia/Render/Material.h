#pragma once

#include "ShaderProperty.h"
#include "GPURsrc.h"
#include <memory>

#include <map>
#include <string>

namespace Ubpa::Utopia {
	struct Shader;

	struct Material : GPURsrc {
		std::shared_ptr<const Shader> shader;
		std::map<std::string, ShaderProperty, std::less<>> properties;
	};
}
