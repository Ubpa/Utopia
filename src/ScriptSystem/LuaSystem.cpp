#include "LuaSystem.h"

#include <DustEngine/ScriptSystem/LuaMngr.h>

using namespace Ubpa::DustEngine;

void LuaSystem::Register(UECS::World* world, std::string name, sol::function onUpdate) {
	world->systemMngr.Register(std::unique_ptr<LuaSystem>(new LuaSystem(world, name, onUpdate)));
}

void LuaSystem::RegisterChunkFunc(UECS::Schedule* s, sol::function onUpdate, std::string name, UECS::EntityFilter filter) {
	s->Register(WrapChunkFunc(std::move(onUpdate)), std::move(name), std::move(filter));
}

std::function<void(Ubpa::UECS::ChunkView)> LuaSystem::WrapChunkFunc(sol::function func) {
	auto bytes = func.dump();
	return [=](Ubpa::UECS::ChunkView chunk) {
		auto L = LuaMngr::Instance().Request();
		{
			sol::state_view lua(L);
			sol::function f = lua.load(bytes.as_string_view());
			f.call(chunk);
		}
		LuaMngr::Instance().Recycle(L);
	};
}

LuaSystem::LuaSystem(UECS::World* world, std::string name, sol::function onUpdate)
	: UECS::System{ world,name }, onUpdate{ onUpdate.dump() } {}

void LuaSystem::OnUpdate(UECS::Schedule& schedule) {
	auto L = LuaMngr::Instance().Request();
	{
		sol::state_view lua(L);
		sol::function f = lua.load(onUpdate.as_string_view());
		f.call(&schedule);
	}
	LuaMngr::Instance().Recycle(L);
}
