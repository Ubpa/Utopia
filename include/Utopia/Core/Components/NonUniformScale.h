#pragma once

#include <UGM/scale.h>

namespace Ubpa::Utopia {
	struct NonUniformScale {
		scalef3 value{ 1.f };
	};
}

#include "details/NonUniformScale_AutoRefl.inl"
