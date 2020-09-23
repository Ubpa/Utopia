#pragma once

#include <UECS/World.h>

namespace Ubpa::DustEngine {
	struct WorldTimeSystem {
		static constexpr char SystemFuncName[] = "WorldTimeSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
