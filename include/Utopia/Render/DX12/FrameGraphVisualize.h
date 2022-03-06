#pragma once

#include <Utopia/Render/DX12/IPipelineStage.h>

namespace Ubpa::Utopia {
	class FrameGraphVisualize : public IPipelineStage {
	public:
		static constexpr const char NamePrefix[] = "FrameGraphVisualize";

		FrameGraphVisualize();
		virtual ~FrameGraphVisualize();

		virtual void NewFrame() override;

		virtual bool RegisterInputOutputPassNodes(UFG::FrameGraph& framegraph, std::span<const size_t> inputNodeIDs) override;

		void RegisterPassResourcesData(size_t inWidth, size_t inHeight);
		virtual void RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) override;
		virtual void RegisterPassFunc(UDX12::FG::Executor& executor) override;

		virtual std::span<const size_t> GetOutputNodeIDs() const override;
		std::span<const size_t> GetInputNodeIDs() const;
		std::span<const D3D12_GPU_DESCRIPTOR_HANDLE> GetOutputSrvGpuHandles() const;
		std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> GetOutputSrvCpuHandles() const;
	private:
		static constexpr DXGI_FORMAT outputFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

		std::shared_ptr<Shader> shader;
		D3D12_SHADER_RESOURCE_VIEW_DESC outputSrvDesc;

		size_t width;
		size_t height;

		std::vector<size_t> inputIDs;
		std::vector<size_t> inputOriginIDs;
		std::vector<size_t> outputIDs;
		std::vector<size_t> passIDs;
		size_t uiPassID;

		std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> outputSrvGpuHandles;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> outputSrvCpuHandles;
	};
}
