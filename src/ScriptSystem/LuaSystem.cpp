#include "LuaSystem.h"

#include <DustEngine/ScriptSystem/LuaMngr.h>

using namespace Ubpa::DustEngine;

void LuaSystem::Register(UECS::World* world, std::string name, sol::function onUpdate) {
	world->systemMngr.Register(std::unique_ptr<LuaSystem>(new LuaSystem(world, name, onUpdate)));
}

// World, Entity, size_t index, CmptsView
const Ubpa::UECS::SystemFunc* LuaSystem::RegisterSystemFunc_Entity(
	UECS::Schedule* s,
	sol::function systemFunc,
	std::string name,
	UECS::EntityLocator locator,
	UECS::EntityFilter filter
) {
	assert(!locator.CmptTypes().empty());
	auto bytes = systemFunc.dump();
	auto sysfunc = s->Register([bytes, locator = std::move(locator)](UECS::World* w, UECS::ChunkView chunk) {
		if (chunk.EntityNum() == 0)
			return;

		auto L = LuaMngr::Instance().Request();
		{
			sol::state_view lua(L);
			sol::function f = lua.load(bytes.as_string_view());
			auto arrayEntity = chunk.GetCmptArray<UECS::Entity>();
			std::vector<void*> cmpts;
			std::vector<size_t> sizes;
			cmpts.reserve(locator.CmptTypes().size());
			sizes.reserve(locator.CmptTypes().size());
			for (const auto& t : locator.CmptTypes()) {
				cmpts.push_back(chunk.GetCmptArray(t));
				sizes.push_back(UECS::RTDCmptTraits::Instance().Sizeof(t));
			}

			size_t i = 0;
			do {
				UECS::CmptsView view{ &locator, cmpts.data() };
				f(w, arrayEntity[i], i, view);
				for (size_t j = 0; j < cmpts.size(); j++)
					cmpts[j] = (reinterpret_cast<uint8_t*>(cmpts[j]) + sizes[j]);
			} while (++i < chunk.EntityNum());
		}
		LuaMngr::Instance().Recycle(L);
	}, std::move(name), std::move(filter));
	return sysfunc;
}

// World, ChunkView
const Ubpa::UECS::SystemFunc* LuaSystem::RegisterSystemFunc_Chunk(
	UECS::Schedule* s,
	sol::function systemFunc,
	std::string name,
	UECS::EntityFilter filter
) {
	assert(!filter.AllCmptTypes().empty() || !filter.AnyCmptTypes().empty());
	auto bytes = systemFunc.dump();
	auto sysfunc = s->Register([bytes](UECS::World* w, UECS::ChunkView chunk) {
		if (chunk.EntityNum() == 0)
			return;

		auto L = LuaMngr::Instance().Request();
		{
			sol::state_view lua(L);
			sol::function f = lua.load(bytes.as_string_view());
			f.call(w, chunk);
		}
		LuaMngr::Instance().Recycle(L);
	}, std::move(name), std::move(filter));
	return sysfunc;
}

// World
const Ubpa::UECS::SystemFunc* LuaSystem::RegisterSystemFunc_Job(
	UECS::Schedule* s,
	sol::function systemFunc,
	std::string name
) {
	auto bytes = systemFunc.dump();
	auto sysfunc = s->Register([bytes](UECS::World* w) {
		auto L = LuaMngr::Instance().Request();
		{
			sol::state_view lua(L);
			sol::function f = lua.load(bytes.as_string_view());
			f.call(w);
		}
		LuaMngr::Instance().Recycle(L);
	}, std::move(name));
	return sysfunc;
}

LuaSystem::LuaSystem(UECS::World* world, std::string name, sol::function onUpdate)
	: UECS::System{ world,name }, onUpdate{ onUpdate.dump() } {}

void LuaSystem::OnUpdate(UECS::Schedule& schedule) {
	sol::state_view lua(LuaMngr::Instance().Main());
	sol::function f = lua.load(onUpdate.as_string_view());
	f.call(&schedule);
}
