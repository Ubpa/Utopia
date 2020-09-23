#pragma once

#include <UECS/World.h>

namespace Ubpa::DustEngine {
	struct LuaScriptQueueSystem {
		static void OnUpdate(UECS::Schedule& schedule);
	};
}
