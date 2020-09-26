#include <DustEngine/App/DX12App/DX12App.h>

#include <DustEngine/Core/GameTimer.h>
#include <DustEngine/Asset/AssetMngr.h>
#include <DustEngine/Render/DX12/RsrcMngrDX12.h>
#include <DustEngine/Render/Shader.h>
#include <DustEngine/Render/HLSLFile.h>
#include <DustEngine/Render/ShaderMngr.h>

using Microsoft::WRL::ComPtr;
using namespace Ubpa::DustEngine;
using namespace Ubpa::UECS;
using namespace Ubpa;

class TestApp : public DX12App {
public:
	TestApp(HINSTANCE hInstance);
	~TestApp();

	bool Initialize();

private:
	virtual void Update() override {}
	virtual void Draw() override {}

	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
};

LRESULT TestApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try {
		TestApp theApp(hInstance);
		if (!theApp.Initialize())
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

TestApp::~TestApp() {
	if (!uDevice.IsNull())
		FlushCommandQueue();
}

void PrintL(const std::string& str, size_t indent) {
	for (size_t i = 0; i < indent; i++)
		OutputDebugStringA("  ");
	OutputDebugStringA(str.c_str());
	OutputDebugStringA("\n");
}

void PrintL(const std::wstring& str, size_t indent) {
	for (size_t i = 0; i < indent; i++)
		OutputDebugStringA("  ");
	OutputDebugStringW(str.c_str());
	OutputDebugStringW(L"\n");
}

bool TestApp::Initialize() {
	if (!InitMainWindow())
		return false;

	if (!InitDirect3D())
		return false;

	OnResize();
	FlushCommandQueue();

	AssetMngr::Instance().ImportAssetRecursively(L"..\\assets");

	auto shaderGUIDs = AssetMngr::Instance().FindAssets(std::wregex{ LR"(.*\.shader)" });
	for (const auto& guid : shaderGUIDs) {
		const auto& path = AssetMngr::Instance().GUIDToAssetPath(guid);
		auto shader = AssetMngr::Instance().LoadAsset<Shader>(path);
		ShaderMngr::Instance().Register(shader);
	}

	auto PrintShader = [](Shader* shader) {
		RsrcMngrDX12::Instance().RegisterShader(shader);
		auto PrintRefl = [](ID3D12ShaderReflection* refl, const std::string& name) {
			D3D12_SHADER_DESC shaderDesc;
			ThrowIfFailed(refl->GetDesc(&shaderDesc));
			size_t indent = 0;
			PrintL("[" + name + " refl]", indent++);
			
			// PrintL("slot num:" + std::to_string(refl->GetNumInterfaceSlots()), indent);
			PrintL("input param num:" + std::to_string(shaderDesc.InputParameters), indent);
			PrintL("output param num:" + std::to_string(shaderDesc.OutputParameters), indent);
			PrintL("cb num:" + std::to_string(shaderDesc.ConstantBuffers), indent);
			PrintL("rsrc num:" + std::to_string(shaderDesc.BoundResources), indent);

			for (UINT i = 0; i < shaderDesc.ConstantBuffers; i++) {
				PrintL("[cb refl]", indent++);
				auto cb = refl->GetConstantBufferByIndex(i);
				D3D12_SHADER_BUFFER_DESC cbDesc;
				ThrowIfFailed(cb->GetDesc(&cbDesc));
				PrintL(std::string("name:") + cbDesc.Name, indent);
				PrintL(std::string("num var:") + std::to_string(cbDesc.Variables), indent);
				PrintL(std::string("size:") + std::to_string(cbDesc.Size), indent);
				
				for (UINT j = 0; j < cbDesc.Variables; j++) {
					PrintL("[var refl]", indent++);
					auto var = cb->GetVariableByIndex(j);
					D3D12_SHADER_VARIABLE_DESC varDesc;
					ThrowIfFailed(var->GetDesc(&varDesc));
					PrintL(std::string("name:") + varDesc.Name, indent);
					PrintL(std::string("size:") + std::to_string(varDesc.Size), indent);
					PrintL(std::string("offset:") + std::to_string(varDesc.StartOffset), indent);

					auto type = var->GetType();
					D3D12_SHADER_TYPE_DESC typeDesc;
					ThrowIfFailed(type->GetDesc(&typeDesc));
					PrintL(std::string("class:") + std::to_string(typeDesc.Class), indent);
					PrintL(std::string("type:") + std::to_string(typeDesc.Type), indent);
					PrintL(std::string("type name:") + typeDesc.Name, indent);

					indent--;
				}
				indent--;
			}
			for (UINT i = 0; i < shaderDesc.BoundResources; i++) {
				PrintL("[rsrc refl]", indent++);
				D3D12_SHADER_INPUT_BIND_DESC rsrcDesc;
				ThrowIfFailed(refl->GetResourceBindingDesc(i, &rsrcDesc));
				PrintL(std::string("name:") + rsrcDesc.Name, indent);
				PrintL(std::string("type:") + std::to_string(rsrcDesc.Type), indent);
				PrintL(std::string("bind point:") + std::to_string(rsrcDesc.BindPoint), indent);
				PrintL(std::string("space:") + std::to_string(rsrcDesc.Space), indent);
				PrintL(std::string("dimension:") + std::to_string(rsrcDesc.Dimension), indent);
				indent--;
			}
			for (UINT i = 0; i < shaderDesc.InputParameters; i++) {
				PrintL("[input refl]", indent++);
				D3D12_SIGNATURE_PARAMETER_DESC sigDesc;
				ThrowIfFailed(refl->GetInputParameterDesc(i, &sigDesc));
				PrintL(std::string("name:") + sigDesc.SemanticName, indent);
				PrintL(std::string("index:") + std::to_string(sigDesc.SemanticIndex), indent);
				PrintL(std::string("register:") + std::to_string(sigDesc.Register), indent);
				PrintL(std::string("system value type:") + std::to_string(sigDesc.SystemValueType), indent);
				PrintL(std::string("component type:") + std::to_string(sigDesc.ComponentType), indent);
				indent--;
			}
			for (UINT i = 0; i < shaderDesc.OutputParameters; i++) {
				PrintL("[output refl]", indent++);
				D3D12_SIGNATURE_PARAMETER_DESC sigDesc;
				ThrowIfFailed(refl->GetOutputParameterDesc(i, &sigDesc));
				PrintL(std::string("name:") + sigDesc.SemanticName, indent);
				PrintL(std::string("index:") + std::to_string(sigDesc.SemanticIndex), indent);
				PrintL(std::string("register:") + std::to_string(sigDesc.Register), indent);
				PrintL(std::string("system value type:") + std::to_string(sigDesc.SystemValueType), indent);
				PrintL(std::string("component type:") + std::to_string(sigDesc.ComponentType), indent);
				indent--;
			}
		};
		PrintRefl(RsrcMngrDX12::Instance().GetShaderRefl_vs(shader, 0), shader->shaderName + " vs");
		PrintRefl(RsrcMngrDX12::Instance().GetShaderRefl_ps(shader, 0), shader->shaderName + " ps");
	};
	
	auto geometry = ShaderMngr::Instance().Get("StdPipeline/Geometry");
	auto deferLighting = ShaderMngr::Instance().Get("StdPipeline/Defer Lighting");
	PrintShader(geometry);
	PrintShader(deferLighting);

	return true;
}
