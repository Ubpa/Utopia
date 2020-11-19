#pragma once

#include "../Material.h"
#include <USTL/memory.h>

#include <vector>

namespace Ubpa::Utopia {
	struct MeshRenderer {
		std::vector<USTL::shared_object<Material> > materials;
	};
}

#include "details/MeshRenderer_AutoRefl.inl"
