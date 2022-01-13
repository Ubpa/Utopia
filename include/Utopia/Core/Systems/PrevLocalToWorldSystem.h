#pragma once

#include <UECS/UECS.hpp>

namespace Ubpa::Utopia {
	struct PrevLocalToWorldSystem {
		static constexpr const char SystemFuncName[] = "PrevLocalToWorldSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
