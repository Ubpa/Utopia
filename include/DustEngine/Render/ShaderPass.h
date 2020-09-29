#pragma once

#include "RenderState.h"

#include <string>
#include <map>

namespace Ubpa::DustEngine {
	struct ShaderPass {
		std::string vertexName; // e.g. vert
		std::string fragmentName; // e.g. frag
		RenderState renderState;
		std::map<std::string, std::string, std::less<>> tags;
	};
}

#include "details/ShaderPass_AutoRefl.inl"
