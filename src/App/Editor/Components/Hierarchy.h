#pragma once

#include <UECS/World.h>

namespace Ubpa::DustEngine {
	struct Hierarchy {
		UECS::World* world{ nullptr };
		UECS::Entity select;
		UECS::Entity hover;
	};
}
