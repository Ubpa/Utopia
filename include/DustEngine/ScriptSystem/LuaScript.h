#pragma once

#include <string>

namespace Ubpa::DustEngine {
	class LuaScript {
	public:
		LuaScript(std::string str) : str(std::move(str)) {}
		const std::string& GetString() const noexcept { return str; }
	private:
		std::string str;
	};
}
