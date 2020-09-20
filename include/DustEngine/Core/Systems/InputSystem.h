#pragma once

#include <UECS/World.h>

#include <UGM/transform.h>

namespace Ubpa::DustEngine {
	class InputSystem : public UECS::System {
	public:
		using System::System;

		static constexpr char SystemFuncName[] = "InputSystem";

		virtual void OnUpdate(UECS::Schedule& schedule) override;
	};
}
