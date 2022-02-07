#pragma once

#include <USignal/USignal.hpp>

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
		void Clear();

	private:
		ShaderMngr() = default;
		~ShaderMngr();
		void UnregisterOnDestroyed(std::size_t id);
		mutable std::shared_mutex m;
		std::map<std::string, std::weak_ptr<Shader>, std::less<>> shaderMap;
		struct ConnectData {
			ScopedConnection<void(std::size_t)> conn;
			std::string name;
		};
		std::map<std::size_t, ConnectData> id2data;
	};
}
