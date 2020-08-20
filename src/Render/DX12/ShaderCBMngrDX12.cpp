#include <DustEngine/Render/DX12/ShaderCBMngrDX12.h>

#include <DustEngine/Core/Shader.h>

using namespace Ubpa::DustEngine;

ShaderCBMngrDX12::~ShaderCBMngrDX12() {
	for (auto [shaderID, buffer] : bufferMap) {
		delete buffer;
	}
}

Ubpa::UDX12::DynamicUploadBuffer* ShaderCBMngrDX12::GetBuffer(const Shader* shader) {
	auto target = bufferMap.find(shader->GetInstanceID());
	if (target != bufferMap.end())
		return target->second;

	auto rst = bufferMap.emplace_hint(
		target,
		shader->GetInstanceID(),
		new UDX12::DynamicUploadBuffer{ device }
	);
	return rst->second;
}
