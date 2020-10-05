#pragma once

#include <UDX12/UDX12.h>

#include <dxgi1_4.h>

#include <WindowsX.h>

namespace Ubpa::Utopia {
	class DX12App {
	public:
		static DX12App* GetApp() noexcept { return mApp; }

		HINSTANCE AppInst() const { return mhAppInst; }
		HWND MainWnd() const { return mhMainWnd; }

		virtual bool Init() = 0;

		// 1. process message
		// 2. game loop
		// - GameTimer Tick
		// - pause ?
		// - then CalculateFrameStats, Update, Draw
		// - else sleep
		int Run();

		static constexpr uint8_t NumFrameResources = 3;
		static constexpr uint8_t NumSwapChainBuffer = 2;

	protected:
		virtual void Update() = 0;
		virtual void Draw() = 0;
		virtual void OnResize();

		virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	protected:
		bool InitMainWindow();
		bool InitDirect3D();

		float AspectRatio() const noexcept { return static_cast<float>(mClientWidth) / mClientHeight; }

		D3D12_VIEWPORT GetScreenViewport() const noexcept;
		D3D12_RECT GetScissorRect() const noexcept { return { 0, 0, mClientWidth, mClientHeight }; }

		// change in MsgProc
		bool mMinimized = false;  // is the application minimized?
		bool mMaximized = false;  // is the application maximized?
		bool mResizing = false;   // are the resize bars being dragged?
		bool mFullscreenState = false;// fullscreen enabled
		bool mAppPaused = false;  // is the application paused?
		int mClientWidth = 0;
		int mClientHeight = 0;

		void FlushCommandQueue();

		// Swap the back and front buffers
		void SwapBackBuffer();

		ID3D12Resource* CurrentBackBuffer() const noexcept { return mSwapChainBuffer[curBackBuffer].Get(); }
		D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const noexcept { return swapchainRTVCpuDH.GetCpuHandle(curBackBuffer); }
		DXGI_FORMAT GetBackBufferFormat() const noexcept { return mBackBufferFormat; }

		static constexpr char FR_CommandAllocator[] = "__CommandAllocator";
		UDX12::FrameResourceMngr* GetFrameResourceMngr() const noexcept { return frameRsrcMngr.get(); }
		ID3D12CommandAllocator* GetCurFrameCommandAllocator() noexcept;

	protected:
		DX12App(HINSTANCE hInstance);
		virtual ~DX12App();

		Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
		Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;

		// Derived class should set these in derived constructor to customize starting values.
		std::wstring mMainWndCaption = L"Ubpa D3D App";

		Ubpa::UDX12::Device uDevice;
		Ubpa::UDX12::CmdQueue uCmdQueue;
		Ubpa::UDX12::GCmdList uGCmdList;

		// for init, resize, run in "main frame"
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mainCmdAlloc;

	private:
		inline static DX12App* mApp{ nullptr };

		// command queue, alloc, list
		void CreateCommandObjects();
		// create rtv and dsv for backbuffer(double buffer rtv x 2 + depth stencil buffer)
		void CreateSwapChain();
		void CreateSwapChainDH();

		void LogAdapters();
		void LogAdapterOutputs(IDXGIAdapter* adapter);
		void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

		int frameCnt{ 0 };
		float timeElapsed{ 0.0f };
		void CalculateFrameStats();

		HINSTANCE mhAppInst = nullptr; // application instance handle
		HWND      mhMainWnd = nullptr; // main window handle

		int curBackBuffer = 0;
		Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[NumSwapChainBuffer];

		UDX12::DescriptorHeapAllocation swapchainRTVCpuDH;

		Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
		UINT64 mCurrentFence = 0;

		DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

		std::unique_ptr<UDX12::FrameResourceMngr> frameRsrcMngr;

		DX12App(const DX12App& rhs) = delete;
		DX12App& operator=(const DX12App& rhs) = delete;

		static LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
	};
}
