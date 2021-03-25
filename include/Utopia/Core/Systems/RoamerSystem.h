#pragma once

#include <UECS/UECS.hpp>

namespace Ubpa::Utopia {
	struct RoamerSystem {
		static constexpr char SystemFuncName[] = "RoamerSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
