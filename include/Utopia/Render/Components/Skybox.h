#pragma once

#include "../Material.h"

namespace Ubpa::Utopia {
	// singleton
	struct Skybox {
		std::shared_ptr<Material> material;
	};
}

#include "details/Skybox_AutoRefl.inl"
