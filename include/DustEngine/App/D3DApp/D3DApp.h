#pragma once

#include <UDX12/UDX12.h>

#include <dxgi1_4.h>

#include <WindowsX.h>

namespace Ubpa::DustEngine {
	class D3DApp {
	public:
		static D3DApp* GetApp() noexcept { return mApp; }

		HINSTANCE AppInst() const { return mhAppInst; }
		HWND MainWnd() const { return mhMainWnd; }

		// 1. process message
		// 2. game loop
		// - GameTimer Tick
		// - pause ?
		// - then CalculateFrameStats, Update, Draw
		// - else sleep
		int Run();

	protected:
		virtual void Update() = 0;
		virtual void Draw() = 0;

		virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;

	protected:
		bool InitMainWindow();
		bool InitDirect3D();
		void OnResize();

		float AspectRatio() const noexcept { return static_cast<float>(mClientWidth) / mClientHeight; }

		D3D12_VIEWPORT GetScreenViewport() const noexcept;
		D3D12_RECT GetScissorRect() const noexcept { return { 0, 0, mClientWidth, mClientHeight }; }

		// change in MsgProc
		bool mMinimized = false;  // is the application minimized?
		bool mMaximized = false;  // is the application maximized?
		bool mResizing = false;   // are the resize bars being dragged?
		bool mFullscreenState = false;// fullscreen enabled
		bool mAppPaused = false;  // is the application paused?
		int mClientWidth = 800;
		int mClientHeight = 600;

		void FlushCommandQueue();

		// Swap the back and front buffers
		void SwapBackBuffer();

		ID3D12Resource* CurrentBackBuffer() const noexcept { return mSwapChainBuffer[mCurrBackBuffer].Get(); }
		ID3D12Resource* GetDepthStencilBuffer() const noexcept { return mDepthStencilBuffer.Get(); }
		D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const noexcept;
		D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const noexcept { return mDsvHeap->GetCPUDescriptorHandleForHeapStart(); }
		DXGI_FORMAT GetBackBufferFormat() const noexcept { return mBackBufferFormat; }
		DXGI_FORMAT GetDepthStencilBufferFormat() const noexcept { return mDepthStencilFormat; }

	protected:
		D3DApp(HINSTANCE hInstance);
		virtual ~D3DApp();

		Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
		Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;

		// Derived class should set these in derived constructor to customize starting values.
		std::wstring mMainWndCaption = L"Ubpa D3D App";

		Ubpa::UDX12::Device uDevice;
		Ubpa::UDX12::CmdQueue uCmdQueue;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
		Ubpa::UDX12::GCmdList uGCmdList;

	private:
		inline static D3DApp* mApp{ nullptr };

		// create rtv and dsv for backbuffer(double buffer rtv x 2 + depth stencil buffer)
		void CreateRtvAndDsvDescriptorHeaps();
		void LogAdapters();
		void LogAdapterOutputs(IDXGIAdapter* adapter);
		void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);
		void CreateSwapChain();
		// command queue, alloc, list
		void CreateCommandObjects();

		int frameCnt{ 0 };
		float timeElapsed{ 0.0f };
		void CalculateFrameStats();

		HINSTANCE mhAppInst = nullptr; // application instance handle
		HWND      mhMainWnd = nullptr; // main window handle

		UINT mRtvDescriptorSize = 0;
		UINT mDsvDescriptorSize = 0;
		UINT mCbvSrvUavDescriptorSize = 0;

		static constexpr int SwapChainBufferCount = 2;
		int mCurrBackBuffer = 0;
		Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
		Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

		Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
		UINT64 mCurrentFence = 0;

		DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		D3DApp(const D3DApp& rhs) = delete;
		D3DApp& operator=(const D3DApp& rhs) = delete;

		static LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
	};
}
