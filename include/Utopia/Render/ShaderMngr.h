#pragma once

#include <map>
#include <string>
#include <memory>

namespace Ubpa::Utopia {
	struct Shader;

	class ShaderMngr {
	public:
		static ShaderMngr& Instance() noexcept {
			static ShaderMngr instance;
			return instance;
		}

		void Register(std::shared_ptr<Shader>);
		std::shared_ptr<Shader> Get(std::string_view name) const;
		const std::map<std::string, std::weak_ptr<Shader>, std::less<>> GetShaderMap() const noexcept { return shaderMap; }
		// clear expired weak_ptr
		void Refresh();

	private:
		ShaderMngr() = default;
		std::map<std::string, std::weak_ptr<Shader>, std::less<>> shaderMap;
	};
}
