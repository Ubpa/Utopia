#pragma once

#include "IPipeline.h"

namespace Ubpa::Utopia {
	class StdPipeline final : public IPipeline {
	public:
		virtual ~StdPipeline();

		virtual void Init(InitDesc desc) override;
		virtual void Render(
			std::span<const UECS::World* const> worlds,
			std::span<const CameraData> cameras,
			std::span<const WorldCameraLink> links, // worlds -> cameras
			std::span<ID3D12Resource* const> defaultRTs // camera index -> rt
		) override;

	private:
		struct Impl;
		Impl* pImpl{ nullptr };
	};
}
