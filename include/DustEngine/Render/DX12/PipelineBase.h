#pragma once

#include <UDX12/UDX12.h>

#include <UECS/Entity.h>

#include <vector>

namespace Ubpa::UECS {
	class World;
}

namespace Ubpa::DustEngine {
	struct Material;

	class PipelineBase {
	public:
		struct InitDesc {
			size_t numFrame;
			ID3D12Device* device;
			ID3D12CommandQueue* cmdQueue;
			DXGI_FORMAT rtFormat;
		};
		struct CameraData {
			CameraData(UECS::Entity entity, const UECS::World& world)
				: entity{ entity }, world{ world } {}
			UECS::Entity entity;
			const UECS::World& world;
		};

		PipelineBase(InitDesc initDesc) : initDesc{ initDesc } {}

		virtual ~PipelineBase() = default;

		// data : cpu -> gpu
		// run in update
		virtual void BeginFrame(const std::vector<const UECS::World*>& worlds, const CameraData& cameraData) = 0;
		// run in draw
		virtual void Render(ID3D12Resource* rt) = 0;
		// run at the end of draw
		virtual void EndFrame() = 0;

		void Resize(
			size_t width,
			size_t height,
			D3D12_VIEWPORT screenViewport,
			D3D12_RECT scissorRect
		) {
			resizeData.width = width;
			resizeData.height = height;
			resizeData.screenViewport = screenViewport;
			resizeData.scissorRect = scissorRect;
			Impl_Resize();
		}

		static void SetGraphicsRootSRV(ID3D12GraphicsCommandList* cmdList, const Material* material);

	protected:
		virtual void Impl_Resize() = 0;

		struct ResizeData {
			size_t width{ 0 };
			size_t height{ 0 };
			D3D12_VIEWPORT screenViewport;
			D3D12_RECT scissorRect;
		};
		const InitDesc initDesc;
		const ResizeData& GetResizeData() const { return resizeData; }

	private:
		ResizeData resizeData;
	};
}
