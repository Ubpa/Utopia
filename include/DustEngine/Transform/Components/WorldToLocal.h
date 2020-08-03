#pragma once

#include <UGM/transform.h>

namespace Ubpa::DustEngine {
	struct WorldToLocal {
		transformf value{ transformf::eye() };
	};
}

#include "details/WorldToLocal_AutoRefl.inl"
