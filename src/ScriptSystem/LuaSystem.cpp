#include "LuaSystem.h"

#include <DustEngine/ScriptSystem/LuaMngr.h>

using namespace Ubpa::DustEngine;

void LuaSystem::RegisterSystem(UECS::World* world, std::string name, sol::function onUpdate) {
	world->systemMngr.Register(std::unique_ptr<LuaSystem>(new LuaSystem(world, name, onUpdate)));
}

const Ubpa::UECS::SystemFunc* LuaSystem::RegisterEntityJob(
	UECS::Schedule* s,
	sol::function systemFunc,
	std::string name,
	UECS::ArchetypeFilter filter,
	UECS::CmptLocator cmptLocator,
	UECS::SingletonLocator singletonLocator
) {
	assert(!cmptLocator.CmptTypes().empty());
	auto bytes = systemFunc.dump();
	auto sysfunc = s->RegisterChunkJob(
	[bytes = std::move(bytes), cmptLocator = std::move(cmptLocator)]
	(UECS::World* w, UECS::SingletonsView singletonsView, UECS::ChunkView chunk) {
		if (chunk.EntityNum() == 0)
			return;

		auto L = LuaMngr::Instance().Request();
		{
			sol::state_view lua(L);
			sol::function f = lua.load(bytes.as_string_view());

			auto arrayEntity = chunk.GetCmptArray<UECS::Entity>();
			std::vector<void*> cmpts;
			std::vector<UECS::CmptType> types;
			std::vector<UECS::CmptPtr> cmptPtrs;
			std::vector<size_t> sizes;
			cmpts.reserve(cmptLocator.CmptTypes().size());
			types.reserve(cmptLocator.CmptTypes().size());
			cmptPtrs.reserve(cmptLocator.CmptTypes().size());
			sizes.reserve(cmptLocator.CmptTypes().size());
			for (const auto& t : cmptLocator.CmptTypes()) {
				cmpts.push_back(chunk.GetCmptArray(t));
				types.push_back(t);
				cmptPtrs.emplace_back(t, cmpts.back());
				sizes.push_back(UECS::RTDCmptTraits::Instance().Sizeof(t));
			}

			size_t i = 0;
			do {
				UECS::CmptsView view{ cmptPtrs.data(), cmptPtrs.size() };
				f(w, singletonsView, arrayEntity[i], i, view);
				for (size_t j = 0; j < cmpts.size(); j++) {
					cmpts[j] = (reinterpret_cast<uint8_t*>(cmpts[j]) + sizes[j]);
					cmptPtrs[j] = { types[j], cmpts[j] };
				}
			} while (++i < chunk.EntityNum());
		}
		LuaMngr::Instance().Recycle(L);
	}, std::move(name), std::move(filter), std::move(singletonLocator));
	return sysfunc;
}

const Ubpa::UECS::SystemFunc* LuaSystem::RegisterChunkJob(
	UECS::Schedule* s,
	sol::function systemFunc,
	std::string name,
	UECS::ArchetypeFilter filter,
	UECS::SingletonLocator singletonLocator
) {
	auto bytes = systemFunc.dump();
	auto sysfunc = s->RegisterChunkJob([bytes](UECS::World* w, UECS::SingletonsView singletonsView, UECS::ChunkView chunk) {
		auto L = LuaMngr::Instance().Request();
		{
			sol::state_view lua(L);
			sol::function f = lua.load(bytes.as_string_view());
			f.call(w, singletonsView, chunk);
		}
		LuaMngr::Instance().Recycle(L);
	}, std::move(name), std::move(filter), std::move(singletonLocator));
	return sysfunc;
}

const Ubpa::UECS::SystemFunc* LuaSystem::RegisterJob(
	UECS::Schedule* s,
	sol::function systemFunc,
	std::string name,
	UECS::SingletonLocator singletonLocator
) {
	auto bytes = systemFunc.dump();
	auto sysfunc = s->RegisterJob([bytes](UECS::World* w, UECS::SingletonsView singletonsView) {
		auto L = LuaMngr::Instance().Request();
		{
			sol::state_view lua(L);
			sol::function f = lua.load(bytes.as_string_view());
			f.call(w, singletonsView);
		}
		LuaMngr::Instance().Recycle(L);
	}, std::move(name), std::move(singletonLocator));
	return sysfunc;
}

LuaSystem::LuaSystem(UECS::World* world, std::string name, sol::function onUpdate)
	: UECS::System{ world,name }, onUpdate{ onUpdate.dump() } {}

void LuaSystem::OnUpdate(UECS::Schedule& schedule) {
	sol::state_view lua(LuaMngr::Instance().Main());
	sol::function f = lua.load(onUpdate.as_string_view());
	f.call(&schedule);
}
