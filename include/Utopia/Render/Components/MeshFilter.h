#pragma once

#include "../Mesh.h"

namespace Ubpa::Utopia {
	struct MeshFilter {
		std::shared_ptr<Mesh> mesh;
	};
}

#include "details/MeshFilter_AutoRefl.inl"
