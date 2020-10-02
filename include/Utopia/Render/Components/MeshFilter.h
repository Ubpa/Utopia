#pragma once

#include "../Mesh.h"

#include <vector>

namespace Ubpa::Utopia {
	struct MeshFilter {
		Mesh* mesh{ nullptr };
	};
}

#include "details/MeshFilter_AutoRefl.inl"
