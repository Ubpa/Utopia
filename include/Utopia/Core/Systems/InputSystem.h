#pragma once

#include <UECS/UECS.hpp>

namespace Ubpa::Utopia {
	struct InputSystem {
		static constexpr char SystemFuncName[] = "InputSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
