#pragma once

#include <UECS/World.h>
#include <ULuaPP/ULuaPP.h>

namespace Ubpa::DustEngine {
	class LuaSystem : public UECS::System {
	public:
		using System::System;

		static void Register(UECS::World* world, std::string name, sol::function onUpdate);

		// World, Entity, size_t index, CmptsView
		static const UECS::SystemFunc* RegisterSystemFunc_Entity(
			UECS::Schedule*,
			sol::function systemFunc,
			std::string name,
			UECS::EntityLocator,
			UECS::EntityFilter
		);

		// World, ChunkView
		static const UECS::SystemFunc* RegisterSystemFunc_Chunk(
			UECS::Schedule*,
			sol::function systemFunc,
			std::string name,
			UECS::EntityFilter
		);

		// World
		static const UECS::SystemFunc* RegisterSystemFunc_Job(
			UECS::Schedule*,
			sol::function systemFunc,
			std::string name
		);

	private:
		LuaSystem(UECS::World* world, std::string name, sol::function onUpdate);

		virtual void OnUpdate(UECS::Schedule& schedule) override;

		sol::bytecode onUpdate;
	};
}

#include "detail/UECS_AutoRefl/System_AutoRefl.inl"
#include "detail/LuaSystem_AutoRefl.inl"
