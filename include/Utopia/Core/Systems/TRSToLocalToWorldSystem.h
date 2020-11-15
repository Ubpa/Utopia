#pragma once

#include <UECS/World.h>

namespace Ubpa::Utopia {
	struct TRSToLocalToWorldSystem {
		static constexpr const char SystemFuncName[] = "TRSToLocalToWorldSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
