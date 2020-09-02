#pragma once

#include <UECS/World.h>

namespace Ubpa::DustEngine {
	struct Hierarchy {
		const UECS::World* world{ nullptr };
		UECS::Entity select;
	};
}
