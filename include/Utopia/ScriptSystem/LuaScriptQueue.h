#pragma once

#include "LuaScript.h"

#include <vector>
#include <memory>

namespace Ubpa::Utopia {
	struct LuaScriptQueue {
		std::vector<std::shared_ptr<LuaScript>> value;
	};
}

#include "details/LuaScriptQueue_AutoRefl.h"
