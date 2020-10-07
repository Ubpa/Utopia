#include <Utopia/Render/ShaderMngr.h>

#include <Utopia/Render/Shader.h>

using namespace Ubpa::Utopia;


void ShaderMngr::Register(std::shared_ptr<Shader> shader) {
	shaderMap[shader->name] = shader;
}

std::shared_ptr<Shader> ShaderMngr::Get(std::string_view name) const {
	auto target = shaderMap.find(name);
	if (target == shaderMap.end() || target->second.expired())
		return nullptr;

	return target->second.lock();
}

void ShaderMngr::Refresh() {
	auto iter = shaderMap.begin();
	while (iter != shaderMap.end()) {
		auto iter_copy = iter;
		++iter;
		if (iter->second.expired())
			shaderMap.erase(iter);
	}
}
