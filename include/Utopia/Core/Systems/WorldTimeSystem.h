#pragma once

#include <UECS/UECS.hpp>

namespace Ubpa::Utopia {
	struct WorldTimeSystem {
		static constexpr char SystemFuncName[] = "WorldTimeSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
