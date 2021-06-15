#pragma once

#include "PipelineBase.h"

namespace Ubpa::Utopia {
	class StdDXRPipeline final : public PipelineBase {
	public:
		StdDXRPipeline(InitDesc desc);
		virtual ~StdDXRPipeline();

		virtual void Render(const std::vector<const UECS::World*>& worlds, const CameraData& cameraData, ID3D12Resource* default_rtb) override;

	private:
		struct Impl;
		Impl* pImpl;
	};
}
