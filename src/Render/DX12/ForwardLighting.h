#pragma once

#include "IPipelineStage.h"

namespace Ubpa::Utopia {
	class ForwardLighting : public IPipelineStage {
	public:
		ForwardLighting();
		virtual ~ForwardLighting();

		virtual void NewFrame() override;

		/**
		 * Irradiance Map
		 * Prefilltered Map
		 */
		virtual bool RegisterInputOutputPassNodes(UFG::FrameGraph& framegraph, std::span<const size_t> inputNodeIDs) override;

		virtual void RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) override;


		void RegisterPassFuncData(
			D3D12_GPU_DESCRIPTOR_HANDLE inIblDataSrvGpuHandle,
			D3D12_GPU_VIRTUAL_ADDRESS inCameraCBAddress,
			const ShaderCBMngr* inShaderCBMngr,
			const RenderContext* inRenderCtx);

		virtual void RegisterPassFunc(UDX12::FG::Executor& executor) override;

		/**
		 * Lighted Result
		 * Depth Stencil
		 */
		virtual std::span<const size_t> GetOutputNodeIDs() const override;
	private:
		std::shared_ptr<Shader> shader;

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;

		D3D12_GPU_DESCRIPTOR_HANDLE iblDataSrvGpuHandle;
		D3D12_GPU_VIRTUAL_ADDRESS cameraCBAddress;
		const ShaderCBMngr* shaderCBMngr;
		const RenderContext* renderCtx;

		size_t irradianceMapID;
		size_t prefillteredMapID;

		/**
		 * Lighted Result ID
		 * Depth Stencil ID
		 */
		size_t outputIDs[2];

		size_t passID;
	};
}
