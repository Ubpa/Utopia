#pragma once

#include <UGM/transform.h>

namespace Ubpa::Utopia {
	struct LocalToParent {
		transformf value{ transformf::eye() };
	};
}

#include "details/LocalToParent_AutoRefl.inl"

