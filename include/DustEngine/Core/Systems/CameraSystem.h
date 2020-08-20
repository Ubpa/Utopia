#pragma once

#include <UECS/World.h>

#include <UGM/transform.h>

namespace Ubpa::DustEngine {
	class CameraSystem : public UECS::System {
	public:
		using System::System;

		static constexpr char SystemFuncName[] = "CameraSystem";

		virtual void OnUpdate(UECS::Schedule& schedule) override;
	};
}
