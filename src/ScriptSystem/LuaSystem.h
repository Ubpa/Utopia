#pragma once

#include <UECS/World.h>
#include <ULuaPP/ULuaPP.h>

namespace Ubpa::DustEngine {
	class LuaSystem : public UECS::System {
	public:
		using System::System;

		static void Register(UECS::World* world, std::string name, sol::function onUpdate);

		static void RegisterChunkFunc(UECS::Schedule*, sol::function onUpdate, std::string name, UECS::EntityFilter);

		static std::function<void(UECS::ChunkView)> WrapChunkFunc(sol::function);

	private:
		LuaSystem(UECS::World* world, std::string name, sol::function onUpdate);

		virtual void OnUpdate(UECS::Schedule& schedule) override;

		sol::bytecode onUpdate;
	};
}

#include "detail/UECS_Refl/System_AutoRefl.inl"
#include "detail/LuaSystem_AutoRefl.inl"
