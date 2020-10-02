#pragma once

#include <UECS/World.h>

namespace Ubpa::Utopia {
	struct WorldToLocalSystem {
		static constexpr char SystemFuncName[] = "WorldToLocalSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
