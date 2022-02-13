#pragma once

#include "PipelineCommonUtils.h"

namespace Ubpa::Utopia {
	struct MaterialCBDesc {
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

	class ShaderCBMngr {
	public:
		ShaderCBMngr(ID3D12Device* device);

		void NewFrame();

		void RegisterRenderContext(const RenderContext& ctx,
			std::span<const CameraConstants* const> cameraConstantsSpan);

		D3D12_GPU_VIRTUAL_ADDRESS GetCameraCBAddress(size_t ctxID, size_t cameraIdx) const;
		D3D12_GPU_VIRTUAL_ADDRESS GetLightCBAddress(size_t ctxID) const;
		D3D12_GPU_VIRTUAL_ADDRESS GetObjectCBAddress(size_t ctxID, size_t entityIdx) const;

		void SetGraphicsRoot_CBV_SRV(
			ID3D12GraphicsCommandList* cmdList,
			size_t ctxID,
			const Shader* shader,
			const Material& material,
			const std::map<std::string_view, D3D12_GPU_VIRTUAL_ADDRESS>& commonCBs,
			const std::map<std::string_view, D3D12_GPU_DESCRIPTOR_HANDLE>& commonSRVs
		) const;

	private:
		MaterialCBDesc RegisterShaderMaterialCB(
			const Shader* shader,
			const std::unordered_set<const Material*>& materials
		);

		UDX12::DynamicUploadVector materialCBUploadVector;
		UDX12::DynamicUploadVector commonCBUploadVector; // camera, light, object
		
		struct RenderContextData {
			size_t cameraOffset = static_cast<size_t>(-1);
			size_t lightOffset = static_cast<size_t>(-1);
			size_t objectOffset = static_cast<size_t>(-1);

			std::unordered_map<size_t, size_t> entity2offset;

			std::unordered_map<const Shader*, MaterialCBDesc> materialCBDescMap; // shader ID -> desc
		};
		std::map<size_t, RenderContextData> renderCtxDataMap;
	};
}