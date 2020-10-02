#include <Utopia/ScriptSystem/LuaScriptQueueSystem.h>

#include <Utopia/ScriptSystem/LuaScriptQueue.h>
#include <Utopia/ScriptSystem/LuaContext.h>
#include <Utopia/ScriptSystem/LuaCtxMngr.h>
#include <Utopia/ScriptSystem/LuaScript.h>

#include <_deps/sol/sol.hpp>

using namespace Ubpa::Utopia;
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
