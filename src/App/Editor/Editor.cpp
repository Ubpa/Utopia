#include <Utopia/App/Editor/Editor.h>

// DXR
#include <Utopia/Render/DX12/StdDXRPipeline.h>

#include <Utopia/App/Editor/Components/Hierarchy.h>
#include <Utopia/App/Editor/Components/Inspector.h>
#include <Utopia/App/Editor/Components/ProjectViewer.h>
#include <Utopia/App/Editor/Components/SystemController.h>

#include <Utopia/Core/WorldAssetImporter.h>
#include <Utopia/Render/HLSLFileImporter.h>
#include <Utopia/Render/TextureImporter.h>
#include <Utopia/Render/TextureCubeImporter.h>
#include <Utopia/Render/ShaderImporter.h>
#include <Utopia/Render/MaterialImporter.h>
#include <Utopia/Render/MeshImporter.h>
#include <Utopia/Render/ModelImporter.h>
#include <Utopia/Render/DX12/PipelineImporter.h>

#include <Utopia/App/Editor/Systems/HierarchySystem.h>
#include <Utopia/App/Editor/Systems/InspectorSystem.h>
#include <Utopia/App/Editor/Systems/ProjectViewerSystem.h>
#include <Utopia/App/Editor/Systems/LoggerSystem.h>
#include <Utopia/App/Editor/Systems/SystemControllerSystem.h>

#include <Utopia/App/Editor/InspectorRegistry.h>

#include <Utopia/App/DX12App/DX12App.h>

#include <Utopia/Core/AssetMngr.h>
#include <Utopia/Core/Serializer.h>

#include <Utopia/Render/DX12/GPURsrcMngrDX12.h>
#include <Utopia/Render/DX12/StdPipeline.h>
#include <Utopia/Render/Texture2D.h>
#include <Utopia/Render/TextureCube.h>
#include <Utopia/Render/Shader.h>
#include <Utopia/Render/Material.h>
#include <Utopia/Render/HLSLFile.h>
#include <Utopia/Render/ShaderMngr.h>
#include <Utopia/Render/Mesh.h>
#include <Utopia/Render/Components/Components.h>
#include <Utopia/Render/Systems/Systems.h>

#include <Utopia/Core/GameTimer.h>
#include <Utopia/Core/Components/Components.h>
#include <Utopia/Core/Systems/Systems.h>
#include <Utopia/Core/ImGUIMngr.h>

#include <Utopia/Core/Register_Core.h>
#include <Utopia/Render/Register_Render.h>

#include <_deps/imgui/imgui.h>
#include <_deps/imgui/imgui_impl_win32.h>
#include <_deps/imgui/imgui_impl_dx12.h>

#include <Utopia/Core/StringsSink.h>
#include <spdlog/spdlog.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UECS;
using namespace Ubpa;
using Microsoft::WRL::ComPtr;

struct Editor::Impl {
	Impl(Editor* editor) : pEditor{editor}, curGameWorld { &gameWorld } {}
	~Impl();

	Editor* pEditor;

	bool Init();
	void Update();

	void BuildWorld();

	static void InitWorld(Ubpa::UECS::World&);
	//static void InitInspectorRegistry();
	static void LoadInternalTextures();
	static void LoadShaders();

	std::unique_ptr<Ubpa::UECS::World> runningGameWorld;
	Ubpa::UECS::World* curGameWorld;
	Ubpa::UECS::World gameWorld;
	Ubpa::UECS::World sceneWorld;
	Ubpa::UECS::World editorWorld;

	void OnGameResize();
	size_t gameWidth{ 0 }, gameHeight{ 0 };
	ImVec2 gamePos{ 0,0 };
	ComPtr<ID3D12Resource> gameRT;
	const DXGI_FORMAT gameRTFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	Ubpa::UDX12::DescriptorHeapAllocation gameRT_SRV;
	Ubpa::UDX12::DescriptorHeapAllocation gameRT_RTV;

	void OnSceneResize();
	size_t sceneWidth{ 0 }, sceneHeight{ 0 };
	ImVec2 scenePos{ 0,0 };
	ComPtr<ID3D12Resource> sceneRT;
	const DXGI_FORMAT sceneRTFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	Ubpa::UDX12::DescriptorHeapAllocation sceneRT_SRV;
	Ubpa::UDX12::DescriptorHeapAllocation sceneRT_RTV;

	std::shared_ptr<StdPipeline> stdPipeline;
	std::shared_ptr<StdDXRPipeline> stdDXRPipeline;

	bool show_demo_window = true;
	bool show_another_window = false;

	ImGuiContext* gameImGuiCtx = nullptr;
	ImGuiContext* sceneImGuiCtx = nullptr;
	ImGuiContext* editorImGuiCtx = nullptr;

	enum class GameState {
		NotStart,
		Starting,
		Running,
		Stopping,
	};
	GameState gameState = GameState::NotStart;

	static void InspectMaterial(Material* material, InspectorRegistry::InspectContext ctx);
};

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler_Shared(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler_Context(ImGuiContext* ctx, bool ingore_mouse, bool ingore_keyboard, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT Editor::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler_Shared(hwnd, msg, wParam, lParam))
		return 1;

	bool imguiWantCaptureMouse = false;
	bool imguiWantCaptureKeyboard = false;

	if (pImpl->gameImGuiCtx && pImpl->sceneImGuiCtx && pImpl->editorImGuiCtx) {
		ImGui::SetCurrentContext(pImpl->gameImGuiCtx);
		auto& gameIO = ImGui::GetIO();
		bool gameWantCaptureMouse = gameIO.WantCaptureMouse;
		bool gameWantCaptureKeyboard = gameIO.WantCaptureKeyboard;
		ImGui::SetCurrentContext(pImpl->sceneImGuiCtx);
		auto& sceneIO = ImGui::GetIO();
		bool sceneWantCaptureMouse = sceneIO.WantCaptureMouse;
		bool sceneWantCaptureKeyboard = sceneIO.WantCaptureKeyboard;
		ImGui::SetCurrentContext(pImpl->editorImGuiCtx);
		auto& editorIO = ImGui::GetIO();
		bool editorWantCaptureMouse = editorIO.WantCaptureMouse;
		bool editorWantCaptureKeyboard = editorIO.WantCaptureKeyboard;

		if (ImGui_ImplWin32_WndProcHandler_Context(pImpl->gameImGuiCtx, false, false, hwnd, msg, wParam, lParam))
			return 1;

		if (ImGui_ImplWin32_WndProcHandler_Context(pImpl->sceneImGuiCtx, gameWantCaptureMouse, gameWantCaptureKeyboard, hwnd, msg, wParam, lParam))
			return 1;

		if (
			ImGui_ImplWin32_WndProcHandler_Context(
				pImpl->editorImGuiCtx,
				gameWantCaptureMouse || sceneWantCaptureMouse,
				gameWantCaptureKeyboard || sceneWantCaptureKeyboard,
				hwnd, msg, wParam, lParam
			)
		) {
			return 1;
		}

		imguiWantCaptureMouse = gameWantCaptureMouse || sceneWantCaptureMouse || editorWantCaptureMouse;
		imguiWantCaptureKeyboard = gameWantCaptureKeyboard || sceneWantCaptureKeyboard || editorWantCaptureKeyboard;
	}

	// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your editor application.
	// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your editor application.
	switch (msg)
	{
	case WM_KEYUP:
		if (imguiWantCaptureKeyboard)
			return 0;
		if (wParam == VK_ESCAPE)
		{
			//PostQuitMessage(0);
		}
		return 0;
	}

	return DX12App::MsgProc(hwnd, msg, wParam, lParam);
}

Editor::Editor(HINSTANCE hInstance)
	: DX12App(hInstance), pImpl{ new Impl{this} }
{}

Editor::~Editor() {
	FlushCommandQueue();

	ImGUIMngr::Instance().Clear();
	AssetMngr::Instance().Clear();

	delete pImpl;
}

bool Editor::Init() {
	if (!InitMainWindow())
		return false;

	if (!InitDirect3D())
		return false;

	OnResize();

	if (!pImpl->Init())
		return false;

	FlushCommandQueue();

	return true;
}

World* Editor::GetGameWorld() {
	return &pImpl->gameWorld;
}

World* Editor::GetSceneWorld() {
	return &pImpl->sceneWorld;
}
World* Editor::GetEditorWorld() {
	return &pImpl->editorWorld;
}

UECS::World* Editor::GetCurrentGameWorld() {
	return pImpl->curGameWorld;
}

void Editor::Update() {
	pImpl->Update();
}

Editor::Impl::~Impl() {
	if (!gameRT_SRV.IsNull())
		Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(gameRT_SRV));
	if (!gameRT_RTV.IsNull())
		Ubpa::UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Free(std::move(gameRT_RTV));
	if (!sceneRT_SRV.IsNull())
		Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(sceneRT_SRV));
	if (!sceneRT_RTV.IsNull())
		Ubpa::UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Free(std::move(sceneRT_RTV));
}

bool Editor::Impl::Init() {
	ImGUIMngr::Instance().Init(pEditor->MainWnd(), pEditor->uDevice.Get(), DX12App::NumFrameResources);
	editorImGuiCtx = ImGUIMngr::Instance().CreateContext("editor");
	gameImGuiCtx = ImGUIMngr::Instance().CreateContext("game");
	sceneImGuiCtx = ImGUIMngr::Instance().CreateContext("scene");
	ImGui::SetCurrentContext(editorImGuiCtx);
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui::GetIO().IniFilename = "imgui_App_Editor_editor.ini";
	ImGui::SetCurrentContext(gameImGuiCtx);
	ImGui::GetIO().IniFilename = "imgui_App_Editor_game.ini";
	ImGui::SetCurrentContext(sceneImGuiCtx);
	ImGui::GetIO().IniFilename = "imgui_App_Editor_scene.ini";
	ImGui::SetCurrentContext(nullptr);
	
	AssetMngr::Instance().SetRootPath(LR"(..\assets)");
	AssetMngr::Instance().RegisterAssetImporterCreator(std::make_shared<WorldAssetImporterCreator>());
	AssetMngr::Instance().RegisterAssetImporterCreator(std::make_shared<HLSLFileImporterCreator>());
	AssetMngr::Instance().RegisterAssetImporterCreator(std::make_shared<TextureImporterCreator>());
	AssetMngr::Instance().RegisterAssetImporterCreator(std::make_shared<TextureCubeImporterCreator>());
	AssetMngr::Instance().RegisterAssetImporterCreator(std::make_shared<ShaderImporterCreator>());
	AssetMngr::Instance().RegisterAssetImporterCreator(std::make_shared<MaterialImporterCreator>());
	AssetMngr::Instance().RegisterAssetImporterCreator(std::make_shared<MeshImporterCreator>());
	AssetMngr::Instance().RegisterAssetImporterCreator(std::make_shared<ModelImporterCreator>());
	AssetMngr::Instance().RegisterAssetImporterCreator(std::make_shared<PipelineImporterCreator>());
	AssetMngr::Instance().ImportAssetRecursively(LR"(.)");
	
	//InitInspectorRegistry();
	InspectorRegistry::Instance().Register(&Editor::Impl::InspectMaterial);

	UDRefl_Register_Core();
	UDRefl_Register_Render();

	LoadInternalTextures();
	LoadShaders();

	stdPipeline = AssetMngr::Instance().LoadAsset<StdPipeline>(LR"(_internal\pipelines\StdPipeline.pipeline)");
	stdDXRPipeline = AssetMngr::Instance().LoadAsset<StdDXRPipeline>(LR"(_internal\pipelines\StdDXRPipeline.pipeline)");

	gameRT_SRV = Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(1);
	gameRT_RTV = Ubpa::UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Allocate(1);
	sceneRT_SRV = Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(1);
	sceneRT_RTV = Ubpa::UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Allocate(1);

	auto logger = spdlog::synchronous_factory::create<StringsSink>("logger");
	spdlog::details::registry::instance().set_default_logger(logger);

	BuildWorld();

	return true;
}

void Editor::Impl::OnGameResize() {
	Ubpa::rgbaf background = { 0.f,0.f,0.f,1.f };
	CD3DX12_CLEAR_VALUE clearvalue(gameRTFormat, background.data());
	auto desc = Ubpa::UDX12::Desc::RSRC::RT2D(gameWidth, (UINT)gameHeight, gameRTFormat);
	const auto defaultHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(pEditor->uDevice->CreateCommittedResource(
		&defaultHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_PRESENT,
		&clearvalue,
		IID_PPV_ARGS(gameRT.ReleaseAndGetAddressOf())
	));
	pEditor->uDevice->CreateShaderResourceView(gameRT.Get(), nullptr, gameRT_SRV.GetCpuHandle());
	pEditor->uDevice->CreateRenderTargetView(gameRT.Get(), nullptr, gameRT_RTV.GetCpuHandle());
}

void Editor::Impl::OnSceneResize() {
	Ubpa::rgbaf background = { 0.f,0.f,0.f,1.f };
	CD3DX12_CLEAR_VALUE clearvalue(sceneRTFormat, background.data());
	auto desc = Ubpa::UDX12::Desc::RSRC::RT2D(sceneWidth, (UINT)sceneHeight, sceneRTFormat);
	const auto defaultHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(pEditor->uDevice->CreateCommittedResource(
		&defaultHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_PRESENT,
		&clearvalue,
		IID_PPV_ARGS(sceneRT.ReleaseAndGetAddressOf())
	));
	pEditor->uDevice->CreateShaderResourceView(sceneRT.Get(), nullptr, sceneRT_SRV.GetCpuHandle());
	pEditor->uDevice->CreateRenderTargetView(sceneRT.Get(), nullptr, sceneRT_RTV.GetCpuHandle());
}

void Editor::Impl::Update() {
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame_Shared();
	ImGui_ImplWin32_NewFrame(editorImGuiCtx, { 0.f, 0.f }, (float)pEditor->mClientWidth, (float)pEditor->mClientHeight);
	ImGui_ImplWin32_NewFrame(gameImGuiCtx, gamePos, (float)gameWidth, (float)gameHeight);
	ImGui_ImplWin32_NewFrame(sceneImGuiCtx, scenePos, (float)sceneWidth, (float)sceneHeight);

	bool isGameOpen = false;
	bool isSceneOpen = false;
	{ // editor
		ImGui::SetCurrentContext(editorImGuiCtx);
		ImGui::NewFrame(); // editor ctx

		static bool opt_fullscreen = true;
		static bool opt_padding = false;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->GetWorkPos());
			ImGui::SetNextWindowSize(viewport->GetWorkSize());
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}
		else
		{
			dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
		}

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
		// and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		if (!opt_padding)
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", nullptr, window_flags);
		if (!opt_padding)
			ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}
		else
		{
			ImGui::Text("ERROR: Docking is not enabled! See Demo > Configuration.");
			ImGui::Text("Set io.ConfigFlags |= ImGuiConfigFlags_DockingEnable in your code, or ");
			ImGui::SameLine(0.0f, 0.0f);
			if (ImGui::SmallButton("click here"))
				io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		}

		ImGui::End();

		bool isFlush = false;
		// 1. game window
		if (ImGui::Begin("Game", nullptr, ImGuiWindowFlags_NoScrollbar)) {
			isGameOpen = true;
			auto content_max_minus_local_pos = ImGui::GetContentRegionAvail();
			auto content_max = ImGui::GetWindowContentRegionMax();
			auto game_window_pos = ImGui::GetWindowPos();
			gamePos.x = game_window_pos.x + content_max.x - content_max_minus_local_pos.x;
			gamePos.y = game_window_pos.y + content_max.y - content_max_minus_local_pos.y;

			auto gameSize = ImGui::GetContentRegionAvail();
			auto w = (size_t)gameSize.x;
			auto h = (size_t)gameSize.y;
			if (gameSize.x > 0 && gameSize.y > 0 && w > 0 && h > 0 && (w != gameWidth || h != gameHeight)) {
				gameWidth = w;
				gameHeight = h;

				if (!isFlush) {
					// Flush before changing any resources.
					pEditor->FlushCommandQueue();
					isFlush = true;
				}

				OnGameResize();
			}
			ImGui::Image(
				ImTextureID(gameRT_SRV.GetGpuHandle().ptr),
				ImGui::GetContentRegionAvail()
			);
		}
		ImGui::End(); // game window

		// 2. scene window
		if (ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoScrollbar)) {
			isSceneOpen = true;
			auto content_max_minus_local_pos = ImGui::GetContentRegionAvail();
			auto content_max = ImGui::GetWindowContentRegionMax();
			auto scene_window_pos = ImGui::GetWindowPos();
			scenePos.x = scene_window_pos.x + content_max.x - content_max_minus_local_pos.x;
			scenePos.y = scene_window_pos.y + content_max.y - content_max_minus_local_pos.y;

			auto sceneSize = ImGui::GetContentRegionAvail();
			auto w = (size_t)sceneSize.x;
			auto h = (size_t)sceneSize.y;
			if (sceneSize.x > 0 && sceneSize.y > 0 && w > 0 && h > 0 && (w != sceneWidth || h != sceneHeight)) {
				sceneWidth = w;
				sceneHeight = h;

				if (!isFlush) {
					// Flush before changing any resources.
					pEditor->FlushCommandQueue();
					isFlush = true;
				}

				OnSceneResize();
			}
			ImGui::Image(
				ImTextureID(sceneRT_SRV.GetGpuHandle().ptr),
				ImGui::GetContentRegionAvail()
			);
		}
		ImGui::End(); // scene window

		// 3.game control
		if (ImGui::Begin("Game Control", nullptr, ImGuiWindowFlags_NoScrollbar)) {
			static std::string startStr = "start";
			if (ImGui::Button(startStr.c_str())) {
				switch (gameState)
				{
				case Impl::GameState::NotStart:
					startStr = "stop";
					gameState = Impl::GameState::Starting;
					break;
				case Impl::GameState::Running:
					startStr = "start";
					gameState = Impl::GameState::Stopping;
					break;
				case Impl::GameState::Starting:
				case Impl::GameState::Stopping:
				default:
					assert("error" && false);
					break;
				}
			}
		}
		ImGui::End(); // Game Control window

		editorWorld.Update();
	}

	{ // game update
		ImGui::SetCurrentContext(gameImGuiCtx);
		ImGui::NewFrame(); // game ctx

		switch (gameState)
		{
		case Impl::GameState::NotStart:
			gameWorld.Update();
			break;
		case Impl::GameState::Starting:
		{
			runningGameWorld = std::make_unique<Ubpa::UECS::World>(gameWorld);
			if (auto hierarchy = editorWorld.entityMngr.WriteSingleton<Hierarchy>())
				hierarchy->world = runningGameWorld.get();
			if (auto ctrl = editorWorld.entityMngr.WriteSingleton<SystemController>())
				ctrl->world = runningGameWorld.get();
			curGameWorld = runningGameWorld.get();
			gameState = Impl::GameState::Running;
			GameTimer::Instance().Reset();
			// break;
		}
		[[fallthrough]];
		case Impl::GameState::Running:
			runningGameWorld->Update();
			//ImGui::Begin("in game");
			//ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			//ImGui::End();
			break;
		case Impl::GameState::Stopping:
		{
			auto w = runningGameWorld.get();
			runningGameWorld.reset();
			if (auto hierarchy = editorWorld.entityMngr.WriteSingleton<Hierarchy>())
				hierarchy->world = &gameWorld;
			if (auto ctrl = editorWorld.entityMngr.WriteSingleton<SystemController>())
				ctrl->world = &gameWorld;
			curGameWorld = &gameWorld;
			gameState = Impl::GameState::NotStart;
			break;
		}
		default:
			break;
		}
	}

	{ // scene update
		ImGui::SetCurrentContext(sceneImGuiCtx);
		ImGui::NewFrame(); // scene ctx

		//UpdateCamera();
		sceneWorld.Update();
		ImGui::Begin("in scene");
		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::End();
	}

	ImGui::SetCurrentContext(nullptr);

	// update mesh, texture ...
	pEditor->GetFrameResourceMngr()->BeginFrame();

	auto cmdAlloc = pEditor->GetCurFrameCommandAllocator();
	cmdAlloc->Reset();

	ThrowIfFailed(pEditor->uGCmdList->Reset(cmdAlloc, nullptr));

	auto UpdateRenderResource = [&](Ubpa::UECS::World* w) {
		w->RunEntityJob([&](MeshFilter* meshFilter, MeshRenderer* meshRenderer) {
			if (!meshFilter->mesh)
				return;

			GPURsrcMngrDX12::Instance().RegisterMesh(
				pEditor->uGCmdList.Get(),
				*meshFilter->mesh
			);

			for (auto& material : meshRenderer->materials) {
				if (!material)
					continue;
				for (auto& [name, property] : material->properties) {
					if (std::holds_alternative<SharedVar<Texture2D>>(property.value)) {
						GPURsrcMngrDX12::Instance().RegisterTexture2D(
							*std::get<SharedVar<Texture2D>>(property.value)
						);
					}
					else if (std::holds_alternative<SharedVar<TextureCube>>(property.value)) {
						GPURsrcMngrDX12::Instance().RegisterTextureCube(
							*std::get<SharedVar<TextureCube>>(property.value)
						);
					}
				}
			}
		}, false);

		if (auto skybox = w->entityMngr.WriteSingleton<Skybox>(); skybox && skybox->material) {
			for (auto& [name, property] : skybox->material->properties) {
				if (std::holds_alternative<SharedVar<Texture2D>>(property.value)) {
					GPURsrcMngrDX12::Instance().RegisterTexture2D(
						*std::get<SharedVar<Texture2D>>(property.value)
					);
				}
				else if (std::holds_alternative<SharedVar<TextureCube>>(property.value)) {
					GPURsrcMngrDX12::Instance().RegisterTextureCube(
						*std::get<SharedVar<TextureCube>>(property.value)
					);
				}
			}
		}
	};
	UpdateRenderResource(&gameWorld);
	UpdateRenderResource(&sceneWorld);

	// commit upload, delete ...
	pEditor->uGCmdList->Close();
	auto deletebatch = GPURsrcMngrDX12::Instance().CommitUploadAndTakeDeleteBatch(pEditor->uCmdQueue.Get()); // upload before execute uGCmdList
	pEditor->uCmdQueue.Execute(pEditor->uGCmdList.Get());
	pEditor->GetFrameResourceMngr()->GetCurrentFrameResource()->RegisterResource("deletebatch", std::make_shared<UDX12::ResourceDeleteBatch>(std::move(deletebatch)));
	pEditor->GetFrameResourceMngr()->GetCurrentFrameResource()->DelayUnregisterResource("deletebatch");

	{
		static bool flag = false;
		if (!flag) {
			OutputDebugStringA(curGameWorld->GenUpdateFrameGraph().Dump().c_str());
			flag = true;
		}
	}

	ThrowIfFailed(pEditor->uGCmdList->Reset(cmdAlloc, nullptr));


	std::map<IPipeline*, std::vector<IPipeline::CameraData>> gameCameras;
	std::map<IPipeline*, std::vector<IPipeline::CameraData>> sceneCameras;

	if (isGameOpen) {
		curGameWorld->RunEntityJob([&](Ubpa::UECS::Entity e, Camera* cam) {
			IPipeline* pipeline;
			switch (cam->pipeline_mode) {
			case Camera::PipelineMode::Std:
				pipeline = stdPipeline.get();
				break;
			case Camera::PipelineMode::StdDXR:
				pipeline = stdDXRPipeline.get();
				break;
			case Camera::PipelineMode::Custom:
				pipeline = cam->custom_pipeline.get();
				if (!pipeline)
					pipeline = stdPipeline.get(); // default
				break;
			}
			gameCameras[pipeline].emplace_back(e, curGameWorld);
		}, false);
	}
	if (isSceneOpen) {
		sceneWorld.RunEntityJob([&](Ubpa::UECS::Entity e, Camera* cam) {
			IPipeline* pipeline;
			switch (cam->pipeline_mode) {
			case Camera::PipelineMode::Std:
				pipeline = stdPipeline.get();
				break;
			case Camera::PipelineMode::StdDXR:
				pipeline = stdDXRPipeline.get();
				break;
			case Camera::PipelineMode::Custom:
				pipeline = cam->custom_pipeline.get();
				if (!pipeline)
					pipeline = stdPipeline.get(); // default
				break;
			}
			sceneCameras[pipeline].emplace_back(e, &sceneWorld);
		}, false);
	}

	std::set<IPipeline*> pipelines;
	for (const auto& [pipeline, cameras] : gameCameras)
		pipelines.insert(pipeline);
	for (const auto& [pipeline, cameras] : sceneCameras)
		pipelines.insert(pipeline);


	IPipeline::InitDesc initDesc;
	initDesc.device = pEditor->uDevice.Get();
	initDesc.cmdQueue = pEditor->uCmdQueue.Get();
	initDesc.numFrame = DX12App::NumFrameResources;

	for (auto* pipeline : pipelines) {
		std::vector<UECS::World*> worlds;
		std::vector<IPipeline::CameraData> cameras;
		std::vector<IPipeline::WorldCameraLink> links;
		std::vector<ID3D12Resource*> defaultRTs;

		// fill worlds
		{
			if (sceneCameras.contains(pipeline))
				worlds = { curGameWorld, &sceneWorld };
			else if (gameCameras.contains(pipeline))
				worlds = { curGameWorld };
		}

		// fill cameras
		{
			if (gameCameras.contains(pipeline))
				cameras.insert(cameras.end(), gameCameras.at(pipeline).begin(), gameCameras.at(pipeline).end());

			if (sceneCameras.contains(pipeline))
				cameras.insert(cameras.end(), sceneCameras.at(pipeline).begin(), sceneCameras.at(pipeline).end());
		}

		if (gameCameras.contains(pipeline)) {
			IPipeline::WorldCameraLink link;
			link.worldIndices = { 0 };
			for (size_t i = 0; i < gameCameras.at(pipeline).size(); i++)
				link.cameraIndices.push_back(i);
			links.push_back(std::move(link));

			for (size_t i = 0; i < gameCameras.at(pipeline).size(); i++)
				defaultRTs.push_back(gameRT.Get());
		}

		if (sceneCameras.contains(pipeline)) {
			IPipeline::WorldCameraLink link;
			link.worldIndices = { 0, 1 };

			const size_t gameCamOffset = gameCameras.contains(pipeline) ? gameCameras.at(pipeline).size() : 0;
			for (size_t i = 0; i < sceneCameras.at(pipeline).size(); i++)
				link.cameraIndices.push_back(gameCamOffset + i);
			links.push_back(std::move(link));

			for (size_t i = 0; i < sceneCameras.at(pipeline).size(); i++)
				defaultRTs.push_back(sceneRT.Get());
		}

		pipeline->Init(initDesc);

		bool containNullRT = false;
		for (auto* RT : defaultRTs) {
			if (RT == nullptr) {
				containNullRT = true;
				break;
			}
		}
		if (!containNullRT)
			pipeline->Render(worlds, cameras, links, defaultRTs);
	}

	{ // game
		ImGui::SetCurrentContext(gameImGuiCtx);
		ImGui::Render();

		if (gameRT)
		{
			pEditor->uGCmdList.ResourceBarrierTransition(gameRT.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			const auto gameRTHandle = gameRT_RTV.GetCpuHandle();
			pEditor->uGCmdList->OMSetRenderTargets(1, &gameRTHandle, FALSE, NULL);
			pEditor->uGCmdList.SetDescriptorHeaps(Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap());
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pEditor->uGCmdList.Get());
			pEditor->uGCmdList.ResourceBarrierTransition(gameRT.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		}
	}

	{ // scene
		ImGui::SetCurrentContext(sceneImGuiCtx);
		ImGui::Render();

		if (sceneRT)
		{
			pEditor->uGCmdList.ResourceBarrierTransition(sceneRT.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			const auto sceneRTHandle = sceneRT_RTV.GetCpuHandle();
			pEditor->uGCmdList->OMSetRenderTargets(1, &sceneRTHandle, FALSE, NULL);
			pEditor->uGCmdList.SetDescriptorHeaps(Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap());
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pEditor->uGCmdList.Get());
			pEditor->uGCmdList.ResourceBarrierTransition(sceneRT.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		}
	}

	{ // editor
		ImGui::SetCurrentContext(editorImGuiCtx);

		pEditor->uGCmdList.ResourceBarrierTransition(pEditor->CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pEditor->uGCmdList->ClearRenderTargetView(pEditor->CurrentBackBufferView(), DirectX::Colors::Black, 0, NULL);
		const auto curBack = pEditor->CurrentBackBufferView();
		pEditor->uGCmdList->OMSetRenderTargets(1, &curBack, FALSE, NULL);
		pEditor->uGCmdList.SetDescriptorHeaps(Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap());
		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pEditor->uGCmdList.Get());
		pEditor->uGCmdList.ResourceBarrierTransition(pEditor->CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	}

	pEditor->uGCmdList->Close();
	pEditor->uCmdQueue.Execute(pEditor->uGCmdList.Get());

	pEditor->SwapBackBuffer();

	pEditor->GetFrameResourceMngr()->EndFrame(pEditor->uCmdQueue.Get());
}

void Editor::Impl::InitWorld(Ubpa::UECS::World& w) {
	auto indices = w.systemMngr.systemTraits.Register<
		// transform
		LocalToParentSystem,
		RotationEulerSystem,
		TRSToLocalToParentSystem,
		TRSToLocalToWorldSystem,
		PrevLocalToWorldSystem,
		WorldToLocalSystem,

		// core
		WorldTimeSystem,
		CameraSystem,
		InputSystem,
		RoamerSystem,

		// editor
		HierarchySystem,
		InspectorSystem,
		ProjectViewerSystem,
		SystemControllerSystem
	>();
	for (auto idx : indices)
		w.systemMngr.Activate(idx);

	w.entityMngr.cmptTraits.Register<
		// transform
		Children,
		LocalToParent,
		LocalToWorld,
		PrevLocalToWorld,
		Parent,
		Rotation,
		RotationEuler,
		Scale,
		NonUniformScale,
		Translation,
		WorldToLocal,

		// core
		Camera,
		MeshFilter,
		MeshRenderer,
		WorldTime,
		Name,
		Skybox,
		Light,
		Input,
		Roamer,

		// editor
		Hierarchy,
		Inspector,
		ProjectViewer,
		SystemController
	>();
}

void Editor::Impl::BuildWorld() {
	{ // game
		InitWorld(gameWorld);
	}

	{ // scene
		InitWorld(sceneWorld);
		{ // scene camera
			auto e = sceneWorld.entityMngr.Create(TypeIDs_of<
				LocalToWorld,
				WorldToLocal,
				Camera,
				Translation,
				Rotation,
				Roamer
			>);
			auto roamer = sceneWorld.entityMngr.WriteComponent<Roamer>(e);
			roamer->reverseFrontBack = true;
			roamer->reverseLeftRight = true;
			roamer->moveSpeed = 1.f;
			roamer->rotateSpeed = 0.1f;
		}

		{ // hierarchy
			auto e = sceneWorld.entityMngr.Create(TypeIDs_of<Hierarchy>);
			auto hierarchy = sceneWorld.entityMngr.WriteComponent<Hierarchy>(e);
			hierarchy->world = &sceneWorld;
		}
		sceneWorld.entityMngr.Create(TypeIDs_of<WorldTime>);
		sceneWorld.entityMngr.Create(TypeIDs_of<ProjectViewer>);
		sceneWorld.entityMngr.Create(TypeIDs_of<Inspector>);
		sceneWorld.entityMngr.Create(TypeIDs_of<Input>);
	}
	
	{ // editor
		InitWorld(editorWorld);
		{ // hierarchy
			auto e = editorWorld.entityMngr.Create(TypeIDs_of<Hierarchy>);
			auto hierarchy = editorWorld.entityMngr.WriteComponent<Hierarchy>(e);
			hierarchy->world = &gameWorld;
		}
		{ // system controller
			auto e = editorWorld.entityMngr.Create(TypeIDs_of<SystemController>);
			auto systemCtrl = editorWorld.entityMngr.WriteComponent<SystemController>(e);
			systemCtrl->world = &gameWorld;
		}
		editorWorld.entityMngr.Create(TypeIDs_of<Inspector>);
		editorWorld.entityMngr.Create(TypeIDs_of<ProjectViewer>);
		editorWorld.entityMngr.Create(TypeIDs_of<Input>);
		editorWorld.systemMngr.RegisterAndActivate<LoggerSystem, SystemControllerSystem>();
	}
}

void Editor::Impl::LoadInternalTextures() {
	auto tex2dGUIDs = AssetMngr::Instance().FindAssets(std::wregex{ LR"(_internal\\.*\.(png|jpg|bmp|hdr|tga))" });
	for (const auto& guid : tex2dGUIDs) {
		const auto& path = AssetMngr::Instance().GUIDToAssetPath(guid);
		auto asset = AssetMngr::Instance().LoadMainAsset(path);
		if (asset.GetType().Is<Texture2D>())
			GPURsrcMngrDX12::Instance().RegisterTexture2D(*asset.AsShared<Texture2D>());
		else if (asset.GetType().Is<TextureCube>())
			GPURsrcMngrDX12::Instance().RegisterTextureCube(*asset.AsShared<TextureCube>());
		else
			assert(false);
	}
	auto texcubeGUIDs = AssetMngr::Instance().FindAssets(std::wregex{ LR"(_internal\\.*\.texcube)" });
	for (const auto& guid : texcubeGUIDs) {
		const auto& path = AssetMngr::Instance().GUIDToAssetPath(guid);
		auto asset = AssetMngr::Instance().LoadMainAsset(path);
		if (asset.GetType().Is<TextureCube>())
			GPURsrcMngrDX12::Instance().RegisterTextureCube(*asset.AsShared<TextureCube>());
		else
			assert(false);
	}
}

void Editor::Impl::LoadShaders() {
	auto& assetMngr = AssetMngr::Instance();
	auto shaderGUIDs = assetMngr.FindAssets(std::wregex{ LR"(.*\.shader)" });
	for (const auto& guid : shaderGUIDs) {
		const auto& path = assetMngr.GUIDToAssetPath(guid);
		auto shader = assetMngr.LoadAsset<Shader>(path);
		GPURsrcMngrDX12::Instance().RegisterShader(*shader);
		ShaderMngr::Instance().Register(shader);
	}
}

void Editor::Impl::InspectMaterial(Material* material, InspectorRegistry::InspectContext ctx) {
	std::string header = std::string{ AssetMngr::Instance().NameofAsset(material) } + " (" + std::string{ type_name<Material>().View() } + ")";
	
	if (ImGui::CollapsingHeader(header.data())) {
		ImGui::PushID(material);

		ImGui::Text("(*)");
		ImGui::SameLine();

		{ // button
			if (material->shader) {
				if (ImGui::Button(material->shader->name.c_str()))
					ImGui::OpenPopup("Meterial_Shader_Seletor");
			}
			else {
				if (ImGui::Button("nullptr"))
					ImGui::OpenPopup("Meterial_Shader_Seletor");
			}
		}

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(InspectorRegistry::Playload::Asset)) {
				IM_ASSERT(payload->DataSize == sizeof(InspectorRegistry::Playload::Asset));
				const auto& asset_handle = *(InspectorRegistry::Playload::AssetHandle*)payload->Data;
				UDRefl::SharedObject asset;
				if (asset_handle.name.empty()) {
					// main
					asset = AssetMngr::Instance().GUIDToMainAsset(asset_handle.guid);
				}
				else
					asset = AssetMngr::Instance().GUIDToAsset(asset_handle.guid, asset_handle.name);
				if (asset.GetType().Is<Shader>()) {
					material->shader = asset.AsShared<Shader>();
					material->properties = material->shader->properties;
				}
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::BeginPopup("Meterial_Shader_Seletor")) {
			if (material->shader)
				ImGui::PushID((void*)material->shader->GetInstanceID());
			else
				ImGui::PushID(0);
			// Helper class to easy setup a text filter.
			// You may want to implement a more feature-full filtering scheme in your own application.
			static ImGuiTextFilter filter;
			filter.Draw();
			int ID = 0;
			size_t N = ShaderMngr::Instance().GetShaderMap().size();
			for (const auto& [name, shader] : ShaderMngr::Instance().GetShaderMap()) {
				auto shader_s = shader.lock();
				if (shader_s.get() != material->shader.get() && filter.PassFilter(name.c_str())) {
					ImGui::PushID(ID);
					ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(ID / float(N), 0.6f, 0.6f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(ID / float(N), 0.7f, 0.7f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(ID / float(N), 0.8f, 0.8f));
					if (ImGui::Button(name.c_str())) {
						material->shader = shader_s;
						material->properties = shader_s->properties;
					}
					ImGui::PopStyleColor(3);
					ImGui::PopID();
				}
				ID++;
			}
			ImGui::PopID();
			ImGui::EndPopup();
		}
		ImGui::SameLine();
		ImGui::Text("shader");

		for (const auto& [n, var] : UDRefl::ObjectView{ Type_of<Material>, material }.GetVars()) {
			if (n == "shader")
				continue;
			InspectorRegistry::Instance().InspectRecursively(n, var.GetType().GetID(), var.GetPtr(), ctx);
		}

		if (ImGui::Button("apply"))
			AssetMngr::Instance().ReserializeAsset(AssetMngr::Instance().GetAssetPath(material));

		ImGui::PopID();
	}
}
