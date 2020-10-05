#pragma once

#include <UECS/World.h>

namespace Ubpa::Utopia {
	struct Hierarchy {
		UECS::World* world{ nullptr };
		UECS::Entity select;
		UECS::Entity hover;
	};
}
