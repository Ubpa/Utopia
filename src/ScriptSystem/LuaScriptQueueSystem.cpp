#include <DustEngine/ScriptSystem/LuaScriptQueueSystem.h>

#include <DustEngine/ScriptSystem/LuaScriptQueue.h>
#include <DustEngine/ScriptSystem/LuaContext.h>
#include <DustEngine/ScriptSystem/LuaCtxMngr.h>
#include <DustEngine/ScriptSystem/LuaScript.h>

#include <_deps/sol/sol.hpp>

using namespace Ubpa::DustEngine;
using namespace Ubpa::UECS;

void LuaScriptQueueSystem::OnUpdate(Schedule& schedule) {
	schedule.RegisterCommand([](World* w) {
		auto scripts = w->entityMngr.GetSingleton<LuaScriptQueue>();
		if (!scripts)
			return;

		sol::state_view lua{ LuaCtxMngr::Instance().GetContext(w)->Main() };
		for (auto script : scripts->value) {
			if (!script)
				continue;
			lua.safe_script(script->GetText());
		}
		scripts->value.clear();
	});
}
