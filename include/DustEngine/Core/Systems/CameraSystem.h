#pragma once

#include <UECS/World.h>

namespace Ubpa::DustEngine {
	struct CameraSystem {
		static constexpr char SystemFuncName[] = "CameraSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
