#pragma once

#include <UECS/World.h>

namespace Ubpa::DustEngine {
	struct RotationEulerSystem {
		static constexpr char SystemFuncName[] = "RotationEulerSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}