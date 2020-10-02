#pragma once

#include <UECS/World.h>

namespace Ubpa::Utopia {
	struct LuaScriptQueueSystem {
		static void OnUpdate(UECS::Schedule& schedule);
	};
}
