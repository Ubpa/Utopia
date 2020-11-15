#pragma once

#include "../Core/Object.h"
#include "ShaderPass.h"
#include "ShaderProperty.h"
#include "RootParameter.h"

#include <string>
#include <vector>
#include <map>

namespace Ubpa::Utopia {
	class HLSLFile;

	struct Shader : Object {
		std::shared_ptr<const HLSLFile> hlslFile;
		std::string name; // e.g. a/b/c/d
		std::vector<RootParameter> rootParameters;
		std::map<std::string, ShaderProperty, std::less<>> properties;
		std::vector<ShaderPass> passes;
	};
}

#include "../Core/details/Object_AutoRefl.inl"
#include "details/Shader_AutoRefl.inl"
