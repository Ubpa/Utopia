#pragma once

#include <UECS/World.h>

namespace Ubpa::DustEngine {
	struct InputSystem {
		static constexpr char SystemFuncName[] = "InputSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
