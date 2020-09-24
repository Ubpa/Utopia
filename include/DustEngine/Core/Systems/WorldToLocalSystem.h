#pragma once

#include <UECS/World.h>

namespace Ubpa::DustEngine {
	struct WorldToLocalSystem {
		static constexpr char SystemFuncName[] = "WorldToLocalSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
