#pragma once

#include "../Material.h"

namespace Ubpa::Utopia {
	// singleton
	struct Skybox {
		const Material* material;
	};
}

#include "details/Skybox_AutoRefl.inl"
