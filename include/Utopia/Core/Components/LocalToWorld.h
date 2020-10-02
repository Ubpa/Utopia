#pragma once

#include <UGM/transform.h>

namespace Ubpa::Utopia {
	struct LocalToWorld {
		transformf value{ transformf::eye() };
	};
}

#include "details/LocalToWorld_AutoRefl.inl"
