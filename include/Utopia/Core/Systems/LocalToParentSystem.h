#pragma once

#include <UECS/UECS.hpp>

#include <UGM/transform.hpp>

namespace Ubpa::Utopia {
	struct LocalToParentSystem {
		static constexpr char SystemFuncName[] = "LocalToParentSystem";

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
