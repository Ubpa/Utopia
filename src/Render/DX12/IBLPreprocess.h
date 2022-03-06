#pragma once

#include <Utopia/Render/DX12/IPipelineStage.h>

namespace Ubpa::Utopia {
	class IBLPreprocess : public IPipelineStage {
	public:
		IBLPreprocess();
		virtual ~IBLPreprocess();

		virtual void NewFrame() override;

		/** no input */
		virtual bool RegisterInputOutputPassNodes(UFG::FrameGraph& framegraph, std::span<const size_t> inputNodeIDs) override;

		virtual void RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) override;
		void RegisterPassFuncData(IBLData* inIblData, D3D12_GPU_DESCRIPTOR_HANDLE inSkyboxSrvGpuHandle);
		virtual void RegisterPassFunc(UDX12::FG::Executor& executor) override;

		/**
		 * Irradiance Map
		 * Prefilter Map
		 */
		virtual std::span<const size_t> GetOutputNodeIDs() const override;
	private:
		std::shared_ptr<Shader> irradianceShader;
		std::shared_ptr<Shader> prefilterShader;

		IBLData* iblData;
		D3D12_GPU_DESCRIPTOR_HANDLE skyboxSrvGpuHandle;

		/**
		 * Irradiance Map
		 * Prefilter Map
		 */
		size_t outputIDs[2];

		size_t passID;
	};
}
