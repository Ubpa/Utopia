#include <Utopia/Render/ShaderMngr.h>

#include <Utopia/Render/Shader.h>

using namespace Ubpa::Utopia;


void ShaderMngr::Register(std::shared_ptr<Shader> shader) {
	std::lock_guard guard{ m };
	shaderMap[shader->name] = shader;
	id2name[shader->GetInstanceID()] = shader->name;
	shader->destroyed.Connect<&ShaderMngr::Unregister>(this);
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
	auto target = id2name.find(id);
	if (target == id2name.end())
		return;
	shaderMap.erase(target->second);
	id2name.erase(target);
}

ShaderMngr::~ShaderMngr() {
	for (const auto& [name, shader] : shaderMap) {
		if (shader.expired())
			continue;
		shader.lock()->destroyed.Disconnect(this);
	}
}
