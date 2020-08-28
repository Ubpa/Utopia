#include <DustEngine/App/DX12App/DX12App.h>

#include <DustEngine/Render/DX12/RsrcMngrDX12.h>
#include <DustEngine/Render/DX12/ShaderCBMngrDX12.h>
#include <DustEngine/Render/DX12/MeshLayoutMngr.h>
#include <DustEngine/Render/DX12/StdPipeline.h>

#include <DustEngine/Asset/AssetMngr.h>

#include <DustEngine/Core/Texture2D.h>
#include <DustEngine/Core/Image.h>
#include <DustEngine/Core/HLSLFile.h>
#include <DustEngine/Core/Shader.h>
#include <DustEngine/Core/ShaderMngr.h>
#include <DustEngine/Core/Mesh.h>
#include <DustEngine/Core/Components/Camera.h>
#include <DustEngine/Core/Components/MeshFilter.h>
#include <DustEngine/Core/Components/MeshRenderer.h>
#include <DustEngine/Core/Components/WorldTime.h>
#include <DustEngine/Core/Systems/CameraSystem.h>
#include <DustEngine/Core/Systems/WorldTimeSystem.h>
#include <DustEngine/Core/GameTimer.h>

#include <DustEngine/Transform/Transform.h>

#include <DustEngine/Core/ImGUIMngr.h>

#include <DustEngine/_deps/imgui/imgui.h>
#include <DustEngine/_deps/imgui/imgui_impl_win32.h>
#include <DustEngine/_deps/imgui/imgui_impl_dx12.h>

#include <DustEngine/ScriptSystem/LuaContext.h>
#include <DustEngine/ScriptSystem/LuaCtxMngr.h>
#include <DustEngine/ScriptSystem/LuaScript.h>

#include <ULuaPP/ULuaPP.h>

using Microsoft::WRL::ComPtr;

class GameStarter : public Ubpa::DustEngine::DX12App {
public:
    GameStarter(HINSTANCE hInstance);
    ~GameStarter();

    bool Initialize();

private:
    void OnResize();
    virtual void Update() override;
    virtual void Draw() override;

	void OnMouseDown(WPARAM btnState, int x, int y);
    void OnMouseUp(WPARAM btnState, int x, int y);
    void OnMouseMove(WPARAM btnState, int x, int y);

	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

	void UpdateCamera();

	void BuildWorld();
	void LoadTextures();
	void BuildShaders();
    void BuildMaterials();

private:
	float mTheta = 0.4f * Ubpa::PI<float>;
	float mPhi = 1.3f * Ubpa::PI<float>;
	float mRadius = 5.0f;

    POINT mLastMousePos;

	Ubpa::UECS::World world;
	Ubpa::UECS::Entity cam{ Ubpa::UECS::Entity::Invalid() };

	std::unique_ptr<Ubpa::DustEngine::IPipeline> pipeline;

	bool show_demo_window = true;
	bool show_another_window = false;
};

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT GameStarter::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try {
        GameStarter theApp(hInstance);
        if(!theApp.Initialize())
            return 1;

        int rst = theApp.Run();
		return rst;
    }
    catch(Ubpa::UDX12::Util::Exception& e) {
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 1;
    }
}

GameStarter::GameStarter(HINSTANCE hInstance)
	: DX12App(hInstance)
{
}

GameStarter::~GameStarter() {
    if(!uDevice.IsNull())
        FlushCommandQueue();

	Ubpa::DustEngine::ImGUIMngr::Instance().Clear();
}

bool GameStarter::Initialize() {
	if (!InitMainWindow())
		return false;

	if (!InitDirect3D())
		return false;

	Ubpa::DustEngine::MeshLayoutMngr::Instance().Init();

	Ubpa::DustEngine::ImGUIMngr::Instance().Init(MainWnd(), uDevice.Get(), NumFrameResources);

	Ubpa::DustEngine::AssetMngr::Instance().ImportAssetRecursively(LR"(..\\assets)");

	BuildWorld();

	Ubpa::DustEngine::RsrcMngrDX12::Instance().GetUpload().Begin();
	LoadTextures();
	BuildShaders();
	BuildMaterials();
	Ubpa::DustEngine::RsrcMngrDX12::Instance().GetUpload().End(uCmdQueue.Get());

	Ubpa::DustEngine::IPipeline::InitDesc initDesc;
	initDesc.device = uDevice.Get();
	initDesc.backBufferFormat = GetBackBufferFormat();
	initDesc.depthStencilFormat = GetDepthStencilBufferFormat();
	initDesc.cmdQueue = uCmdQueue.Get();
	initDesc.numFrame = NumFrameResources;
	pipeline = std::make_unique<Ubpa::DustEngine::StdPipeline>(initDesc);

	// Do the initial resize code.
	OnResize();

    // Wait until initialization is complete.
    FlushCommandQueue();

    return true;
}
 
void GameStarter::OnResize()
{
    DX12App::OnResize();
	
	assert(pipeline);
	pipeline->Resize(mClientWidth, mClientHeight, GetScreenViewport(), GetScissorRect(), GetDepthStencilBuffer());
}

void GameStarter::Update() {
	UpdateCamera();

	world.Update();

	// update mesh, texture ...
	GetFrameResourceMngr()->BeginFrame();

	auto cmdAlloc = GetCurFrameCommandAllocator();
	cmdAlloc->Reset();

	ThrowIfFailed(uGCmdList->Reset(cmdAlloc, nullptr));
	auto& upload = Ubpa::DustEngine::RsrcMngrDX12::Instance().GetUpload();
	auto& deleteBatch = Ubpa::DustEngine::RsrcMngrDX12::Instance().GetDeleteBatch();
	upload.Begin();

	// update mesh
	world.RunEntityJob([&](const Ubpa::DustEngine::MeshFilter* meshFilter) {
		Ubpa::DustEngine::RsrcMngrDX12::Instance().RegisterMesh(
			upload,
			deleteBatch,
			uGCmdList.Get(),
			meshFilter->mesh
		);
	}, false);

	// commit upload, delete ...
	upload.End(uCmdQueue.Get());
	deleteBatch.Commit(uDevice.Get(), uCmdQueue.Get());
	uGCmdList->Close();
	uCmdQueue.Execute(uGCmdList.Get());
	GetFrameResourceMngr()->EndFrame(uCmdQueue.Get());

	pipeline->UpdateRenderContext(world);
}

void GameStarter::Draw()
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

	pipeline->Render(CurrentBackBuffer());

	SwapBackBuffer();

	pipeline->EndFrame();
}

void GameStarter::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(MainWnd());
}

void GameStarter::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void GameStarter::OnMouseMove(WPARAM btnState, int x, int y)
{
    if((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = Ubpa::to_radian(0.25f*static_cast<float>(x - mLastMousePos.x));
        float dy = Ubpa::to_radian(0.25f*static_cast<float>(y - mLastMousePos.y));

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
 
void GameStarter::UpdateCamera()
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

void GameStarter::BuildWorld() {
	world.systemMngr.Register<
		Ubpa::DustEngine::CameraSystem,
		Ubpa::DustEngine::LocalToParentSystem,
		Ubpa::DustEngine::RotationEulerSystem,
		Ubpa::DustEngine::TRSToLocalToParentSystem,
		Ubpa::DustEngine::TRSToLocalToWorldSystem,
		Ubpa::DustEngine::WorldToLocalSystem,
		Ubpa::DustEngine::WorldTimeSystem
	>();
	world.entityMngr.cmptTraits.Register<
		Ubpa::DustEngine::Camera,
		Ubpa::DustEngine::MeshFilter,
		Ubpa::DustEngine::MeshRenderer,
		Ubpa::DustEngine::WorldTime,
		Ubpa::DustEngine::Children,
		Ubpa::DustEngine::LocalToParent,
		Ubpa::DustEngine::LocalToWorld,
		Ubpa::DustEngine::Parent,
		Ubpa::DustEngine::Rotation,
		Ubpa::DustEngine::RotationEuler,
		Ubpa::DustEngine::Scale,
		Ubpa::DustEngine::Translation,
		Ubpa::DustEngine::WorldToLocal
	>();

	world.entityMngr.Create<Ubpa::DustEngine::WorldTime>();

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
	std::get<Ubpa::DustEngine::MeshFilter*>(dynamicCube)->mesh = quadMesh;

	auto mainLua = Ubpa::DustEngine::LuaCtxMngr::Instance().Register(&world)->Main();
	sol::state_view solLua(mainLua);
	auto gameLuaScript = Ubpa::DustEngine::AssetMngr::Instance()
		.LoadAsset<Ubpa::DustEngine::LuaScript>(L"..\\assets\\scripts\\Game.lua");
	solLua["world"] = &world;
	solLua.script(gameLuaScript->GetText());
}

void GameStarter::LoadTextures() {
	auto tex2dGUIDs = Ubpa::DustEngine::AssetMngr::Instance().FindAssets(std::wregex{ LR"(.*\.tex2d)" });
	for (const auto& guid : tex2dGUIDs) {
		const auto& path = Ubpa::DustEngine::AssetMngr::Instance().GUIDToAssetPath(guid);
		Ubpa::DustEngine::RsrcMngrDX12::Instance().RegisterTexture2D(
			Ubpa::DustEngine::RsrcMngrDX12::Instance().GetUpload(),
			Ubpa::DustEngine::AssetMngr::Instance().LoadAsset<Ubpa::DustEngine::Texture2D>(path)
		);
	}
}

void GameStarter::BuildShaders() {
	auto& assetMngr = Ubpa::DustEngine::AssetMngr::Instance();
	auto shaderGUIDs = assetMngr.FindAssets(std::wregex{ LR"(.*\.shader)" });
	for (const auto& guid : shaderGUIDs) {
		const auto& path = assetMngr.GUIDToAssetPath(guid);
		auto shader = assetMngr.LoadAsset<Ubpa::DustEngine::Shader>(path);
		Ubpa::DustEngine::RsrcMngrDX12::Instance().RegisterShader(shader);
		Ubpa::DustEngine::ShaderMngr::Instance().Register(shader);
	}
}

void GameStarter::BuildMaterials() {
	auto material = Ubpa::DustEngine::AssetMngr::Instance()
		.LoadAsset<Ubpa::DustEngine::Material>(L"..\\assets\\materials\\iron.mat");
	world.RunEntityJob([=](Ubpa::DustEngine::MeshRenderer* meshRenderer) {
		meshRenderer->materials.push_back(material);
	});
}
