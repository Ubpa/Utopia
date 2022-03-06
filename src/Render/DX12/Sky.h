#pragma once

#include <Utopia/Render/DX12/IPipelineStage.h>

namespace Ubpa::Utopia {
	class Sky : public IPipelineStage {
	public:
		Sky();
		virtual ~Sky();

		virtual void NewFrame() override;

		/**
		 * inputs
		 * - Depth Stencil
		 */
		virtual bool RegisterInputOutputPassNodes(UFG::FrameGraph& framegraph, std::span<const size_t> inputNodeIDs) override;

		virtual void RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) override;

		void RegisterPassFuncData(
			D3D12_GPU_DESCRIPTOR_HANDLE inSkyboxSrvGpuHandle,
			D3D12_GPU_VIRTUAL_ADDRESS inCameraCBAddress);

		virtual void RegisterPassFunc(UDX12::FG::Executor& executor) override;

		/** mixed scene color */
		virtual std::span<const size_t> GetOutputNodeIDs() const override;
	private:
		std::shared_ptr<Shader> shader;

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;

		D3D12_GPU_DESCRIPTOR_HANDLE skyboxSrvGpuHandle;
		D3D12_GPU_VIRTUAL_ADDRESS cameraCBAddress;

		size_t depthStencilID;

		size_t outputID;

		size_t passID;
	};
}
