#pragma once

#include "IPipelineStage.h"

namespace Ubpa::Utopia {
	class Tonemapping : public IPipelineStage {
	public:
		Tonemapping();
		virtual ~Tonemapping();

		virtual void NewFrame() override;

		virtual bool RegisterInputOutputPassNodes(UFG::FrameGraph& framegraph, std::span<const size_t> inputNodeIDs) override;

		virtual void RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) override;
		virtual void RegisterPassFunc(UDX12::FG::Executor& executor) override;

		virtual std::span<const size_t> GetOutputNodeIDs() const override;
	private:
		std::shared_ptr<Shader> shader;
		D3D12_SHADER_RESOURCE_VIEW_DESC inputSrvDesc;

		size_t inputID;
		size_t outputID;
		size_t passID;
	};
}
