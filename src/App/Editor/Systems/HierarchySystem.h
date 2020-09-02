#pragma once

#include <UECS/World.h>

namespace Ubpa::DustEngine {
	class HierarchySystem : public UECS::System {
	public:
		using UECS::System::System;
	private:
		virtual void OnUpdate(UECS::Schedule&) override;
	};
}
