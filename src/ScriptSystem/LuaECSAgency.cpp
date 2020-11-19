#include "LuaECSAgency.h"

#include <Utopia/ScriptSystem/LuaCtxMngr.h>
#include <Utopia/ScriptSystem/LuaContext.h>

#include <spdlog/spdlog.h>

using namespace Ubpa::Utopia;

std::function<void(Ubpa::UECS::Schedule&)> LuaECSAgency::SafeOnUpdate(sol::function onUpdate) {
	return [onUpdate = std::move(onUpdate)](Ubpa::UECS::Schedule& schedule) {
		auto rst = onUpdate.call(schedule);
		if (!rst.valid()) {
			sol::error err = rst;
			spdlog::error(err.what());
		}
	};
}

const Ubpa::UECS::SystemFunc* LuaECSAgency::RegisterEntityJob(
	UECS::Schedule* s,
	sol::function systemFunc,
	std::string name,
	bool isParallel,
	UECS::ArchetypeFilter archetypeFilter,
	UECS::CmptLocator cmptLocator,
	UECS::SingletonLocator singletonLocator,
	UECS::RandomAccessor randomAccessor
) {
	assert(!cmptLocator.CmptAccessTypes().empty());
	auto bytes = systemFunc.dump();
	auto sysfunc = s->RegisterChunkJob(
	[bytes = std::move(bytes), cmptLocator = std::move(cmptLocator)]
	(UECS::World* w, UECS::SingletonsView singletonsView, UECS::ChunkView chunk, size_t idx) {
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
				UECS::CmptsView view{ Span{cmptPtrs.data(), cmptPtrs.size()} };
				auto rst = f.call(w, singletonsView, arrayEntity[i], idx + i, view);
				if (!rst.valid()) {
					sol::error err = rst;
					spdlog::error(err.what());
				}
				for (size_t j = 0; j < cmpts.size(); j++) {
					cmpts[j] = (reinterpret_cast<uint8_t*>(cmpts[j]) + sizes[j]);
					cmptPtrs[j] = { types[j], cmpts[j] };
				}
			} while (++i < chunk.EntityNum());
		}
		luaCtx->Recycle(L);
	}, std::move(name), std::move(archetypeFilter), isParallel, std::move(singletonLocator), std::move(randomAccessor));
	return sysfunc;
}

const Ubpa::UECS::SystemFunc* LuaECSAgency::RegisterChunkJob(
	UECS::Schedule* s,
	sol::function systemFunc,
	std::string name,
	UECS::ArchetypeFilter filter,
	bool isParallel,
	UECS::SingletonLocator singletonLocator,
	UECS::RandomAccessor randomAccessor
) {
	auto bytes = systemFunc.dump();
	auto sysfunc = s->RegisterChunkJob(
		[bytes](UECS::World* w, UECS::SingletonsView singletonsView, size_t entityBeginIndexInQuery, UECS::ChunkView chunk) {
			auto luaCtx = LuaCtxMngr::Instance().GetContext(w);
			auto L = luaCtx->Request();
			{
				sol::state_view lua(L);
				sol::function f = lua.load(bytes.as_string_view());
				auto rst = f.call(w, singletonsView, entityBeginIndexInQuery, chunk);
				if (!rst.valid()) {
					sol::error err = rst;
					spdlog::error(err.what());
				}
			}
			luaCtx->Recycle(L);
		},
		std::move(name), std::move(filter), isParallel, std::move(singletonLocator), std::move(randomAccessor)
	);
	return sysfunc;
}

const Ubpa::UECS::SystemFunc* LuaECSAgency::RegisterJob(
	UECS::Schedule* s,
	sol::function systemFunc,
	std::string name,
	UECS::SingletonLocator singletonLocator,
	UECS::RandomAccessor randomAccessor
) {
	auto bytes = systemFunc.dump();
	auto sysfunc = s->RegisterJob([bytes](UECS::World* w, UECS::SingletonsView singletonsView) {
		auto luaCtx = LuaCtxMngr::Instance().GetContext(w);
		auto L = luaCtx->Request();
		{
			sol::state_view lua(L);
			sol::function f = lua.load(bytes.as_string_view());
			auto rst = f.call(w, singletonsView);
			if (!rst.valid()) {
				sol::error err = rst;
				spdlog::error(err.what());
			}
		}
		luaCtx->Recycle(L);
	}, std::move(name), std::move(singletonLocator), std::move(randomAccessor));
	return sysfunc;
}
