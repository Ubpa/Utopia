#pragma once

#include <UECS/World.h>

#include <UGM/transform.h>

namespace Ubpa::DustEngine {
	class LocalToParentSystem : public UECS::System {
	public:
		using System::System;

		static constexpr char SystemFuncName[] = "LocalToParentSystem";

		void ChildLocalToWorld(const transformf& parent_l2w, UECS::Entity e);

		virtual void OnUpdate(UECS::Schedule& schedule) override;
	};
}
