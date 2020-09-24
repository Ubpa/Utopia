#pragma once

#include <UGM/vec.h>

namespace Ubpa::DustEngine {
	struct Translation {
		vecf3 value{ 0.f };
	};
}

#include "details/Translation_AutoRefl.inl"
