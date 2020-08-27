#include <DustEngine/Core/ShaderMngr.h>

#include <DustEngine/Core/Shader.h>

using namespace Ubpa::DustEngine;


void ShaderMngr::Register(Shader* shader) {
	shaderMap[shader->shaderName] = shader;
}

Shader* ShaderMngr::Get(std::string_view name) const {
	auto target = shaderMap.find(name);
	if (target == shaderMap.end())
		return nullptr;

	return target->second;
}
