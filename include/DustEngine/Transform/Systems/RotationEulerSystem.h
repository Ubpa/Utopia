#pragma once

#include <UECS/World.h>

namespace Ubpa::DustEngine {
	class RotationEulerSystem : public UECS::System {
	public:
		using System::System;

		static constexpr char SystemFuncName[] = "RotationEulerSystem";

		virtual void OnUpdate(UECS::Schedule& schedule) override;
	};
}