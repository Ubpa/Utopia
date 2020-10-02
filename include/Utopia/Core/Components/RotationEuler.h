#pragma once

#include <UGM/euler.h>

namespace Ubpa::Utopia {
	struct RotationEuler {
		eulerf value{ 0.f };
	};
}

#include "details/RotationEuler_AutoRefl.inl"
