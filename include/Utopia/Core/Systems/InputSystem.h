#pragma once

#include <UECS/World.h>

namespace Ubpa::Utopia {
	struct InputSystem {
		static constexpr char SystemFuncName[] = "InputSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
