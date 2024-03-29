#pragma once

#include <Utopia/Render/DX12/IPipelineStage.h>

namespace Ubpa::Utopia {
	class TAA : public IPipelineStage {
	public:
		TAA();
		virtual ~TAA();

		virtual void NewFrame() override;

		/**
		 * inputs
		 * - Previous Scene Color
		 * - Current Scene Color
		 * - Motion
		 */
		virtual bool RegisterInputOutputPassNodes(UFG::FrameGraph& framegraph, std::span<const size_t> inputNodeIDs) override;

		virtual void RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) override;

		void RegisterPassFuncData(D3D12_GPU_VIRTUAL_ADDRESS inCameraCBAddress);
		virtual void RegisterPassFunc(UDX12::FG::Executor& executor) override;

		/** mixed scene color */
		virtual std::span<const size_t> GetOutputNodeIDs() const override;
	private:
		std::shared_ptr<Shader> shader;

		static constexpr const char RsrcTableID[] = "TAA::RsrcTable";

		D3D12_SHADER_RESOURCE_VIEW_DESC prevSceneColorSrvDesc;
		D3D12_SHADER_RESOURCE_VIEW_DESC currSceneColorSrvDesc;
		D3D12_SHADER_RESOURCE_VIEW_DESC motionSrvDesc;

		D3D12_GPU_VIRTUAL_ADDRESS cameraCBAddress;

		size_t prevSceneColorID;
		size_t currSceneColorID;
		size_t motionID;

		size_t mixSceneColorID;

		size_t taaPassID;
		size_t copyPassID;
	};
}
