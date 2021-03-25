#pragma once

#include <UECS/UECS.hpp>

namespace Ubpa::Utopia {
	struct TRSToLocalToWorldSystem {
		static constexpr const char SystemFuncName[] = "TRSToLocalToWorldSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
