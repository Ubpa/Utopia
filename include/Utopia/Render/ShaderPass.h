#pragma once

#include "RenderState.h"

#include <string>
#include <map>

namespace Ubpa::Utopia {
	struct ShaderPass {
		std::string vertexName;
		std::string fragmentName;
		RenderState renderState;
		std::map<std::string, std::string, std::less<>> tags;

		// Background 1000
		// Geometry (default) 2000
		// AlphaTest 2450
		// Transparent 2500
		// Overlay 4000
		//  < 2500 : "opaque", optimize the drawing order
		// >= 2500 : "transparent", in back-to-front order
		enum class Queue : size_t {
			Background = 1000,
			Geometry = 2000,
			AlphaTest = 2450,
			Transparent = 2500,
			Overlay = 4000
		};
		size_t queue{ 2000 };
	};
}

#include "details/ShaderPass_AutoRefl.inl"
