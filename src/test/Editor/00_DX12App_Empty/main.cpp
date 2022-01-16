#include <Utopia/Editor/DX12App/DX12App.h>

using namespace Ubpa::Utopia;

class EmptyDX12App : public DX12App {
public:
    EmptyDX12App(HINSTANCE hInstance) : DX12App(hInstance) { mMainWndCaption = L"Empty DX12 App"; }
    virtual void Update() override {
        GetFrameResourceMngr()->BeginFrame();
        ID3D12CommandAllocator* cmdAlloc = GetCurFrameCommandAllocator();
        cmdAlloc->Reset();
        ThrowIfFailed(uGCmdList->Reset(cmdAlloc, nullptr));

        uGCmdList.ResourceBarrierTransition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        uGCmdList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightBlue, 0, NULL);

        // add commands to uGCmdList ...

        uGCmdList.ResourceBarrierTransition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        uGCmdList->Close();
        uCmdQueue.Execute(uGCmdList.Get());
        SwapBackBuffer();
        GetFrameResourceMngr()->EndFrame(uCmdQueue.Get());
    }
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {
	int rst;
    try {
        EmptyDX12App app(hInstance);
        if(!app.Init())
            return 1;
        
		rst = app.Run();
    }
    catch(Ubpa::UDX12::Util::Exception& e) {
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        rst = 1;
	}

	return rst;
}
