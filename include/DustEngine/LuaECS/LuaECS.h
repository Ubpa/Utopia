#pragma once

#include <ULuaPP/ULuaPP.h>

namespace Ubpa::DustEngine {
	// World
	// - GetSystemMngr
	// - GetEntityMngr
	class LuaECS {
	public:
		static LuaECS& Instance() {
			static LuaECS instance;
			return instance;
		}

		void Init(lua_State* L);
	};
}
