#pragma once

#include "PipelineCommonUtils.h"

namespace Ubpa::Utopia {
	class ShaderCBMngr {
	public:
		ShaderCBMngr(ID3D12Device* device);

		void NewFrame();

		void RegisterRenderContext(const RenderContext& ctx,
			std::span<const CameraConstants* const> cameraConstantsSpan);

		D3D12_GPU_VIRTUAL_ADDRESS GetCameraCBAddress(size_t ctxID, size_t cameraIdx) const;
		D3D12_GPU_VIRTUAL_ADDRESS GetLightCBAddress(size_t ctxID) const;
		D3D12_GPU_VIRTUAL_ADDRESS GetDirectionalShadowCBAddress(size_t ctxID) const;
		D3D12_GPU_VIRTUAL_ADDRESS GetObjectCBAddress(size_t ctxID, size_t entityIdx) const;

		void SetGraphicsRoot_CBV_SRV(
			ID3D12GraphicsCommandList* cmdList,
			size_t ctxID,
			const Material& material,
			const std::map<std::string_view, D3D12_GPU_VIRTUAL_ADDRESS>& commonCBs,
			const std::map<std::string_view, D3D12_GPU_DESCRIPTOR_HANDLE>& commonSRVs
		) const;

	private:
		UDX12::DynamicUploadVector materialCBUploadVector;
		UDX12::DynamicUploadVector commonCBUploadVector; // camera, light, object
		
		struct RenderContextData {
			size_t cameraOffset = static_cast<size_t>(-1);
			size_t lightOffset = static_cast<size_t>(-1);
			size_t objectOffset = static_cast<size_t>(-1);
			size_t directionalShadowOffset = static_cast<size_t>(-1);

			std::unordered_map<size_t, size_t> entity2offset;

			std::unordered_map<size_t, MaterialCBDesc> materialCBDescMap; // material ID -> desc
		};
		std::map<size_t, RenderContextData> renderCtxDataMap;
		std::unordered_map<size_t, MaterialCBDesc> commonMaterialCBDescMap; // material ID -> desc
	};
}