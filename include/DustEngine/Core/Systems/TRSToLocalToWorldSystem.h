#pragma once

#include <UECS/World.h>

namespace Ubpa::DustEngine {
	struct TRSToLocalToWorldSystem {
		static constexpr const char SystemFuncName[] = "TRSToWorldToLocalSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
