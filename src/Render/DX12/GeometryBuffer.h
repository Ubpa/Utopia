#pragma once

#include "IPipelineStage.h"

namespace Ubpa::Utopia {
	class GeometryBuffer : public IPipelineStage {
	public:
		GeometryBuffer(bool inNeedLinearZ = false);
		virtual ~GeometryBuffer();

		virtual void NewFrame() override;

		/** Empty input nodes. */
		virtual bool RegisterInputOutputPassNodes(UFG::FrameGraph& framegraph, std::span<const size_t> inputNodeIDs) override;

		virtual void RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) override;

		void RegisterPassFuncData(
			D3D12_GPU_VIRTUAL_ADDRESS inCameraCBAddress,
			const ShaderCBMngr* inShaderCBMngr,
			const RenderContext* inRenderCtx,
			std::string inLightMode);

		virtual void RegisterPassFunc(UDX12::FG::Executor& executor) override;

		/**
		 * Geometry Buffer 0
		 * Geometry Buffer 1
		 * Geometry Buffer 2
		 * Motion
		 * Depth Stencil
		 * (optional) Linear Z
		 */
		virtual std::span<const size_t> GetOutputNodeIDs() const override;
	private:
		bool needLinearZ;

		std::shared_ptr<Shader> shader;

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;

		D3D12_GPU_VIRTUAL_ADDRESS cameraCBAddress;
		const ShaderCBMngr* shaderCBMngr;
		const RenderContext* renderCtx;
		std::string lightMode;

		/**
		 * GeometryBuffer0
		 * GeometryBuffer1
		 * GeometryBuffer2
		 * Motion
		 * Depth Stencil
		 * (optional) Linear Z
		 */
		size_t outputIDs[6];

		size_t passID;
	};
}
