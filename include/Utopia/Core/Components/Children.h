#pragma once

#include <UECS/Entity.h>
#include <set>

namespace Ubpa::Utopia {
	struct Children {
		std::set<UECS::Entity> value;
	};
}

#include "details/Children_AutoRefl.inl"
