#pragma once

#include <UGM/transform.h>

namespace Ubpa::DustEngine {
	struct LocalToParent {
		transformf value{ transformf::eye() };
	};
}

#include "details/LocalToParent_AutoRefl.inl"

