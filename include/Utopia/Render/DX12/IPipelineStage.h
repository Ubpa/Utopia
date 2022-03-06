#pragma once

#include <Utopia/Render/DX12/PipelineCommonUtils.h>

namespace Ubpa::Utopia {
	class IPipelineStage {
	public:
		virtual ~IPipelineStage() = default;

		virtual void NewFrame() = 0;

		virtual bool RegisterInputOutputPassNodes(UFG::FrameGraph& framegraph, std::span<const size_t> inputNodeIDs) = 0;

		virtual void RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) = 0;
		virtual void RegisterPassFunc(UDX12::FG::Executor& executor) = 0;

		virtual std::span<const size_t> GetOutputNodeIDs() const = 0;
	};
}
