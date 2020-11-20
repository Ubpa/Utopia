#include <Utopia/App/DX12App/DX12App.h>

#include <Utopia/Render/DX12/RsrcMngrDX12.h>

#include <Utopia/Core/GameTimer.h>

using Microsoft::WRL::ComPtr;
using namespace Ubpa::Utopia;
using namespace DirectX;
using namespace std;

DX12App::DX12App(HINSTANCE hInstance)
	: mhAppInst(hInstance)
{
	// Only one DX12App can be constructed.
	assert(mApp == nullptr);
	mApp = this;
}

DX12App::~DX12App() {
	Ubpa::Utopia::RsrcMngrDX12::Instance().Clear(uCmdQueue.Get());

	if (!uDevice.IsNull())
		FlushCommandQueue();
	if(!swapchainRTVCpuDH.IsNull())
		Ubpa::UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Free(std::move(swapchainRTVCpuDH));

	Ubpa::UDX12::DescriptorHeapMngr::Instance().Clear();

	mApp = nullptr;
}

LRESULT CALLBACK DX12App::MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
	return DX12App::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

LRESULT DX12App::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg)
	{
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mAppPaused = true;
			GameTimer::Instance().Stop();
		}
		else
		{
			mAppPaused = false;
			GameTimer::Instance().Start();
		}
		return 0;

		// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		mClientWidth = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if (!uDevice.IsNull())
		{
			if (wParam == SIZE_MINIMIZED)
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{

				// Restoring from minimized state?
				if (mMinimized)
				{
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}

				// Restoring from maximized state?
				else if (mMaximized)
				{
					mAppPaused = false;
					mMaximized = false;
					OnResize();
				}
				else if (mResizing)
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing = true;
		GameTimer::Instance().Stop();
		return 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing = false;
		GameTimer::Instance().Start();
		OnResize();
		return 0;

		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// The WM_MENUCHAR message is sent when a menu is active and the user presses 
		// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		return 0;
	case WM_MOUSEMOVE:
		return 0;
	case WM_KEYUP:
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}


int DX12App::Run() {
	MSG msg = { 0 };

	Ubpa::Utopia::GameTimer::Instance().Reset();

	while (msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else { // Otherwise, do animation/game stuff.
			Ubpa::Utopia::GameTimer::Instance().Tick();

			if (!mAppPaused) {
				CalculateFrameStats();
				Update();
				Draw();
			}
			else
				Sleep(100);
		}
	}

	return (int)msg.wParam;
}

void DX12App::OnResize() {
	assert(!uDevice.IsNull());
	assert(mSwapChain);
	assert(mainCmdAlloc);

	// Flush before changing any resources.
	FlushCommandQueue();

	ThrowIfFailed(uGCmdList->Reset(mainCmdAlloc.Get(), nullptr));

	// Release the previous resources we will be recreating.
	for (int i = 0; i < NumSwapChainBuffer; ++i)
		mSwapChainBuffer[i].Reset();

	// Resize the swap chain.
	ThrowIfFailed(mSwapChain->ResizeBuffers(
		NumSwapChainBuffer,
		mClientWidth, mClientHeight,
		mBackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	curBackBuffer = 0;

	for (UINT i = 0; i < NumSwapChainBuffer; i++) {
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		uDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, swapchainRTVCpuDH.GetCpuHandle(i));
	}

	// Execute the resize commands.
	ThrowIfFailed(uGCmdList->Close());
	uCmdQueue.Execute(uGCmdList.raw.Get());

	// Wait until resize is complete.
	FlushCommandQueue();
}

bool DX12App::InitMainWindow() {
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = mhAppInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd";

	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	mhMainWnd = CreateWindow(L"MainWnd", mMainWndCaption.c_str(),
		WS_MAXIMIZE | WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, mhAppInst, 0);
	if (!mhMainWnd)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return true;
}

bool DX12App::InitDirect3D() {
#if defined(DEBUG) || defined(_DEBUG) 
	// Enable the D3D12 debug layer.
	{
		ComPtr<ID3D12Debug> debugController;
		//ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
		if (debugController)
			debugController->EnableDebugLayer();
	}
#endif

	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

	// Try to create hardware device.
	// maybe thorw execption internal
	// https://github.com/Meith/DX12/issues/1
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // default adapter
		D3D_FEATURE_LEVEL_12_0,
		IID_PPV_ARGS(&uDevice.raw));

	// Fallback to WARP device.
	if (FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_12_0,
			IID_PPV_ARGS(&uDevice.raw)));
	}

	ThrowIfFailed(uDevice->CreateFence(mCurrentFence, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)));


	Ubpa::Utopia::RsrcMngrDX12::Instance().Init(uDevice.raw.Get());
	Ubpa::UDX12::DescriptorHeapMngr::Instance().Init(uDevice.Get(), 16384, 16384, 16384, 16384, 16384);

	frameRsrcMngr = std::make_unique<Ubpa::UDX12::FrameResourceMngr>(NumFrameResources, uDevice.Get());
	for (const auto& fr : frameRsrcMngr->GetFrameResources()) {
		ComPtr<ID3D12CommandAllocator> allocator;
		ThrowIfFailed(uDevice->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&allocator)));
		fr->RegisterResource(FR_CommandAllocator, allocator);
	}

	//D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	//msQualityLevels.Format = mBackBufferFormat;
	//msQualityLevels.SampleCount = 4;
	//msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	//msQualityLevels.NumQualityLevels = 0;
	//ThrowIfFailed(uDevice->CheckFeatureSupport(
	//	D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
	//	&msQualityLevels,
	//	sizeof(msQualityLevels)));

#ifdef _DEBUG
	LogAdapters();
#endif

	CreateCommandObjects();
	CreateSwapChain();
	CreateSwapChainDH();

	return true;
}

void DX12App::CreateCommandObjects() {
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(uDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&uCmdQueue.raw)));

	ThrowIfFailed(uDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mainCmdAlloc.GetAddressOf())));

	ThrowIfFailed(uDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mainCmdAlloc.Get(), // Associated command allocator
		nullptr,                   // Initial PipelineStateObject
		IID_PPV_ARGS(uGCmdList.raw.GetAddressOf())));

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	uGCmdList->Close();
}

void DX12App::CreateSwapChain() {
	// Release the previous swapchain we will be recreating.
	mSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mClientWidth;
	sd.BufferDesc.Height = mClientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = mBackBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = NumSwapChainBuffer;
	sd.OutputWindow = mhMainWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain uses queue to perform flush.
	ThrowIfFailed(mdxgiFactory->CreateSwapChain(
		uCmdQueue.raw.Get(),
		&sd,
		mSwapChain.GetAddressOf()));
}

void DX12App::CreateSwapChainDH() {
	swapchainRTVCpuDH = Ubpa::UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Allocate(NumSwapChainBuffer);
}

D3D12_VIEWPORT DX12App::GetScreenViewport() const noexcept {
	D3D12_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(mClientWidth);
	viewport.Height = static_cast<float>(mClientHeight);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	return viewport;
}

void DX12App::FlushCommandQueue() {
	// Advance the fence value to mark commands up to this fence point.
	mCurrentFence++;

	// Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	ThrowIfFailed(uCmdQueue->Signal(mFence.Get(), mCurrentFence));

	// Wait until the GPU has completed commands up to this fence point.
	if (mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

		// Fire event when GPU hits current fence.  
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void DX12App::SwapBackBuffer() {
	ThrowIfFailed(mSwapChain->Present(0, 0));
	curBackBuffer = (curBackBuffer + 1) % NumSwapChainBuffer;
}

ID3D12CommandAllocator* DX12App::GetCurFrameCommandAllocator() noexcept {
	return GetFrameResourceMngr()->GetCurrentFrameResource()
		->GetResource<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>(FR_CommandAllocator).Get();
}

void DX12App::CalculateFrameStats() {
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.

	frameCnt++;

	// Compute averages over one second period.
	if ((Ubpa::Utopia::GameTimer::Instance().TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		wstring fpsStr = to_wstring(fps);
		wstring mspfStr = to_wstring(mspf);

		wstring windowText = mMainWndCaption +
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr;

		SetWindowText(mhMainWnd, windowText.c_str());

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void DX12App::LogAdapters()
{
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	while (mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = L"***Adapter: ";
		text += desc.Description;
		text += L"\n";

		OutputDebugString(text.c_str());

		adapterList.push_back(adapter);

		++i;
	}

	for (size_t i = 0; i < adapterList.size(); ++i)
	{
		LogAdapterOutputs(adapterList[i]);
		UDX12::Util::ReleaseCom(adapterList[i]);
	}
}

void DX12App::LogAdapterOutputs(IDXGIAdapter* adapter)
{
	UINT i = 0;
	IDXGIOutput* output = nullptr;
	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);

		std::wstring text = L"***Output: ";
		text += desc.DeviceName;
		text += L"\n";
		OutputDebugString(text.c_str());

		LogOutputDisplayModes(output, mBackBufferFormat);

		UDX12::Util::ReleaseCom(output);

		++i;
	}
}

void DX12App::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
	UINT count = 0;
	UINT flags = 0;

	// Call with nullptr to get list count.
	output->GetDisplayModeList(format, flags, &count, nullptr);

	std::vector<DXGI_MODE_DESC> modeList(count);
	output->GetDisplayModeList(format, flags, &count, &modeList[0]);

	for (auto& x : modeList)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		std::wstring text =
			L"Width = " + std::to_wstring(x.Width) + L" " +
			L"Height = " + std::to_wstring(x.Height) + L" " +
			L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
			L"\n";

		::OutputDebugString(text.c_str());
	}
}