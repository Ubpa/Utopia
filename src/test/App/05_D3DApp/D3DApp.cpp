#include <DustEngine/App/D3DApp/D3DApp.h>

#include <DustEngine/_deps/imgui/imgui.h>
#include <DustEngine/_deps/imgui/imgui_impl_win32.h>
#include <DustEngine/_deps/imgui/imgui_impl_dx12.h>

#include <DustEngine/Render/DX12/RsrcMngrDX12.h>
#include <DustEngine/Render/DX12/ShaderCBMngrDX12.h>
#include <DustEngine/Render/DX12/MeshLayoutMngr.h>
#include <DustEngine/Render/DX12/StdPipeline.h>

#include <DustEngine/Asset/AssetMngr.h>

#include <DustEngine/Transform/Transform.h>

#include <DustEngine/Core/Texture2D.h>
#include <DustEngine/Core/Image.h>
#include <DustEngine/Core/HLSLFile.h>
#include <DustEngine/Core/Shader.h>
#include <DustEngine/Core/Mesh.h>
#include <DustEngine/Core/Components/Camera.h>
#include <DustEngine/Core/Components/MeshFilter.h>
#include <DustEngine/Core/Components/MeshRenderer.h>
#include <DustEngine/Core/Systems/CameraSystem.h>
#include <DustEngine/Core/GameTimer.h>

#include <UDX12/UploadBuffer.h>

#include <UGM/UGM.h>

#include <windowsx.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

const int gNumFrameResources = 3;

struct AnimateMeshSystem : Ubpa::UECS::System {
	using Ubpa::UECS::System::System;
	size_t cnt = 0;
	virtual void OnUpdate(Ubpa::UECS::Schedule& schedule) override {
		if (cnt < 600) {
			schedule.RegisterEntityJob([](Ubpa::DustEngine::MeshFilter* meshFilter) {
				if (meshFilter->mesh->IsEditable()) {
					auto positions = meshFilter->mesh->GetPositions();
					for (auto& pos : positions)
						pos[1] = 0.2f * (Ubpa::rand01<float>() - 0.5f);
					meshFilter->mesh->SetPositions(positions);
				}
			}, "AnimateMesh");
		}
		else if (cnt == 600) {
			schedule.RegisterEntityJob([](Ubpa::DustEngine::MeshFilter* meshFilter) {
				meshFilter->mesh->SetToNonEditable();
			}, "set mesh static");
		}
		cnt++;
	}
};

class MyD3DApp : public Ubpa::DustEngine::D3DApp
{
public:
    MyD3DApp(HINSTANCE hInstance);
    ~MyD3DApp();

    bool Initialize();

private:
    void OnResize();
    virtual void Update() override;
    virtual void Draw() override;

	void OnMouseDown(WPARAM btnState, int x, int y);
    void OnMouseUp(WPARAM btnState, int x, int y);
    void OnMouseMove(WPARAM btnState, int x, int y);

	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

    void OnKeyboardInput();
	void UpdateCamera();

	void BuildWorld();
	void LoadTextures();
    void BuildMaterials();

private:
	float mTheta = 0.4f * XM_PI;
	float mPhi = 1.3f * XM_PI;
	float mRadius = 5.0f;

    POINT mLastMousePos;

	Ubpa::DustEngine::Texture2D* albedoTex2D;
	Ubpa::DustEngine::Texture2D* roughnessTex2D;
	Ubpa::DustEngine::Texture2D* metalnessTex2D;

	Ubpa::UECS::World world;
	Ubpa::UECS::Entity cam{ Ubpa::UECS::Entity::Invalid() };

	std::unique_ptr<Ubpa::DustEngine::IPipeline> pipeline;
	std::unique_ptr<Ubpa::DustEngine::Mesh> dynamicMesh;

	std::unique_ptr<Ubpa::UDX12::FrameResourceMngr> frameRsrcMngr;

	Ubpa::UDX12::DescriptorHeapAllocation imguiAlloc;

	bool show_demo_window = true;
	bool show_another_window = false;
};

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT MyD3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
		return true;
	// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
	// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
	auto imgui_ctx = ImGui::GetCurrentContext();
	switch (msg)
	{
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mAppPaused = true;
			Ubpa::DustEngine::GameTimer::Instance().Stop();
		}
		else
		{
			mAppPaused = false;
			Ubpa::DustEngine::GameTimer::Instance().Start();
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
		Ubpa::DustEngine::GameTimer::Instance().Stop();
		return 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing = false;
		Ubpa::DustEngine::GameTimer::Instance().Start();
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
		if (imgui_ctx && ImGui::GetIO().WantCaptureMouse)
			return 0;
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		if (imgui_ctx && ImGui::GetIO().WantCaptureMouse)
			return 0;
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		if (imgui_ctx && ImGui::GetIO().WantCaptureMouse)
			return 0;
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_KEYUP:
		if (imgui_ctx && ImGui::GetIO().WantCaptureKeyboard)
			return 0;
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}

		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        MyD3DApp theApp(hInstance);
        if(!theApp.Initialize())
            return 0;

        int rst = theApp.Run();
		Ubpa::DustEngine::RsrcMngrDX12::Instance().Clear();
		return rst;
    }
    catch(Ubpa::UDX12::Util::Exception& e)
    {
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		Ubpa::DustEngine::RsrcMngrDX12::Instance().Clear();
        return 0;
    }

}

MyD3DApp::MyD3DApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

MyD3DApp::~MyD3DApp()
{
    if(!uDevice.IsNull())
        FlushCommandQueue();

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(imguiAlloc));
}

bool MyD3DApp::Initialize()
{
	if (!InitMainWindow())
		return false;

	if (!InitDirect3D())
		return false;

	Ubpa::DustEngine::RsrcMngrDX12::Instance().Init(uDevice.raw.Get());

	Ubpa::UDX12::DescriptorHeapMngr::Instance().Init(uDevice.raw.Get(), 1024, 1024, 1024, 1024, 1024);

	Ubpa::DustEngine::IPipeline::InitDesc initDesc;
	initDesc.device = uDevice.raw.Get();
	initDesc.backBufferFormat = GetBackBufferFormat();
	initDesc.depthStencilFormat = GetDepthStencilBufferFormat();
	initDesc.cmdQueue = uCmdQueue.raw.Get();
	initDesc.numFrame = gNumFrameResources;
	pipeline = std::make_unique<Ubpa::DustEngine::StdPipeline>(initDesc);

	frameRsrcMngr = std::make_unique<Ubpa::UDX12::FrameResourceMngr>(gNumFrameResources, uDevice.raw.Get());
	for (const auto& fr : frameRsrcMngr->GetFrameResources()) {
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
		ThrowIfFailed(initDesc.device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&allocator)));
		fr->RegisterResource("CommandAllocator", allocator);
	}

	Ubpa::DustEngine::MeshLayoutMngr::Instance().Init();

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(MainWnd());
	imguiAlloc = Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(1);
	ImGui_ImplDX12_Init(uDevice.raw.Get(), gNumFrameResources,
		DXGI_FORMAT_R8G8B8A8_UNORM, Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap(),
		imguiAlloc.GetCpuHandle(),
		imguiAlloc.GetGpuHandle());

	// Do the initial resize code.
	OnResize();

	BuildWorld();

	Ubpa::DustEngine::RsrcMngrDX12::Instance().GetUpload().Begin();
	LoadTextures();
	BuildMaterials();
	Ubpa::DustEngine::RsrcMngrDX12::Instance().GetUpload().End(uCmdQueue.raw.Get());

    // Wait until initialization is complete.
    FlushCommandQueue();

    return true;
}
 
void MyD3DApp::OnResize()
{
    D3DApp::OnResize();

	if (pipeline)
		pipeline->Resize(mClientWidth, mClientHeight, GetScreenViewport(), GetScissorRect(), GetDepthStencilBuffer());
}

void MyD3DApp::Update()
{
    OnKeyboardInput();
	UpdateCamera();

	world.Update();

	// update mesh, texture ...
	frameRsrcMngr->BeginFrame();

	auto cmdAlloc = frameRsrcMngr->GetCurrentFrameResource()->GetResource<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>("CommandAllocator");
	cmdAlloc->Reset();

	ThrowIfFailed(uGCmdList->Reset(cmdAlloc.Get(), nullptr));
	auto& upload = Ubpa::DustEngine::RsrcMngrDX12::Instance().GetUpload();
	auto& deleteBatch = Ubpa::DustEngine::RsrcMngrDX12::Instance().GetDeleteBatch();

	// update mesh
	Ubpa::UECS::ArchetypeFilter filter;
	filter.all = { Ubpa::UECS::CmptAccessType::Of<Ubpa::DustEngine::MeshFilter> };
	auto meshFilters = world.entityMngr.GetCmptArray<Ubpa::DustEngine::MeshFilter>(filter);
	upload.Begin();
	for (auto meshFilter : meshFilters) {
		Ubpa::DustEngine::RsrcMngrDX12::Instance().RegisterMesh(
			upload,
			deleteBatch,
			uGCmdList.raw.Get(),
			meshFilter->mesh
		);
	}

	// commit upload, delete ...
	upload.End(uCmdQueue.raw.Get());
	deleteBatch.Commit(uDevice.raw.Get(), uCmdQueue.raw.Get());
	uGCmdList->Close();
	uCmdQueue.Execute(uGCmdList.raw.Get());
	frameRsrcMngr->EndFrame(uCmdQueue.raw.Get());

	pipeline->UpdateRenderContext(world);
}

void MyD3DApp::Draw()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &show_another_window);

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		//ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	// 3. Show another simple window.
	if (show_another_window)
	{
		ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			show_another_window = false;
		ImGui::End();
	}

	pipeline->BeginFrame(CurrentBackBuffer());

	pipeline->Render();

	SwapBackBuffer();

	pipeline->EndFrame();
}

void MyD3DApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(MainWnd());
}

void MyD3DApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void MyD3DApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta -= dy;
		mPhi -= dx;

		// Restrict the angle mPhi.
		mTheta = std::clamp(mTheta, 0.1f, Ubpa::PI<float> -0.1f);
    }
    else if((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.2 unit in the scene.
        float dx = 0.05f*static_cast<float>(x - mLastMousePos.x);
        float dy = 0.05f*static_cast<float>(y - mLastMousePos.y);

        // Update the camera radius based on input.
        mRadius += dx - dy;

        // Restrict the radius.
        mRadius = std::clamp(mRadius, 5.0f, 150.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void MyD3DApp::OnKeyboardInput()
{
}
 
void MyD3DApp::UpdateCamera()
{
	Ubpa::vecf3 eye = {
		mRadius * sinf(mTheta) * sinf(mPhi),
		mRadius * cosf(mTheta),
		mRadius * sinf(mTheta) * cosf(mPhi)
	};
	auto camera = world.entityMngr.Get<Ubpa::DustEngine::Camera>(cam);
	camera->fov = 0.33f * Ubpa::PI<float>;
	camera->aspect = AspectRatio();
	camera->clippingPlaneMin = 1.0f;
	camera->clippingPlaneMax = 1000.0f;
	auto view = Ubpa::transformf::look_at(eye.as<Ubpa::pointf3>(), { 0.f }); // world to camera
	auto c2w = view.inverse();
	world.entityMngr.Get<Ubpa::DustEngine::Translation>(cam)->value = eye;
	world.entityMngr.Get<Ubpa::DustEngine::Rotation>(cam)->value = c2w.decompose_quatenion();
}

void MyD3DApp::BuildWorld() {
	world.systemMngr.Register<
		Ubpa::DustEngine::CameraSystem,
		Ubpa::DustEngine::LocalToParentSystem,
		Ubpa::DustEngine::RotationEulerSystem,
		Ubpa::DustEngine::TRSToLocalToParentSystem,
		Ubpa::DustEngine::TRSToLocalToWorldSystem,
		Ubpa::DustEngine::WorldToLocalSystem,
		AnimateMeshSystem
	>();

	auto e0 = world.entityMngr.Create<
		Ubpa::DustEngine::LocalToWorld,
		Ubpa::DustEngine::WorldToLocal,
		Ubpa::DustEngine::Camera,
		Ubpa::DustEngine::Translation,
		Ubpa::DustEngine::Rotation
	>();
	cam = std::get<Ubpa::UECS::Entity>(e0);

	auto quadMesh = Ubpa::DustEngine::AssetMngr::Instance().LoadAsset<Ubpa::DustEngine::Mesh>("../assets/models/quad.obj");
	auto dynamicCube = world.entityMngr.Create<
		Ubpa::DustEngine::LocalToWorld,
		Ubpa::DustEngine::MeshFilter,
		Ubpa::DustEngine::MeshRenderer,
		Ubpa::DustEngine::Translation,
		Ubpa::DustEngine::Rotation,
		Ubpa::DustEngine::Scale
	>();
	dynamicMesh = std::make_unique<Ubpa::DustEngine::Mesh>();
	dynamicMesh->SetPositions(quadMesh->GetPositions());
	dynamicMesh->SetNormals(quadMesh->GetNormals());
	dynamicMesh->SetUV(quadMesh->GetUV());
	dynamicMesh->SetIndices(quadMesh->GetIndices());
	dynamicMesh->SetSubMeshCount(quadMesh->GetSubMeshes().size());
	for (size_t i = 0; i < quadMesh->GetSubMeshes().size(); i++)
		dynamicMesh->SetSubMesh(i, quadMesh->GetSubMeshes().at(i));
	std::get<Ubpa::DustEngine::MeshFilter*>(dynamicCube)->mesh = dynamicMesh.get();
}

void MyD3DApp::LoadTextures() {
	auto albedoImg = Ubpa::DustEngine::AssetMngr::Instance()
		.LoadAsset<Ubpa::DustEngine::Image>("../assets/textures/iron/albedo.png");
	auto roughnessImg = Ubpa::DustEngine::AssetMngr::Instance()
		.LoadAsset<Ubpa::DustEngine::Image>("../assets/textures/iron/roughness.png");
	auto metalnessImg = Ubpa::DustEngine::AssetMngr::Instance()
		.LoadAsset<Ubpa::DustEngine::Image>("../assets/textures/iron/metalness.png");

	albedoTex2D = new Ubpa::DustEngine::Texture2D;
	albedoTex2D->image = albedoImg;
	if (!Ubpa::DustEngine::AssetMngr::Instance().CreateAsset(albedoTex2D, "../assets/textures/iron/albedo.tex2d")) {
		delete albedoTex2D;
		albedoTex2D = Ubpa::DustEngine::AssetMngr::Instance()
			.LoadAsset<Ubpa::DustEngine::Texture2D>("../assets/textures/iron/albedo.tex2d");
	}

	roughnessTex2D = new Ubpa::DustEngine::Texture2D;
	roughnessTex2D->image = roughnessImg;
	if (!Ubpa::DustEngine::AssetMngr::Instance().CreateAsset(roughnessTex2D, "../assets/textures/iron/roughness.tex2d")) {
		delete roughnessTex2D;
		roughnessTex2D = Ubpa::DustEngine::AssetMngr::Instance()
			.LoadAsset<Ubpa::DustEngine::Texture2D>("../assets/textures/iron/roughness.tex2d");
	}

	metalnessTex2D = new Ubpa::DustEngine::Texture2D;
	metalnessTex2D->image = metalnessImg;
	if (!Ubpa::DustEngine::AssetMngr::Instance().CreateAsset(metalnessTex2D, "../assets/textures/iron/metalness.tex2d")) {
		delete metalnessTex2D;
		metalnessTex2D = Ubpa::DustEngine::AssetMngr::Instance()
			.LoadAsset<Ubpa::DustEngine::Texture2D>("../assets/textures/iron/metalness.tex2d");
	}

	Ubpa::DustEngine::RsrcMngrDX12::Instance().RegisterTexture2D(
		Ubpa::DustEngine::RsrcMngrDX12::Instance().GetUpload(),
		albedoTex2D
	);
	Ubpa::DustEngine::RsrcMngrDX12::Instance().RegisterTexture2D(
		Ubpa::DustEngine::RsrcMngrDX12::Instance().GetUpload(),
		roughnessTex2D
	);
	Ubpa::DustEngine::RsrcMngrDX12::Instance().RegisterTexture2D(
		Ubpa::DustEngine::RsrcMngrDX12::Instance().GetUpload(),
		metalnessTex2D
	);
}

void MyD3DApp::BuildMaterials() {
	std::filesystem::path matPath = "../assets/materials/iron.mat";
	auto material = new Ubpa::DustEngine::Material;
	material->shader = Ubpa::DustEngine::AssetMngr::Instance().LoadAsset<Ubpa::DustEngine::Shader>("../assets/shaders/geometry.shader");
	material->texture2Ds.emplace("gAlbedoMap", albedoTex2D);
	material->texture2Ds.emplace("gRoughnessMap", roughnessTex2D);
	material->texture2Ds.emplace("gMetalnessMap", metalnessTex2D);

	if (!Ubpa::DustEngine::AssetMngr::Instance().CreateAsset(material, matPath)) {
		delete material;
		material = Ubpa::DustEngine::AssetMngr::Instance().LoadAsset<Ubpa::DustEngine::Material>(matPath);
	}
	Ubpa::UECS::ArchetypeFilter filter;
	filter.all = { Ubpa::UECS::CmptAccessType::Of<Ubpa::DustEngine::MeshRenderer> };
	auto meshRenderers = world.entityMngr.GetCmptArray<Ubpa::DustEngine::MeshRenderer>(filter);
	for (auto meshRenderer : meshRenderers)
		meshRenderer->material.push_back(material);
}
