#pragma once

#include "../Material.h"
#include <USTL/memory.h>

namespace Ubpa::Utopia {
	// singleton
	struct Skybox {
		USTL::shared_object<Material> material;
	};
}

#include "details/Skybox_AutoRefl.inl"
