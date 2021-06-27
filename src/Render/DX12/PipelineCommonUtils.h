#pragma once

#include <UDX12/UDX12.h>

#include <vector>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

namespace Ubpa::Utopia {
	struct Material;
	struct Shader;
	struct RenderState;
}
namespace Ubpa::Utopia::PipelineCommonUtils {
	struct ShaderCBDesc {
		// material 0 cbs   -- offset
		//  material 0 cb 0
		//  material 0 cb 1
		//  ...
		// material 1 cbs   -- offset + materialCBSize
		// ...

		// global offset = begin_offset + indexMap[material] * materialCBSize + offsetMap[register index]
		size_t begin_offset;
		size_t materialCBSize{ 0 };
		std::map<size_t, size_t> offsetMap; // register index -> local offset
		std::unordered_map<size_t, size_t> indexMap; // material ID -> index
	};

	ShaderCBDesc UpdateShaderCBs(
		UDX12::DynamicUploadVector& cb,
		const Shader& shader,
		const std::unordered_set<const Material*>& materials,
		const std::set<std::string_view>& commonCBs
	);

	void SetGraphicsRoot_CBV_SRV(
		ID3D12GraphicsCommandList* cmdList,
		UDX12::DynamicUploadVector& cb,
		const ShaderCBDesc& shaderCBDesc,
		const Material& material,
		const std::map<std::string_view, D3D12_GPU_VIRTUAL_ADDRESS>& commonCBs,
		const std::map<std::string_view, D3D12_GPU_DESCRIPTOR_HANDLE>& commonSRVs
	);
}