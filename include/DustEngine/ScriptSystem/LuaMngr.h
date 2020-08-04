#pragma once

#include <ULuaPP/ULuaPP.h>

#include <set>

namespace Ubpa::DustEngine {
	// World
	// - GetSystemMngr
	// - GetEntityMngr
	class LuaMngr {
	public:
		static LuaMngr& Instance() {
			static LuaMngr instance;
			return instance;
		}

		void Init();

		void Reserve(size_t n);

		// lock
		lua_State* Request();
		// lock
		void Recycle(lua_State*);

		void Clear();

	private:
		struct Impl;
		Impl* pImpl{ nullptr };
	};
}
