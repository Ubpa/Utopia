#pragma once

#include <UECS/World.h>
#include <ULuaPP/ULuaPP.h>

namespace Ubpa::Utopia {
	struct LuaECSAgency {
		// World, SingletonsView, Entity, size_t index, CmptsView
		static const UECS::SystemFunc* RegisterEntityJob(
			UECS::Schedule*,
			sol::function systemFunc,
			std::string name,
			UECS::ArchetypeFilter,
			UECS::CmptLocator,
			UECS::SingletonLocator,
			bool isParallel
		);

		// World, SingletonsView, ChunkView
		static const UECS::SystemFunc* RegisterChunkJob(
			UECS::Schedule*,
			sol::function systemFunc,
			std::string name,
			UECS::ArchetypeFilter,
			UECS::SingletonLocator,
			bool isParallel
		);

		// World, SingletonsView
		static const UECS::SystemFunc* RegisterJob(
			UECS::Schedule*,
			sol::function systemFunc,
			std::string name,
			UECS::SingletonLocator
		);
	};
}

#include "detail/LuaECSAgency_AutoRefl.inl"
