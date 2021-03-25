#pragma once

#include <UGM/transform.hpp>

namespace Ubpa::Utopia {
	struct LocalToParent {
		transformf value{ transformf::eye() };
	};
}
