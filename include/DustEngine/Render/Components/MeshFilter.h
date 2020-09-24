#pragma once

#include "../Mesh.h"

#include <vector>

namespace Ubpa::DustEngine {
	struct MeshFilter {
		Mesh* mesh{ nullptr };
	};
}

#include "details/MeshFilter_AutoRefl.inl"
