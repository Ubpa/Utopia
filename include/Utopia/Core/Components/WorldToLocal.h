#pragma once

#include <UGM/transform.hpp>

namespace Ubpa::Utopia {
	struct WorldToLocal {
		transformf value{ transformf::eye() };
	};
}
