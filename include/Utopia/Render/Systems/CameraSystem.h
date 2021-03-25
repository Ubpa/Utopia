#pragma once

#include <UECS/UECS.hpp>

namespace Ubpa::Utopia {
	struct CameraSystem {
		static constexpr char SystemFuncName[] = "CameraSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
