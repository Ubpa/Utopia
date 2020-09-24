#pragma once

#include <string>

namespace Ubpa::DustEngine {
	struct ShaderPass {
		std::string vertexName; // e.g. vert
		std::string fragmentName; // e.g. frag
	};
}

#include "details/ShaderPass_AutoRefl.inl"
