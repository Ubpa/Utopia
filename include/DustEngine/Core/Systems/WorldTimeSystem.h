#pragma once

#include <UECS/World.h>

namespace Ubpa::DustEngine {
	class WorldTimeSystem : public UECS::System {
	public:
		using UECS::System::System;

		static constexpr char SystemFuncName[] = "WorldTimeSystem";

		virtual void OnUpdate(UECS::Schedule& schedule) override;
	};
}
