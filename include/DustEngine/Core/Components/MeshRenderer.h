#pragma once

#include "../Material.h"

#include <vector>

namespace Ubpa::DustEngine {
	struct MeshRenderer {
		std::vector<const Material*> materials;
	};
}

#include "details/MeshRenderer_AutoRefl.inl"
