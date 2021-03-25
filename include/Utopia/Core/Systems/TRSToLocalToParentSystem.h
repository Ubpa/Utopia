#pragma once

#include <UECS/UECS.hpp>

namespace Ubpa::Utopia {
	struct TRSToLocalToParentSystem {
		static constexpr char SystemFuncName[] = "TRSToLocalToParentSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
