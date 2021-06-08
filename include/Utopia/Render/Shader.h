#pragma once

#include "ShaderPass.h"
#include "ShaderProperty.h"
#include "RootParameter.h"

#include "GPURsrc.h"

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace Ubpa::Utopia {
	class HLSLFile;

	struct Shader : GPURsrc {
		SharedVar<HLSLFile> hlslFile;
		std::string name; // e.g. a/b/c/d
		std::vector<RootParameter> rootParameters;
		std::map<std::string, ShaderProperty, std::less<>> properties;
		std::vector<ShaderPass> passes;
	};
}
