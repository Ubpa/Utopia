#pragma once

#include <UECS/World.h>

namespace Ubpa::DustEngine {
	class InspectorSystem : public UECS::System {
	public:
		using UECS::System::System;
	private:
		virtual void OnUpdate(UECS::Schedule&) override;
	};
}
