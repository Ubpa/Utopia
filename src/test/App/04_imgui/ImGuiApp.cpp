#include "../common/d3dApp.h"

#include <Utopia/Asset/AssetMngr.h>

#include <Utopia/Render/DX12/RsrcMngrDX12.h>
#include <Utopia/Render/DX12/StdPipeline.h>
#include <Utopia/Render/Texture2D.h>
#include <Utopia/Render/TextureCube.h>
#include <Utopia/Render/Shader.h>
#include <Utopia/Render/HLSLFile.h>
#include <Utopia/Render/ShaderMngr.h>
#include <Utopia/Render/Mesh.h>
#include <Utopia/Render/Components/Components.h>
#include <Utopia/Render/Systems/Systems.h>

#include <Utopia/Core/Scene.h>
#include <Utopia/Core/GameTimer.h>
#include <Utopia/Core/Components/Components.h>
#include <Utopia/Core/Systems/Systems.h>
#include <Utopia/Core/ImGUIMngr.h>

#include <_deps/imgui/imgui.h>
#include <_deps/imgui/imgui_impl_win32.h>
#include <_deps/imgui/imgui_impl_dx12.h>

#include <windowsx.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

const int gNumFrameResources = 3;

struct AnimateMeshSystem {
	size_t cnt = 0;
	static void OnUpdate(Ubpa::UECS::Schedule& schedule) {
		schedule.RegisterEntityJob(
			[](
				Ubpa::Utopia::MeshFilter* meshFilter,
				Ubpa::UECS::Latest<Ubpa::UECS::Singleton<Ubpa::Utopia::WorldTime>> time
			) {
				if (time->elapsedTime < 10.f) {
					if (meshFilter->mesh->IsEditable()) {
						auto positions = meshFilter->mesh->GetPositions();
						for (auto& pos : positions)
							pos[1] = 0.2f * (Ubpa::rand01<float>() - 0.5f);
						meshFilter->mesh->SetPositions(positions);
					}
				}
				else
					meshFilter->mesh->SetToNonEditable();
			},
			"AnimateMesh"
		);
		schedule.RegisterCommand([](Ubpa::UECS::World* w) {
			auto time = w->entityMngr.GetSingleton<Ubpa::Utopia::WorldTime>();
			if (!time)
				return;

			if (time->elapsedTime < 10.f)
				return;

			w->systemMngr.Deactivate(w->systemMngr.systemTraits.GetID(Ubpa::UECS::SystemTraits::StaticNameof<AnimateMeshSystem>()));
		});
	}
};

class ImGUIApp : public D3DApp
{
public:
    ImGUIApp(HINSTANCE hInstance);
    ImGUIApp(const ImGUIApp& rhs) = delete;
    ImGUIApp& operator=(const ImGUIApp& rhs) = delete;
    ~ImGUIApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update()override;
    virtual void Draw()override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

    void OnKeyboardInput();
	void UpdateCamera();

	void BuildWorld();
	void LoadTextures();
    void BuildShaders();
    void BuildMaterials();

private:
	float mTheta = 0.4f * XM_PI;
	float mPhi = 1.3f * XM_PI;
	float mRadius = 5.0f;

    POINT mLastMousePos;

	Ubpa::UECS::World world;
	Ubpa::UECS::Entity cam{ Ubpa::UECS::Entity::Invalid() };

	std::unique_ptr<Ubpa::Utopia::PipelineBase> pipeline;
	std::shared_ptr<Ubpa::Utopia::Mesh> dynamicMesh;

	std::unique_ptr<Ubpa::UDX12::FrameResourceMngr> frameRsrcMngr;

	bool show_demo_window = true;
	bool show_another_window = false;

	ImGuiContext* gameImGuiCtx = nullptr;
};

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler_Shared(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler_Context(ImGuiContext* ctx, bool ingore_mouse, bool ingore_keyboard, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT ImGUIApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler_Shared(hwnd, msg, wParam, lParam))
		return 1;

	bool imguiWantCaptureMouse = false;
	bool imguiWantCaptureKeyboard = false;

	if (ImGui::GetCurrentContext()) {
		auto& gameIO = ImGui::GetIO();
		bool gameWantCaptureMouse = gameIO.WantCaptureMouse;
		bool gameWantCaptureKeyboard = gameIO.WantCaptureKeyboard;
		if (ImGui_ImplWin32_WndProcHandler_Context(ImGui::GetCurrentContext(), false, false, hwnd, msg, wParam, lParam))
			return 1;

		imguiWantCaptureMouse = gameWantCaptureMouse;
		imguiWantCaptureKeyboard = gameWantCaptureKeyboard;
	}
	// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
	// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
	switch (msg)
	{
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mAppPaused = true;
			Ubpa::Utopia::GameTimer::Instance().Stop();
		}
		else
		{
			mAppPaused = false;
			Ubpa::Utopia::GameTimer::Instance().Start();
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
		Ubpa::Utopia::GameTimer::Instance().Stop();
		return 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing = false;
		Ubpa::Utopia::GameTimer::Instance().Start();
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
		if (imguiWantCaptureMouse)
			return 0;
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		if (imguiWantCaptureMouse)
			return 0;
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		if (imguiWantCaptureMouse)
			return 0;
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_KEYUP:
		if (imguiWantCaptureKeyboard)
			return 0;
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		else if ((int)wParam == VK_F2)
			Set4xMsaaState(!m4xMsaaState);

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
        ImGUIApp theApp(hInstance);
        if(!theApp.Initialize())
            return 0;

        int rst = theApp.Run();
		return rst;
    }
    catch(Ubpa::UDX12::Util::Exception& e)
    {
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }

}

ImGUIApp::ImGUIApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

ImGUIApp::~ImGUIApp()
{
	Ubpa::Utopia::RsrcMngrDX12::Instance().Clear(uCmdQueue.Get());
	Ubpa::Utopia::ImGUIMngr::Instance().Clear();
	if (!uDevice.IsNull())
		FlushCommandQueue();
}

bool ImGUIApp::Initialize()
{
	if (!InitMainWindow())
		return false;

	if (!InitDirect3D())
		return false;

	Ubpa::Utopia::RsrcMngrDX12::Instance().Init(uDevice.raw.Get());

	Ubpa::UDX12::DescriptorHeapMngr::Instance().Init(uDevice.raw.Get(), 1024, 1024, 1024, 1024, 1024);

	Ubpa::Utopia::ImGUIMngr::Instance().Init(MainWnd(), uDevice.Get(), gNumFrameResources, 1);
	gameImGuiCtx = Ubpa::Utopia::ImGUIMngr::Instance().GetContexts().at(0);

	frameRsrcMngr = std::make_unique<Ubpa::UDX12::FrameResourceMngr>(gNumFrameResources, uDevice.raw.Get());
	for (const auto& fr : frameRsrcMngr->GetFrameResources()) {
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
		ThrowIfFailed(uDevice->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&allocator)));
		fr->RegisterResource("CommandAllocator", allocator);
	}

	Ubpa::Utopia::AssetMngr::Instance().ImportAssetRecursively(L"..\\assets");

	BuildWorld();

	LoadTextures();
	BuildShaders();
	BuildMaterials();

	Ubpa::Utopia::PipelineBase::InitDesc initDesc;
	initDesc.device = uDevice.raw.Get();
	initDesc.rtFormat = mBackBufferFormat;
	initDesc.cmdQueue = uCmdQueue.raw.Get();
	initDesc.numFrame = gNumFrameResources;
	pipeline = std::make_unique<Ubpa::Utopia::StdPipeline>(initDesc);
	Ubpa::Utopia::RsrcMngrDX12::Instance().CommitUploadAndDelete(uCmdQueue.raw.Get());

	// Do the initial resize code.
	OnResize();

    // Wait until initialization is complete.
    FlushCommandQueue();

    return true;
}
 
void ImGUIApp::OnResize()
{
    D3DApp::OnResize();

	if (pipeline)
		pipeline->Resize(mClientWidth, mClientHeight, mScreenViewport, mScissorRect);
}

void ImGUIApp::Update()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame_Context(gameImGuiCtx, { 0,0 }, (float)mClientWidth, (float)mClientHeight);
	ImGui_ImplWin32_NewFrame_Shared();

	ImGui::SetCurrentContext(gameImGuiCtx);
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
	if (show_another_window) {
		ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			show_another_window = false;
		ImGui::End();
	}

    OnKeyboardInput();
	UpdateCamera();

	world.Update();

	// update mesh, texture ...
	frameRsrcMngr->BeginFrame();

	auto cmdAlloc = frameRsrcMngr->GetCurrentFrameResource()
		->GetResource<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>("CommandAllocator");
	cmdAlloc->Reset();

	ThrowIfFailed(uGCmdList->Reset(cmdAlloc.Get(), nullptr));

	// update mesh
	
	world.RunEntityJob([&](Ubpa::Utopia::MeshFilter* meshFilter, const Ubpa::Utopia::MeshRenderer* meshRenderer) {
		if (!meshFilter->mesh || meshRenderer->materials.empty())
			return;

		Ubpa::Utopia::RsrcMngrDX12::Instance().RegisterMesh(
			uGCmdList.Get(),
			*meshFilter->mesh
		);

		for (const auto& material : meshRenderer->materials) {
			if (!material)
				continue;
			for (const auto& [name, property] : material->properties) {
				if (std::holds_alternative<std::shared_ptr<const Ubpa::Utopia::Texture2D>>(property)) {
					Ubpa::Utopia::RsrcMngrDX12::Instance().RegisterTexture2D(
						*std::get<std::shared_ptr<const Ubpa::Utopia::Texture2D>>(property)
					);
				}
				else if (std::holds_alternative<std::shared_ptr<const Ubpa::Utopia::TextureCube>>(property)) {
					Ubpa::Utopia::RsrcMngrDX12::Instance().RegisterTextureCube(
						*std::get<std::shared_ptr<const Ubpa::Utopia::TextureCube>>(property)
					);
				}
			}
		}
	}, false);

	if (auto skybox = world.entityMngr.GetSingleton<Ubpa::Utopia::Skybox>(); skybox && skybox->material) {
		for (const auto& [name, property] : skybox->material->properties) {
			if (std::holds_alternative<std::shared_ptr<const Ubpa::Utopia::Texture2D>>(property)) {
				Ubpa::Utopia::RsrcMngrDX12::Instance().RegisterTexture2D(
					*std::get<std::shared_ptr<const Ubpa::Utopia::Texture2D>>(property)
				);
			}
			else if (std::holds_alternative<std::shared_ptr<const Ubpa::Utopia::TextureCube>>(property)) {
				Ubpa::Utopia::RsrcMngrDX12::Instance().RegisterTextureCube(
					*std::get<std::shared_ptr<const Ubpa::Utopia::TextureCube>>(property)
				);
			}
		}
	}

	// commit upload, delete ...
	uGCmdList->Close();
	uCmdQueue.Execute(uGCmdList.raw.Get());
	Ubpa::Utopia::RsrcMngrDX12::Instance().CommitUploadAndDelete(uCmdQueue.raw.Get());

	std::vector<Ubpa::Utopia::PipelineBase::CameraData> gameCameras;
	Ubpa::UECS::ArchetypeFilter camFilter{ {Ubpa::UECS::CmptAccessType::Of<Ubpa::Utopia::Camera>} };
	world.RunEntityJob([&](Ubpa::UECS::Entity e) {
		gameCameras.emplace_back(e, world);
	}, false, camFilter);
	assert(gameCameras.size() == 1); // now only support 1 camera
	pipeline->BeginFrame({ &world }, gameCameras.front());
}

void ImGUIApp::Draw()
{
	auto cmdAlloc = frameRsrcMngr->GetCurrentFrameResource()
		->GetResource<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>("CommandAllocator");
	ThrowIfFailed(uGCmdList->Reset(cmdAlloc.Get(), nullptr));

	ImGui::SetCurrentContext(gameImGuiCtx);

	pipeline->Render(CurrentBackBuffer());

	const auto curBack = CurrentBackBufferView();
	uGCmdList.ResourceBarrierTransition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	uGCmdList->OMSetRenderTargets(1, &curBack, FALSE, NULL);
	uGCmdList.SetDescriptorHeaps(Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap());
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), uGCmdList.Get());
	uGCmdList.ResourceBarrierTransition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	uGCmdList->Close();
	uCmdQueue.Execute(uGCmdList.Get());

    // Swap the back and front buffers
    ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	pipeline->EndFrame();
	frameRsrcMngr->EndFrame(uCmdQueue.raw.Get());
	ImGui_ImplWin32_EndFrame();
}

void ImGUIApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void ImGUIApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void ImGUIApp::OnMouseMove(WPARAM btnState, int x, int y)
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

void ImGUIApp::OnKeyboardInput()
{
}
 
void ImGUIApp::UpdateCamera()
{
	Ubpa::vecf3 eye = {
		mRadius * sinf(mTheta) * sinf(mPhi),
		mRadius * cosf(mTheta),
		mRadius * sinf(mTheta) * cosf(mPhi)
	};
	auto camera = world.entityMngr.Get<Ubpa::Utopia::Camera>(cam);
	camera->fov = 60.f;
	camera->aspect = AspectRatio();
	camera->clippingPlaneMin = 1.0f;
	camera->clippingPlaneMax = 1000.0f;
	auto view = Ubpa::transformf::look_at(eye.as<Ubpa::pointf3>(), { 0.f }); // world to camera
	auto c2w = view.inverse();
	world.entityMngr.Get<Ubpa::Utopia::Translation>(cam)->value = eye;
	world.entityMngr.Get<Ubpa::Utopia::Rotation>(cam)->value = c2w.decompose_quatenion();
}

void ImGUIApp::BuildWorld() {
	auto systemIDs = world.systemMngr.systemTraits.Register<
		Ubpa::Utopia::CameraSystem,
		Ubpa::Utopia::LocalToParentSystem,
		Ubpa::Utopia::RotationEulerSystem,
		Ubpa::Utopia::TRSToLocalToParentSystem,
		Ubpa::Utopia::TRSToLocalToWorldSystem,
		Ubpa::Utopia::WorldToLocalSystem,
		Ubpa::Utopia::WorldTimeSystem,
		AnimateMeshSystem
	>();
	for (auto ID : systemIDs)
		world.systemMngr.Activate(ID);

	{ // skybox
		auto [e, skybox] = world.entityMngr.Create<Ubpa::Utopia::Skybox>();
		const auto& path = Ubpa::Utopia::AssetMngr::Instance().GUIDToAssetPath(xg::Guid{ "bba13c3e-87d1-463a-974b-324d997349e3" });
		skybox->material = Ubpa::Utopia::AssetMngr::Instance().LoadAsset<Ubpa::Utopia::Material>(path);
	}

	{
		auto e = world.entityMngr.Create<
			Ubpa::Utopia::LocalToWorld,
			Ubpa::Utopia::WorldToLocal,
			Ubpa::Utopia::Camera,
			Ubpa::Utopia::Translation,
			Ubpa::Utopia::Rotation
		>();
		cam = std::get<Ubpa::UECS::Entity>(e);
	}

	{
		world.entityMngr.Create<Ubpa::Utopia::WorldTime>();
	}

	auto quadMesh = Ubpa::Utopia::AssetMngr::Instance().LoadAsset<Ubpa::Utopia::Mesh>("../assets/models/quad.obj");
	auto dynamicCube = world.entityMngr.Create<
		Ubpa::Utopia::LocalToWorld,
		Ubpa::Utopia::MeshFilter,
		Ubpa::Utopia::MeshRenderer,
		Ubpa::Utopia::Translation,
		Ubpa::Utopia::Rotation,
		Ubpa::Utopia::Scale
	>();
	dynamicMesh = std::make_shared<Ubpa::Utopia::Mesh>();
	dynamicMesh->SetPositions(quadMesh->GetPositions());
	dynamicMesh->SetNormals(quadMesh->GetNormals());
	dynamicMesh->SetUV(quadMesh->GetUV());
	dynamicMesh->SetIndices(quadMesh->GetIndices());
	dynamicMesh->SetSubMeshCount(quadMesh->GetSubMeshes().size());
	for (size_t i = 0; i < quadMesh->GetSubMeshes().size(); i++)
		dynamicMesh->SetSubMesh(i, quadMesh->GetSubMeshes().at(i));
	std::get<Ubpa::Utopia::MeshFilter*>(dynamicCube)->mesh = dynamicMesh;
}

void ImGUIApp::LoadTextures() {
	auto tex2dGUIDs = Ubpa::Utopia::AssetMngr::Instance().FindAssets(std::wregex{ LR"(\.\.\\assets\\_internal\\.*\.tex2d)" });
	for (const auto& guid : tex2dGUIDs) {
		const auto& path = Ubpa::Utopia::AssetMngr::Instance().GUIDToAssetPath(guid);
		Ubpa::Utopia::RsrcMngrDX12::Instance().RegisterTexture2D(
			*Ubpa::Utopia::AssetMngr::Instance().LoadAsset<Ubpa::Utopia::Texture2D>(path)
		);
	}

	auto texcubeGUIDs = Ubpa::Utopia::AssetMngr::Instance().FindAssets(std::wregex{ LR"(\.\.\\assets\\_internal\\.*\.texcube)" });
	for (const auto& guid : texcubeGUIDs) {
		const auto& path = Ubpa::Utopia::AssetMngr::Instance().GUIDToAssetPath(guid);
		Ubpa::Utopia::RsrcMngrDX12::Instance().RegisterTextureCube(
			*Ubpa::Utopia::AssetMngr::Instance().LoadAsset<Ubpa::Utopia::TextureCube>(path)
		);
	}
}

void ImGUIApp::BuildShaders() {
	auto& assetMngr = Ubpa::Utopia::AssetMngr::Instance();
	auto shaderGUIDs = assetMngr.FindAssets(std::wregex{ LR"(.*\.shader)" });
	for (const auto& guid : shaderGUIDs) {
		const auto& path = assetMngr.GUIDToAssetPath(guid);
		auto shader = assetMngr.LoadAsset<Ubpa::Utopia::Shader>(path);
		Ubpa::Utopia::RsrcMngrDX12::Instance().RegisterShader(*shader);
		Ubpa::Utopia::ShaderMngr::Instance().Register(shader);
	}
}

void ImGUIApp::BuildMaterials() {
	auto material = Ubpa::Utopia::AssetMngr::Instance()
		.LoadAsset<Ubpa::Utopia::Material>(L"..\\assets\\materials\\iron.mat");
	world.RunEntityJob([=](Ubpa::Utopia::MeshRenderer* meshRenderer) {
		meshRenderer->materials.push_back(material);
	});
}
