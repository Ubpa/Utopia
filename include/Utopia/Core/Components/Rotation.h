#pragma once

#include <UGM/quat.h>

namespace Ubpa::Utopia {
	struct Rotation {
		quatf value{ quatf::identity() };
	};
}

#include "details/Rotation_AutoRefl.inl"
