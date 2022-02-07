#pragma once

#include "PipelineCommonUtils.h"

namespace Ubpa::Utopia {
	class IPipelineStage {
	public:
		virtual ~IPipelineStage() = default;

		virtual void NewFrame() = 0;

		virtual bool RegisterInputNodes(std::span<const size_t> inputNodeIDs) = 0;
		virtual void RegisterOutputNodes(UFG::FrameGraph& framegraph) = 0;
		virtual void RegisterPass(UFG::FrameGraph& framegraph) = 0;

		virtual void RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) = 0;
		virtual void RegisterPassFunc(UDX12::FG::Executor& executor) = 0;

		virtual std::span<const size_t> GetOutputNodeIDs() const = 0;
	};
}
