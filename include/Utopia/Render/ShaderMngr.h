#pragma once

#include <map>
#include <string>

namespace Ubpa::Utopia {
	struct Shader;

	class ShaderMngr {
	public:
		static ShaderMngr& Instance() noexcept {
			static ShaderMngr instance;
			return instance;
		}

		void Register(Shader*);
		Shader* Get(std::string_view name) const;

	private:
		ShaderMngr() = default;
		std::map<std::string, Shader*, std::less<>> shaderMap;
	};
}
