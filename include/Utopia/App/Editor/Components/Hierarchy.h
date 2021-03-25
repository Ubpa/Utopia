#pragma once

#include <UECS/UECS.hpp>

namespace Ubpa::Utopia {
	struct Hierarchy {
		UECS::World* world{ nullptr };
		UECS::Entity select;
		UECS::Entity hover;
	};
}
