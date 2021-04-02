#pragma once

#include <map>
#include <string>
#include <memory>
#include <shared_mutex>

namespace Ubpa::Utopia {
	struct Shader;

	class ShaderMngr {
	public:
		static ShaderMngr& Instance() noexcept {
			static ShaderMngr instance;
			return instance;
		}

		void Register(std::shared_ptr<Shader>);
		void Unregister(std::size_t id);
		std::shared_ptr<Shader> Get(std::string_view name) const;
		const std::map<std::string, std::weak_ptr<Shader>, std::less<>> GetShaderMap() const noexcept { return shaderMap; }

	private:
		ShaderMngr() = default;
		~ShaderMngr();
		mutable std::shared_mutex m;
		std::map<std::string, std::weak_ptr<Shader>, std::less<>> shaderMap;
		std::map<std::size_t, std::string> id2name;
	};
}
