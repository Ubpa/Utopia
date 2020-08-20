#pragma once

#include "../Material.h"

#include <vector>

namespace Ubpa::DustEngine {
	struct MeshRenderer {
		std::vector<const Material*> material;
	};
}

#include "details/MeshRenderer_AutoRefl.inl"
