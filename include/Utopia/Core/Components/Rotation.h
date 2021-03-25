#pragma once

#include <UGM/quat.hpp>

namespace Ubpa::Utopia {
	struct Rotation {
		quatf value{ quatf::identity() };
	};
}
