#pragma once

#include "../Material.h"

#include <vector>

namespace Ubpa::Utopia {
	struct MeshRenderer {
		std::vector<const Material*> materials;
	};
}

#include "details/MeshRenderer_AutoRefl.inl"
