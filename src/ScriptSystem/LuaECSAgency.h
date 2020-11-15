#pragma once

#include <UECS/World.h>
#include <ULuaPP/ULuaPP.h>

namespace Ubpa::Utopia {
	struct LuaECSAgency {
		// World, SingletonsView
		static std::function<void(UECS::Schedule&)> SafeOnUpdate(sol::function onUpdate);

		// World, SingletonsView, Entity, size_t index, CmptsView
		static const UECS::SystemFunc* RegisterEntityJob(
			UECS::Schedule*,
			sol::function systemFunc,
			std::string name,
			bool isParallel = true,
			UECS::ArchetypeFilter = {},
			UECS::CmptLocator = {},
			UECS::SingletonLocator = {},
			UECS::RandomAccessor = {}
		);

		// World, SingletonsView, entityBeginIndexInQuery, ChunkView
		static const UECS::SystemFunc* RegisterChunkJob(
			UECS::Schedule*,
			sol::function systemFunc,
			std::string name,
			UECS::ArchetypeFilter = {},
			bool isParallel = true,
			UECS::SingletonLocator = {},
			UECS::RandomAccessor = {}
		);

		// World, SingletonsView
		static const UECS::SystemFunc* RegisterJob(
			UECS::Schedule*,
			sol::function systemFunc,
			std::string name,
			UECS::SingletonLocator = {},
			UECS::RandomAccessor = {}
		);
	};
}

#include "detail/LuaECSAgency_AutoRefl.inl"
