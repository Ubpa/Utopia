#pragma once

#include "LuaScript.h"

#include <vector>

namespace Ubpa::Utopia {
	struct LuaScriptQueue {
		std::vector<const LuaScript*> value;
	};
}

#include "details/LuaScriptQueue_AutoRefl.h"
