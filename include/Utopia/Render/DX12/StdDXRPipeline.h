#pragma once

#include "IPipeline.h"

namespace Ubpa::Utopia {
	class StdDXRPipeline final : public IPipeline {
	public:
		StdDXRPipeline(InitDesc desc);
		virtual ~StdDXRPipeline();

		virtual void Render(const std::vector<const UECS::World*>& worlds, std::span<const CameraData> cameraData, ID3D12Resource* default_rtb) override;

	private:
		struct Impl;
		Impl* pImpl;
	};
}