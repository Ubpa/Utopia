#pragma once

namespace Ubpa::UECS {
	class World;
}

namespace Ubpa::Utopia {
	class LuaContext;

	class LuaCtxMngr {
	public:
		static LuaCtxMngr& Instance() {
			static LuaCtxMngr instance;
			return instance;
		}

		LuaContext* Register(const UECS::World* world);
		void Unregister(const UECS::World* world);

		// if not registered, return nullptr
		LuaContext* GetContext(const UECS::World* world);

		void Clear();

	private:
		LuaCtxMngr();
		~LuaCtxMngr();
		struct Impl;
		Impl* pImpl{ nullptr };
	};
}
