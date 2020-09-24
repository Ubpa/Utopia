#pragma once

#include "../Material.h"

namespace Ubpa::DustEngine {
	// singleton
	struct Skybox {
		const Material* material;
	};
}

#include "details/Skybox_AutoRefl.inl"
