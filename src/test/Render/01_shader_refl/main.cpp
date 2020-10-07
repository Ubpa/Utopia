#include <Utopia/App/DX12App/DX12App.h>

#include <Utopia/Core/GameTimer.h>
#include <Utopia/Asset/AssetMngr.h>
#include <Utopia/Render/DX12/RsrcMngrDX12.h>
#include <Utopia/Render/Shader.h>
#include <Utopia/Render/HLSLFile.h>
#include <Utopia/Render/ShaderMngr.h>

using Microsoft::WRL::ComPtr;
using namespace Ubpa::Utopia;
using namespace Ubpa::UECS;
using namespace Ubpa;

class TestApp : public DX12App {
public:
	TestApp(HINSTANCE hInstance);
	~TestApp();

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

bool TestApp::Init() {
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

	auto PrintShader = [](const Shader& shader) {
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
		PrintRefl(RsrcMngrDX12::Instance().GetShaderRefl_vs(shader, 0), shader.name + " vs");
		PrintRefl(RsrcMngrDX12::Instance().GetShaderRefl_ps(shader, 0), shader.name + " ps");
	};
	
	auto geometry = ShaderMngr::Instance().Get("StdPipeline/Geometry");
	auto deferLighting = ShaderMngr::Instance().Get("StdPipeline/Defer Lighting");
	PrintShader(*geometry);
	PrintShader(*deferLighting);

	return true;
}
