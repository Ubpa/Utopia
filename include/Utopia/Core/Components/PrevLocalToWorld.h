#pragma once

#include <UGM/transform.hpp>

namespace Ubpa::Utopia {
	struct PrevLocalToWorld {
		transformf value{ transformf::eye() };
	};
}
