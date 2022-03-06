#pragma once

#include <Utopia/Render/DX12/IPipelineStage.h>

namespace Ubpa::Utopia {
	class DeferredLighting : public IPipelineStage {
	public:
		DeferredLighting();
		virtual ~DeferredLighting();

		virtual void NewFrame() override;

		/**
		 * Geometry Buffer 0
		 * Geometry Buffer 1
		 * Geometry Buffer 2
		 * Depth Stencil
		 * Irradiance Map
		 * Prefilter Map
		 */
		virtual bool RegisterInputOutputPassNodes(UFG::FrameGraph& framegraph, std::span<const size_t> inputNodeIDs) override;

		virtual void RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) override;

		void RegisterPassFuncData(
			D3D12_GPU_DESCRIPTOR_HANDLE inIblDataSrvGpuHandle,
			D3D12_GPU_VIRTUAL_ADDRESS inCameraCBAddress,
			D3D12_GPU_VIRTUAL_ADDRESS inLightCBAddress
		);
		virtual void RegisterPassFunc(UDX12::FG::Executor& executor) override;

		/** output */
		virtual std::span<const size_t> GetOutputNodeIDs() const override;
	private:
		std::shared_ptr<Shader> shader;

		static constexpr const char RsrcTableID[] = "DeferredLighting::RsrcTable";

		D3D12_SHADER_RESOURCE_VIEW_DESC geometryBufferSharedSrvDesc;
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		D3D12_SHADER_RESOURCE_VIEW_DESC dsSrvDesc;

		/** irradiance, prefilter, BRDF LUT */
		D3D12_GPU_DESCRIPTOR_HANDLE iblDataSrvGpuHandle;
		D3D12_GPU_VIRTUAL_ADDRESS cameraCBAddress;
		D3D12_GPU_VIRTUAL_ADDRESS lightCBAddress;

		size_t geometryBuffer0ID;
		size_t geometryBuffer1ID;
		size_t geometryBuffer2ID;
		size_t depthStencilID;
		size_t irradianceMapID;
		size_t prefilterMapID;

		size_t outputID;

		size_t passID;
	};
}
