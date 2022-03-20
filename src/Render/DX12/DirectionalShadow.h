#pragma once

#include <Utopia/Render/DX12/IPipelineStage.h>

namespace Ubpa::Utopia {
	class DirectionalShadow : public IPipelineStage {
	public:
		DirectionalShadow();
		virtual ~DirectionalShadow();

		virtual void NewFrame() override;

		/** Empty input nodes. */
		virtual bool RegisterInputOutputPassNodes(UFG::FrameGraph& framegraph, std::span<const size_t> inputNodeIDs) override;

		virtual void RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) override;

		void RegisterPassFuncData(
			const ShaderCBMngr* inShaderCBMngr,
			const RenderContext* inRenderCtx);

		virtual void RegisterPassFunc(UDX12::FG::Executor& executor) override;

		virtual std::span<const size_t> GetOutputNodeIDs() const override;
	private:
		std::shared_ptr<Material> material;

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;

		const ShaderCBMngr* shaderCBMngr;
		const RenderContext* renderCtx;

		size_t depthID;

		size_t passID;
	};
}
