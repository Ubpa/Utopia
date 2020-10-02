#pragma once

#include <UECS/World.h>

#include <UGM/transform.h>

namespace Ubpa::Utopia {
	struct LocalToParentSystem {
		static constexpr char SystemFuncName[] = "LocalToParentSystem";

		static void ChildLocalToWorld(UECS::World* w, const transformf& parent_l2w, UECS::Entity e);

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
