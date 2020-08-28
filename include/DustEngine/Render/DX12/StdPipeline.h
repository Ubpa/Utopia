#pragma once

#include "IPipeline.h"

namespace Ubpa::DustEngine {
	class StdPipeline final : public IPipeline {
	public:
		StdPipeline(InitDesc desc);
		virtual ~StdPipeline();

		virtual void UpdateRenderContext(const UECS::World&) override;

		virtual void Render(ID3D12Resource* curBackBuffer) override;
		virtual void EndFrame() override;

	protected:
		virtual void Impl_Resize() override;

	private:
		struct Impl;
		Impl* pImpl;
	};
}
