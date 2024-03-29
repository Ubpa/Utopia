#pragma once

#include "IPipeline.h"

namespace Ubpa::Utopia {
	class StdDXRPipeline final : public IPipeline {
	public:
		virtual ~StdDXRPipeline();

		virtual void Init(InitDesc desc) override;
		virtual bool IsInitialized() const override;
		virtual void Render(
			std::span<const UECS::World* const> worlds,
			std::span<const CameraData> cameras,
			std::span<const WorldCameraLink> links,
			std::span<ID3D12Resource* const> defaultRTs
		) override;
		virtual const std::map<std::string, FrameGraphData>& GetFrameGraphDataMap() const override;

	private:
		struct Impl;
		Impl* pImpl{ nullptr };
	};
}
