#pragma once

#include "Object.h"

#include <string>

namespace Ubpa::DustEngine {
	class HLSLFile;

	struct Shader : Object {
		const HLSLFile* hlslFile;
		std::string shaderName; // e.g. a/b/c/d
		std::string vertexName; // e.g. vert
		std::string fragmentName; // e.g. frag
		std::string targetName; // e.g. 5_0
	};
}
