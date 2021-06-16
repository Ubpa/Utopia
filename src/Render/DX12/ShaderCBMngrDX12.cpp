#include <Utopia/Render/DX12/ShaderCBMngrDX12.h>

#include <Utopia/Render/Shader.h>

using namespace Ubpa::Utopia;

Ubpa::UDX12::DynamicUploadBuffer* ShaderCBMngrDX12::GetBuffer(const Shader& shader) {
	auto target = bufferMap.find(shader.GetInstanceID());
	if (target != bufferMap.end())
		return target->second.get();
	{
		auto conn = shader.destroyed.ScopeConnect<&ShaderCBMngrDX12::Unregister>(this);
		connections.push_back(std::move(conn));
	}
	auto rst = bufferMap.emplace_hint(
		target,
		shader.GetInstanceID(),
		std::make_unique<UDX12::DynamicUploadBuffer>(device, 0)
	);
	return rst->second.get();
}

Ubpa::UDX12::DynamicUploadBuffer* ShaderCBMngrDX12::GetCommonBuffer() {
	constexpr std::size_t ID = static_cast<std::size_t>(-1);
	auto target = bufferMap.find(ID);
	if (target != bufferMap.end())
		return target->second.get();

	auto rst = bufferMap.emplace_hint(
		target,
		ID,
		std::make_unique<UDX12::DynamicUploadBuffer>(device, 0)
	);
	return rst->second.get();
}

void ShaderCBMngrDX12::Unregister(std::size_t ID) {
	std::lock_guard guard{ m };
	bufferMap.erase(ID);
}
