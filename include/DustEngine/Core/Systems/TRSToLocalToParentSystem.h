#pragma once

#include <UECS/World.h>

namespace Ubpa::DustEngine {
	struct TRSToLocalToParentSystem {
		static constexpr char SystemFuncName[] = "TRSToLocalToParentSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
