#pragma once

#include <UECS/World.h>

namespace Ubpa::Utopia {
	struct TRSToLocalToParentSystem {
		static constexpr char SystemFuncName[] = "TRSToLocalToParentSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
