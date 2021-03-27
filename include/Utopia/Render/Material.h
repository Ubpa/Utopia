#pragma once

#include "ShaderProperty.h"
#include "GPURsrc.h"
#include "../Core/SharedVar.h"
#include <memory>

#include <map>
#include <string>

namespace Ubpa::Utopia {
	struct Shader;

	struct Material : GPURsrc {
		SharedVar<Shader> shader;
		std::map<std::string, ShaderProperty, std::less<>> properties;
	};
}
