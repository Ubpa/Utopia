#include "LuaSystem.h"

#include <DustEngine/ScriptSystem/LuaCtxMngr.h>
#include <DustEngine/ScriptSystem/LuaContext.h>

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
	assert(!cmptLocator.CmptAccessTypes().empty());
	auto bytes = systemFunc.dump();
	auto sysfunc = s->RegisterChunkJob(
	[bytes = std::move(bytes), cmptLocator = std::move(cmptLocator)]
	(UECS::World* w, UECS::SingletonsView singletonsView, UECS::ChunkView chunk) {
		if (chunk.EntityNum() == 0)
			return;

		auto luaCtx = LuaCtxMngr::Instance().GetContext(w);
		auto L = luaCtx->Request();
		{
			sol::state_view lua(L);
			sol::function f = lua.load(bytes.as_string_view());

			auto arrayEntity = chunk.GetCmptArray<UECS::Entity>();
			std::vector<void*> cmpts;
			std::vector<UECS::CmptAccessType> types;
			std::vector<UECS::CmptAccessPtr> cmptPtrs;
			std::vector<size_t> sizes;
			cmpts.reserve(cmptLocator.CmptAccessTypes().size());
			types.reserve(cmptLocator.CmptAccessTypes().size());
			cmptPtrs.reserve(cmptLocator.CmptAccessTypes().size());
			sizes.reserve(cmptLocator.CmptAccessTypes().size());
			for (const auto& t : cmptLocator.CmptAccessTypes()) {
				cmpts.push_back(chunk.GetCmptArray(t));
				types.push_back(t);
				cmptPtrs.emplace_back(t, cmpts.back());
				sizes.push_back(w->entityMngr.cmptTraits.Sizeof(t));
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
		luaCtx->Recycle(L);
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
	auto sysfunc = s->RegisterChunkJob(
		[bytes](UECS::World* w, UECS::SingletonsView singletonsView, UECS::ChunkView chunk) {
			auto luaCtx = LuaCtxMngr::Instance().GetContext(w);
			auto L = luaCtx->Request();
			{
				sol::state_view lua(L);
				sol::function f = lua.load(bytes.as_string_view());
				f.call(w, singletonsView, chunk);
			}
			luaCtx->Recycle(L);
		},
		std::move(name), std::move(filter), std::move(singletonLocator)
	);
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
		auto luaCtx = LuaCtxMngr::Instance().GetContext(w);
		auto L = luaCtx->Request();
		{
			sol::state_view lua(L);
			sol::function f = lua.load(bytes.as_string_view());
			f.call(w, singletonsView);
		}
		luaCtx->Recycle(L);
	}, std::move(name), std::move(singletonLocator));
	return sysfunc;
}

LuaSystem::LuaSystem(UECS::World* world, std::string name, sol::function onUpdate)
	: UECS::System{ world,name }, luaCtx{ LuaCtxMngr::Instance().GetContext(world) }
{
	sol::state_view lua{ luaCtx->Main() };
	auto bytecode = onUpdate.dump();
	mainOnUpdate = lua.load(bytecode.as_string_view());
}

void LuaSystem::OnUpdate(UECS::Schedule& schedule) {
	mainOnUpdate.call(&schedule);
}
