#pragma once

#include <UGM/transform.hpp>

namespace Ubpa::Utopia {
	struct LocalToWorld {
		transformf value{ transformf::eye() };
	};
}
