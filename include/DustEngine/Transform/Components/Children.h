#pragma once

#include <UECS/Entity.h>
#include <vector>

namespace Ubpa::DustEngine {
	struct Children {
		std::vector<UECS::Entity> value;
	};
}

#include "details/Children_AutoRefl.inl"
