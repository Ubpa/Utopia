#pragma once

#include <UECS/World.h>

namespace Ubpa::Utopia {
	struct CameraSystem {
		static constexpr char SystemFuncName[] = "CameraSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
