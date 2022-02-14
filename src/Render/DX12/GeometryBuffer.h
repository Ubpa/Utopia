#pragma once

#include "IPipelineStage.h"

namespace Ubpa::Utopia {
	class GeometryBuffer : public IPipelineStage {
	public:
		GeometryBuffer();
		virtual ~GeometryBuffer();

		virtual void NewFrame() override;

		/** empty */
		virtual bool RegisterInputNodes(std::span<const size_t> inputNodeIDs) override;
		virtual void RegisterOutputNodes(UFG::FrameGraph& framegraph) override;
		virtual void RegisterPass(UFG::FrameGraph& framegraph) override;

		virtual void RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) override;

		void RegisterPassFuncData(
			D3D12_GPU_VIRTUAL_ADDRESS inCameraCBAddress,
			const ShaderCBMngr* inShaderCBMngr,
			const RenderContext* inRenderCtx,
			const IBLData* inIblData);

		virtual void RegisterPassFunc(UDX12::FG::Executor& executor) override;

		/**
		 * Geometry Buffer 0
		 * Geometry Buffer 1
		 * Geometry Buffer 2
		 * Motion
		 * Depth Stencil
		 */
		virtual std::span<const size_t> GetOutputNodeIDs() const override;
	private:
		std::shared_ptr<Shader> shader;

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;

		D3D12_GPU_VIRTUAL_ADDRESS cameraCBAddress;
		const ShaderCBMngr* shaderCBMngr;
		const RenderContext* renderCtx;
		const IBLData* iblData;

		/**
		 * GeometryBuffer0
		 * GeometryBuffer1
		 * GeometryBuffer2
		 * Motion
		 * Depth Stencil
		 */
		size_t outputIDs[5];

		size_t geometryBufferPassID;
	};
}
