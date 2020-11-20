#include <Utopia/App/DX12App/DX12App.h>

#include <Utopia/Core/GameTimer.h>

using Microsoft::WRL::ComPtr;

class TestApp : public Ubpa::Utopia::DX12App {
public:
	TestApp(HINSTANCE hInstance);

	virtual bool Init() override;

private:
	virtual void Update() override {}
	virtual void Draw() override {}

	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
};

LRESULT TestApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg)
	{
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		return 0;
	}

	return DX12App::MsgProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try {
		TestApp theApp(hInstance);
		if (!theApp.Init())
			return 1;

		int rst = theApp.Run();
		return rst;
	}
	catch (Ubpa::UDX12::Util::Exception& e) {
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 1;
	}
}

TestApp::TestApp(HINSTANCE hInstance)
	: DX12App(hInstance)
{
}

bool TestApp::Init() {
	if (!InitMainWindow())
		return false;

	if (!InitDirect3D())
		return false;

	OnResize();
	FlushCommandQueue();

	return true;
}
