#pragma once

#include "PipelineBase.h"

namespace Ubpa::Utopia {
	class StdPipeline final : public PipelineBase {
	public:
		StdPipeline(InitDesc desc);
		virtual ~StdPipeline();

		// data : cpu -> gpu
		// run in update
		virtual void BeginFrame(const std::vector<const UECS::World*>& worlds, const CameraData& cameraData) override;

		// run in draw
		virtual void Render(ID3D12Resource* rt) override;

		// run at the end of draw
		virtual void EndFrame() override;

	protected:
		virtual void Impl_Resize() override;

	private:
		struct Impl;
		Impl* pImpl;
	};
}
