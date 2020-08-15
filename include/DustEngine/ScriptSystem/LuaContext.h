#pragma once

struct lua_State;

namespace Ubpa::DustEngine {
	class LuaContext {
	public:
		LuaContext();
		~LuaContext();

		void Reserve(size_t n);

		lua_State* Main() const;

		// lock
		lua_State* Request();

		// lock
		void Recycle(lua_State*);

		// this won't clear main
		void Clear();

	private:
		struct Impl;
		Impl* pImpl;
	};
}