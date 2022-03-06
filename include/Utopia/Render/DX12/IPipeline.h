#pragma once

#include <UDX12/UDX12.h>

#include <UECS/Entity.hpp>

#include <UFG/UFG.hpp>

#include <vector>
#include <span>

namespace Ubpa::UECS {
	class World;
}

namespace Ubpa::Utopia {
	class FrameGraphVisualize;

	class IPipeline {
	public:
		struct InitDesc {
			size_t numFrame;
			ID3D12Device* device;
			ID3D12CommandQueue* cmdQueue;
		};
		struct CameraData {
			UECS::Entity entity;
			const UECS::World* world;
		};
		struct WorldCameraLink {
			std::vector<size_t> worldIndices;
			std::vector<size_t> cameraIndices;
		};
		struct FrameGraphData {
			UFG::FrameGraph fg;
			const FrameGraphVisualize* stage{ nullptr };
		};

		virtual ~IPipeline() = default;

		virtual void Init(InitDesc desc) = 0;
		virtual bool IsInitialized() const = 0;
		virtual void Render(
			std::span<const UECS::World* const> worlds,
			std::span<const CameraData> cameras,
			std::span<const WorldCameraLink> links,
			std::span<ID3D12Resource* const> defaultRTs
		) = 0;
		virtual const std::map<std::string, FrameGraphData>& GetFrameGraphDataMap() const = 0;
	};
}
