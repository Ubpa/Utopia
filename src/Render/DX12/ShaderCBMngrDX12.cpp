#include <Utopia/Render/DX12/ShaderCBMngrDX12.h>

#include <Utopia/Render/Shader.h>

using namespace Ubpa::Utopia;

ShaderCBMngrDX12::~ShaderCBMngrDX12() {
	for (auto [shaderID, buffer] : bufferMap) {
		delete buffer;
	}
}

Ubpa::UDX12::DynamicUploadBuffer* ShaderCBMngrDX12::GetBuffer(const Shader& shader) {
	auto target = bufferMap.find(shader.GetInstanceID());
	if (target != bufferMap.end())
		return target->second;

	auto rst = bufferMap.emplace_hint(
		target,
		shader.GetInstanceID(),
		new UDX12::DynamicUploadBuffer{ device }
	);
	return rst->second;
}

Ubpa::UDX12::DynamicUploadBuffer* ShaderCBMngrDX12::GetCommonBuffer() {
	size_t ID = static_cast<size_t>(-1);
	auto target = bufferMap.find(ID);
	if (target != bufferMap.end())
		return target->second;

	auto rst = bufferMap.emplace_hint(
		target,
		ID,
		new UDX12::DynamicUploadBuffer{ device }
	);
	return rst->second;
}
