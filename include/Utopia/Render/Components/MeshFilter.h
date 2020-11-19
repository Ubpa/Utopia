#pragma once

#include "../Mesh.h"
#include <USTL/memory.h>

namespace Ubpa::Utopia {
	struct MeshFilter {
		USTL::shared_object<Mesh> mesh;
	};
}

#include "details/MeshFilter_AutoRefl.inl"
