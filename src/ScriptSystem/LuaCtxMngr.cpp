#include <DustEngine/ScriptSystem/LuaCtxMngr.h>

#include <DustEngine/ScriptSystem/LuaContext.h>

#include <map>
#include <memory>
#include <cassert>

using namespace Ubpa::DustEngine;

struct LuaCtxMngr::Impl {
	std::map<const UECS::World*, std::unique_ptr<LuaContext>> world2ctx;
};

LuaCtxMngr::LuaCtxMngr()
	:pImpl{ new Impl }
{
}

LuaCtxMngr::~LuaCtxMngr() {
	delete pImpl;
}

LuaContext* LuaCtxMngr::Register(const UECS::World* world) {
	auto target = pImpl->world2ctx.find(world);
	if (target != pImpl->world2ctx.end())
		return target->second.get();

	auto ctx = new LuaContext;
	pImpl->world2ctx.emplace_hint(target, world, std::unique_ptr<LuaContext>{ctx});
	return ctx;
}

void LuaCtxMngr::Unregister(const UECS::World* world) {
	pImpl->world2ctx.erase(world);
}

// if not registered, return nullptr
LuaContext* LuaCtxMngr::GetContext(const UECS::World* world) {
	auto target = pImpl->world2ctx.find(world);
	if (target == pImpl->world2ctx.end())
		return nullptr;

	return target->second.get();
}

void LuaCtxMngr::Clear() {
	pImpl->world2ctx.clear();
}
