#pragma once

#include <UDX12/UDX12.h>

namespace Ubpa::UECS {
	class World;
}

namespace Ubpa::DustEngine {
	class IPipeline {
	public:
		struct InitDesc {
			size_t numFrame;
			ID3D12Device* device;
			ID3D12CommandQueue* cmdQueue;
			DXGI_FORMAT backBufferFormat;
			DXGI_FORMAT depthStencilFormat;
		};

		IPipeline(InitDesc initDesc) : initDesc{ initDesc } {}

		virtual ~IPipeline() = default;

		virtual void UpdateRenderContext(const UECS::World& world) = 0;

		virtual void Render(ID3D12Resource* curBackBuffer) = 0;
		virtual void EndFrame() = 0;

		void Resize(
			size_t width,
			size_t height,
			D3D12_VIEWPORT screenViewport,
			D3D12_RECT scissorRect,
			ID3D12Resource* depthStencilBuffer
		) {
			resizeData.width = width;
			resizeData.height = height;
			resizeData.screenViewport = screenViewport;
			resizeData.scissorRect = scissorRect;
			resizeData.depthStencilBuffer = depthStencilBuffer;
			Impl_Resize();
		}

	protected:
		virtual void Impl_Resize() = 0;

		struct ResizeData {
			size_t width{ 0 };
			size_t height{ 0 };
			D3D12_VIEWPORT screenViewport;
			D3D12_RECT scissorRect;
			ID3D12Resource* depthStencilBuffer{ nullptr };
		};
		const InitDesc initDesc;
		const ResizeData& GetResizeData() const { return resizeData; }

	private:
		ResizeData resizeData;
	};
}
