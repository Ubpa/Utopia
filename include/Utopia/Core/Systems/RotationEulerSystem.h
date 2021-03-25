#pragma once

#include <UECS/UECS.hpp>

namespace Ubpa::Utopia {
	struct RotationEulerSystem {
		static constexpr char SystemFuncName[] = "RotationEulerSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}