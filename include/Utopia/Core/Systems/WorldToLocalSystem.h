#pragma once

#include <UECS/UECS.hpp>

namespace Ubpa::Utopia {
	struct WorldToLocalSystem {
		static constexpr char SystemFuncName[] = "WorldToLocalSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
