#pragma once

#include "LuaScript.h"

#include <vector>

namespace Ubpa::DustEngine {
	struct LuaScriptQueue {
		std::vector<const LuaScript*> value;
	};
}

#include "details/LuaScriptQueue_AutoRefl.h"
