#pragma once

#include "RenderRsrcObject.h"
#include "ShaderPass.h"
#include "RootParameter.h"

#include <string>
#include <vector>

namespace Ubpa::DustEngine {
	class HLSLFile;

	struct Shader : RenderRsrcObject {
		const HLSLFile* hlslFile{ nullptr };
		std::string shaderName; // e.g. a/b/c/d
		std::string targetName; // e.g. 5_0
		std::vector<RootParameter> rootParameters;
		std::vector<ShaderPass> passes;
	};
}

#include "details/Shader_AutoRefl.inl"
