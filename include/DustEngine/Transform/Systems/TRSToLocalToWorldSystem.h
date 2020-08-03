#pragma once

#include <UECS/World.h>

namespace Ubpa::DustEngine {
	class TRSToLocalToWorldSystem : public UECS::System {
	public:
		using System::System;

		static constexpr const char SystemFuncName[] = "TRSToWorldToLocalSystem";

		virtual void OnUpdate(UECS::Schedule& schedule) override;
	};
}
