#pragma once

#include "IPipeline.h"

namespace Ubpa::Utopia {
	class StdPipeline final : public IPipeline {
	public:
		StdPipeline(InitDesc desc);
		virtual ~StdPipeline();

		virtual void Render(const std::vector<const UECS::World*>& worlds, const CameraData& cameraData, ID3D12Resource* default_rtb) override;

	private:
		struct Impl;
		Impl* pImpl;
	};
}
