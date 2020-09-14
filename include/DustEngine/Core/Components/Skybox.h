#pragma once

#include "../TextureCube.h"

namespace Ubpa::DustEngine {
	// singleton
	struct Skybox {
		const TextureCube* texcube;
	};
}

#include "details/Skybox_AutoRefl.inl"
