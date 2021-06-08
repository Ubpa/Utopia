#include <Utopia/Render/ShaderMngr.h>

#include <Utopia/Render/Shader.h>

using namespace Ubpa::Utopia;


void ShaderMngr::Register(std::shared_ptr<Shader> shader) {
	std::lock_guard guard{ m };
	shaderMap[shader->name] = shader;

	id2data.emplace(shader->GetInstanceID(), ConnectData{
		shader->destroyed.ScopeConnect<&ShaderMngr::Unregister>(this),
		shader->name
	});
}

std::shared_ptr<Shader> ShaderMngr::Get(std::string_view name) const {
	std::shared_lock guard{ m };

	auto target = shaderMap.find(name);
	if (target == shaderMap.end() || target->second.expired())
		return nullptr;

	return target->second.lock();
}

void ShaderMngr::Unregister(std::size_t id) {
	std::lock_guard guard{ m };
	auto target = id2data.find(id);
	if (target == id2data.end())
		return;
	shaderMap.erase(target->second.name);
	id2data.erase(target);
}

ShaderMngr::~ShaderMngr() {
	for (const auto& [name, shader] : shaderMap) {
		if (shader.expired())
			continue;
		shader.lock()->destroyed.Disconnect(this);
	}
}
