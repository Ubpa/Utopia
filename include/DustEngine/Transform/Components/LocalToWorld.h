#pragma once

#include <UGM/transform.h>

namespace Ubpa::DustEngine {
	struct LocalToWorld {
		transformf value{ transformf::eye() };
	};
}

#include "details/LocalToWorld_AutoRefl.inl"
