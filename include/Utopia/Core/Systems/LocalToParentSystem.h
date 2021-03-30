#pragma once

#include <UECS/UECS.hpp>

#include <UGM/transform.hpp>

namespace Ubpa::Utopia {
	struct LocalToParentSystem {
		static constexpr char SystemFuncName[] = "LocalToParentSystem";

		static void ChildLocalDeserializeToWorld(UECS::World* w, const transformf& parent_l2w, UECS::Entity e);

		static void OnUpdate(UECS::Schedule& schedule);
	};
}
