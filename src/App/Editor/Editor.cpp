#include <Utopia/App/Editor/Editor.h>

#include <Utopia/App/Editor/Components/Hierarchy.h>
#include <Utopia/App/Editor/Components/Inspector.h>
#include <Utopia/App/Editor/Components/ProjectViewer.h>
#include <Utopia/App/Editor/Components/SystemController.h>

#include <Utopia/App/Editor/Systems/HierarchySystem.h>
#include <Utopia/App/Editor/Systems/InspectorSystem.h>
#include <Utopia/App/Editor/Systems/ProjectViewerSystem.h>
#include <Utopia/App/Editor/Systems/LoggerSystem.h>
#include <Utopia/App/Editor/Systems/SystemControllerSystem.h>

#include <Utopia/App/Editor/InspectorRegistry.h>

#include <Utopia/App/DX12App/DX12App.h>

#include <Utopia/Asset/AssetMngr.h>
#include <Utopia/Asset/Serializer.h>

#include <Utopia/Render/DX12/RsrcMngrDX12.h>
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

#include <Utopia/Core/Scene.h>
#include <Utopia/Core/GameTimer.h>
#include <Utopia/Core/Components/Components.h>
#include <Utopia/Core/Systems/Systems.h>
#include <Utopia/Core/ImGUIMngr.h>

#include <_deps/imgui/imgui.h>
#include <_deps/imgui/imgui_impl_win32.h>
#include <_deps/imgui/imgui_impl_dx12.h>

#include <Utopia/Core/StringsSink.h>
#include <spdlog/spdlog.h>

#include <Utopia/ScriptSystem/LuaContext.h>
#include <Utopia/ScriptSystem/LuaCtxMngr.h>
#include <Utopia/ScriptSystem/LuaScript.h>
#include <Utopia/ScriptSystem/LuaScriptQueue.h>
#include <Utopia/ScriptSystem/LuaScriptQueueSystem.h>

#include <ULuaPP/ULuaPP.h>

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
	void Draw();

	void BuildWorld();

	static void InitWorld(Ubpa::UECS::World&);
	static void InitInspectorRegistry();
	static void LoadTextures();
	static void BuildShaders();

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
	std::unique_ptr<PipelineBase> gamePipeline;

	void OnSceneResize();
	size_t sceneWidth{ 0 }, sceneHeight{ 0 };
	ImVec2 scenePos{ 0,0 };
	ComPtr<ID3D12Resource> sceneRT;
	const DXGI_FORMAT sceneRTFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	Ubpa::UDX12::DescriptorHeapAllocation sceneRT_SRV;
	Ubpa::UDX12::DescriptorHeapAllocation sceneRT_RTV;
	std::unique_ptr<PipelineBase> scenePipeline;

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
			PostQuitMessage(0);
		}
		return 0;
	}

	return DX12App::MsgProc(hwnd, msg, wParam, lParam);
}

Editor::Editor(HINSTANCE hInstance)
	: DX12App(hInstance), pImpl{ new Impl{this} }
{}

Editor::~Editor() {
	ImGUIMngr::Instance().Clear();

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

void Editor::Draw() {
	pImpl->Draw();
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
	ImGUIMngr::Instance().Init(pEditor->MainWnd(), pEditor->uDevice.Get(), DX12App::NumFrameResources, 3);
	
	AssetMngr::Instance().ImportAssetRecursively(L"..\\assets");
	InitInspectorRegistry();

	LoadTextures();
	BuildShaders();
	PipelineBase::InitDesc initDesc;
	initDesc.device = pEditor->uDevice.Get();
	initDesc.rtFormat = gameRTFormat;
	initDesc.cmdQueue = pEditor->uCmdQueue.Get();
	initDesc.numFrame = DX12App::NumFrameResources;
	gamePipeline = std::make_unique<StdPipeline>(initDesc);
	scenePipeline = std::make_unique<StdPipeline>(initDesc);
	RsrcMngrDX12::Instance().CommitUploadAndDelete(pEditor->uCmdQueue.Get());

	gameRT_SRV = Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(1);
	gameRT_RTV = Ubpa::UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Allocate(1);
	sceneRT_SRV = Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(1);
	sceneRT_RTV = Ubpa::UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Allocate(1);

	editorImGuiCtx = ImGUIMngr::Instance().GetContexts().at(0);
	gameImGuiCtx = ImGUIMngr::Instance().GetContexts().at(1);
	sceneImGuiCtx = ImGUIMngr::Instance().GetContexts().at(2);
	ImGui::SetCurrentContext(editorImGuiCtx);
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui::GetIO().IniFilename = "imgui_App_Editor_editor.ini";
	ImGui::SetCurrentContext(gameImGuiCtx);
	ImGui::GetIO().IniFilename = "imgui_App_Editor_game.ini";
	ImGui::SetCurrentContext(sceneImGuiCtx);
	ImGui::GetIO().IniFilename = "imgui_App_Editor_scene.ini";
	ImGui::SetCurrentContext(nullptr);

	auto logger = spdlog::synchronous_factory::create<StringsSink>("logger");
	spdlog::details::registry::instance().set_default_logger(logger);

	BuildWorld();

	return true;
}

void Editor::Impl::OnGameResize() {
	Ubpa::rgbaf background = { 0.f,0.f,0.f,1.f };
	auto rtType = Ubpa::UDX12::FG::RsrcType::RT2D(gameRTFormat, gameWidth, (UINT)gameHeight, background.data());
	const auto defaultHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(pEditor->uDevice->CreateCommittedResource(
		&defaultHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&rtType.desc,
		D3D12_RESOURCE_STATE_PRESENT,
		&rtType.clearValue,
		IID_PPV_ARGS(gameRT.ReleaseAndGetAddressOf())
	));
	pEditor->uDevice->CreateShaderResourceView(gameRT.Get(), nullptr, gameRT_SRV.GetCpuHandle());
	pEditor->uDevice->CreateRenderTargetView(gameRT.Get(), nullptr, gameRT_RTV.GetCpuHandle());

	assert(gamePipeline);
	D3D12_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(gameWidth);
	viewport.Height = static_cast<float>(gameHeight);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	gamePipeline->Resize(gameWidth, gameHeight, viewport, { 0, 0, (LONG)gameWidth, (LONG)gameHeight });
}

void Editor::Impl::OnSceneResize() {
	Ubpa::rgbaf background = { 0.f,0.f,0.f,1.f };
	auto rtType = Ubpa::UDX12::FG::RsrcType::RT2D(sceneRTFormat, sceneWidth, (UINT)sceneHeight, background.data());
	const auto defaultHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(pEditor->uDevice->CreateCommittedResource(
		&defaultHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&rtType.desc,
		D3D12_RESOURCE_STATE_PRESENT,
		&rtType.clearValue,
		IID_PPV_ARGS(sceneRT.ReleaseAndGetAddressOf())
	));
	pEditor->uDevice->CreateShaderResourceView(sceneRT.Get(), nullptr, sceneRT_SRV.GetCpuHandle());
	pEditor->uDevice->CreateRenderTargetView(sceneRT.Get(), nullptr, sceneRT_RTV.GetCpuHandle());

	assert(scenePipeline);
	D3D12_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(sceneWidth);
	viewport.Height = static_cast<float>(sceneHeight);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	scenePipeline->Resize(sceneWidth, sceneHeight, viewport, { 0, 0, (LONG)sceneWidth, (LONG)sceneHeight });
}

void Editor::Impl::Update() {
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame_Context(editorImGuiCtx, { 0.f, 0.f }, (float)pEditor->mClientWidth, (float)pEditor->mClientHeight);
	ImGui_ImplWin32_NewFrame_Context(gameImGuiCtx, gamePos, (float)gameWidth, (float)gameHeight);
	ImGui_ImplWin32_NewFrame_Context(sceneImGuiCtx, scenePos, (float)sceneWidth, (float)sceneHeight);
	ImGui_ImplWin32_NewFrame_Shared();

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
			if (auto hierarchy = editorWorld.entityMngr.GetSingleton<Hierarchy>())
				hierarchy->world = runningGameWorld.get();
			if (auto ctrl = editorWorld.entityMngr.GetSingleton<SystemController>())
				ctrl->world = runningGameWorld.get();
			curGameWorld = runningGameWorld.get();
			runningGameWorld->systemMngr.Activate(runningGameWorld->systemMngr.systemTraits.GetID(UECS::SystemTraits::StaticNameof<LuaScriptQueueSystem>()));
			auto ctx = LuaCtxMngr::Instance().Register(runningGameWorld.get());
			sol::state_view lua{ ctx->Main() };
			lua["world"] = runningGameWorld.get();
			gameState = Impl::GameState::Running;
			GameTimer::Instance().Reset();
			// break;
		}
		case Impl::GameState::Running:
			runningGameWorld->Update();
			ImGui::Begin("in game");
			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			ImGui::End();
			break;
		case Impl::GameState::Stopping:
		{
			auto w = runningGameWorld.get();
			runningGameWorld.reset();
			if (auto hierarchy = editorWorld.entityMngr.GetSingleton<Hierarchy>())
				hierarchy->world = &gameWorld;
			if (auto ctrl = editorWorld.entityMngr.GetSingleton<SystemController>())
				ctrl->world = &gameWorld;
			{
				LuaCtxMngr::Instance().Unregister(w);
			}
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
		w->RunEntityJob([&](MeshFilter* meshFilter, const MeshRenderer* meshRenderer) {
			if (!meshFilter->mesh || meshRenderer->materials.empty())
				return;

			RsrcMngrDX12::Instance().RegisterMesh(
				pEditor->uGCmdList.Get(),
				*meshFilter->mesh
			);

			for (const auto& material : meshRenderer->materials) {
				if (!material)
					continue;
				for (const auto& [name, property] : material->properties) {
					if (std::holds_alternative<std::shared_ptr<const Texture2D>>(property)) {
						RsrcMngrDX12::Instance().RegisterTexture2D(
							*std::get<std::shared_ptr<const Texture2D>>(property)
						);
					}
					else if (std::holds_alternative<std::shared_ptr<const TextureCube>>(property)) {
						RsrcMngrDX12::Instance().RegisterTextureCube(
							*std::get<std::shared_ptr<const TextureCube>>(property)
						);
					}
				}
			}
		}, false);

		if (auto skybox = w->entityMngr.GetSingleton<Skybox>(); skybox && skybox->material) {
			for (const auto& [name, property] : skybox->material->properties) {
				if (std::holds_alternative<std::shared_ptr<const Texture2D>>(property)) {
					RsrcMngrDX12::Instance().RegisterTexture2D(
						*std::get<std::shared_ptr<const Texture2D>>(property)
					);
				}
				else if (std::holds_alternative<std::shared_ptr<const TextureCube>>(property)) {
					RsrcMngrDX12::Instance().RegisterTextureCube(
						*std::get<std::shared_ptr<const TextureCube>>(property)
					);
				}
			}
		}
	};
	UpdateRenderResource(&gameWorld);
	UpdateRenderResource(&sceneWorld);

	// commit upload, delete ...
	pEditor->uGCmdList->Close();
	pEditor->uCmdQueue.Execute(pEditor->uGCmdList.Get());
	RsrcMngrDX12::Instance().CommitUploadAndDelete(pEditor->uCmdQueue.Get());

	{
		std::vector<PipelineBase::CameraData> gameCameras;
		Ubpa::UECS::ArchetypeFilter camFilter{ {Ubpa::UECS::CmptAccessType::Of<Camera>} };
		curGameWorld->RunEntityJob([&](Ubpa::UECS::Entity e) {
			gameCameras.emplace_back(e, *curGameWorld);
			}, false, camFilter);
		assert(gameCameras.size() == 1); // now only support 1 camera
		gamePipeline->BeginFrame({ curGameWorld }, gameCameras.front());
	}

	{
		std::vector<PipelineBase::CameraData> sceneCameras;
		Ubpa::UECS::ArchetypeFilter camFilter{ {Ubpa::UECS::CmptAccessType::Of<Camera>} };
		sceneWorld.RunEntityJob([&](Ubpa::UECS::Entity e) {
			sceneCameras.emplace_back(e, sceneWorld);
			}, false, camFilter);
		assert(sceneCameras.size() == 1); // now only support 1 camera
		scenePipeline->BeginFrame({ curGameWorld, &sceneWorld }, sceneCameras.front());
	}

	{
		static bool flag = false;
		if (!flag) {
			OutputDebugStringA(curGameWorld->GenUpdateFrameGraph().Dump().c_str());
			flag = true;
		}
	}
}

void Editor::Impl::Draw() {
	auto cmdAlloc = pEditor->GetCurFrameCommandAllocator();
	ThrowIfFailed(pEditor->uGCmdList->Reset(cmdAlloc, nullptr));

	{ // game
		ImGui::SetCurrentContext(gameImGuiCtx);
		if (gameRT) {
			gamePipeline->Render(gameRT.Get());
			pEditor->uGCmdList.ResourceBarrierTransition(gameRT.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			const auto gameRTHandle = gameRT_RTV.GetCpuHandle();
			pEditor->uGCmdList->OMSetRenderTargets(1, &gameRTHandle, FALSE, NULL);
			pEditor->uGCmdList.SetDescriptorHeaps(Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap());
			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pEditor->uGCmdList.Get());
			pEditor->uGCmdList.ResourceBarrierTransition(gameRT.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		}
		else
			ImGui::EndFrame();
	}

	{ // scene
		ImGui::SetCurrentContext(sceneImGuiCtx);
		if (sceneRT) {
			scenePipeline->Render(sceneRT.Get());
			pEditor->uGCmdList.ResourceBarrierTransition(sceneRT.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			const auto sceneRTHandle = sceneRT_RTV.GetCpuHandle();
			pEditor->uGCmdList->OMSetRenderTargets(1, &sceneRTHandle, FALSE, NULL);
			pEditor->uGCmdList.SetDescriptorHeaps(Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap());
			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pEditor->uGCmdList.Get());
			pEditor->uGCmdList.ResourceBarrierTransition(sceneRT.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		}
		else
			ImGui::EndFrame();
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

	gamePipeline->EndFrame();
	scenePipeline->EndFrame();
	pEditor->GetFrameResourceMngr()->EndFrame(pEditor->uCmdQueue.Get());
	ImGui_ImplWin32_EndFrame();
}

void Editor::Impl::InitInspectorRegistry() {
	InspectorRegistry::Instance().RegisterCmpts <
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

		// transform
		Children,
		LocalToParent,
		LocalToWorld,
		Parent,
		Rotation,
		RotationEuler,
		Scale,
		NonUniformScale,
		Translation,
		WorldToLocal,

		LuaScriptQueue
	> ();
	InspectorRegistry::Instance().RegisterAssets <
		Shader
	>();
	InspectorRegistry::Instance().RegisterAsset(&InspectMaterial);
}

void Editor::Impl::InitWorld(Ubpa::UECS::World& w) {
	auto indices = w.systemMngr.systemTraits.Register<
		// transform
		LocalToParentSystem,
		RotationEulerSystem,
		TRSToLocalToParentSystem,
		TRSToLocalToWorldSystem,
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
	w.systemMngr.systemTraits.Register<LuaScriptQueueSystem>();

	w.entityMngr.cmptTraits.Register<
		// transform
		Children,
		LocalToParent,
		LocalToWorld,
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

		// script
		LuaScriptQueue,

		// editor
		Hierarchy,
		Inspector,
		ProjectViewer,
		SystemController
	>();
}

void Editor::Impl::BuildWorld() {
	Serializer::Instance().RegisterComponents <
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

		// transform
		Children,
		LocalToParent,
		LocalToWorld,
		Parent,
		Rotation,
		RotationEuler,
		Scale,
		NonUniformScale,
		Translation,
		WorldToLocal,

		LuaScriptQueue,

		// editor
		Hierarchy,
		Inspector,
		ProjectViewer
	> ();

	{ // game
		InitWorld(gameWorld);

		//OutputDebugStringA(Serializer::Instance().ToJSON(&gameWorld).c_str());
		auto scene = AssetMngr::Instance().LoadAsset<Scene>(L"..\\assets\\scenes\\Game.scene");
		Serializer::Instance().ToWorld(&gameWorld, scene->GetText());
		{ // input
			gameWorld.entityMngr.Create<Input>();
		}
		OutputDebugStringA(Serializer::Instance().ToJSON(&gameWorld).c_str());

		auto mainLua = LuaCtxMngr::Instance().Register(&gameWorld)->Main();
		sol::state_view solLua(mainLua);
		solLua["world"] = &gameWorld;
	}

	{ // scene
		InitWorld(sceneWorld);
		{ // scene camera
			auto [e, l2w, w2l, cam, t, rot, roamer] = sceneWorld.entityMngr.Create<
				LocalToWorld,
				WorldToLocal,
				Camera,
				Translation,
				Rotation,
				Roamer
			>();
			roamer->reverseFrontBack = true;
			roamer->reverseLeftRight = true;
			roamer->moveSpeed = 1.f;
			roamer->rotateSpeed = 0.1f;
		}

		{ // hierarchy
			auto [e, hierarchy] = sceneWorld.entityMngr.Create<Hierarchy>();
			hierarchy->world = &sceneWorld;
		}
		sceneWorld.entityMngr.Create<WorldTime>();
		sceneWorld.entityMngr.Create<ProjectViewer>();
		sceneWorld.entityMngr.Create<Inspector>();
		sceneWorld.entityMngr.Create<Input>();
	}
	
	{ // editor
		InitWorld(editorWorld);
		{ // hierarchy
			auto [e, hierarchy] = editorWorld.entityMngr.Create<Hierarchy>();
			hierarchy->world = &gameWorld;
		}
		{ // system controller
			auto [e, systemCtrl] = editorWorld.entityMngr.Create<SystemController>();
			systemCtrl->world = &gameWorld;
		}
		editorWorld.entityMngr.Create<Inspector>();
		editorWorld.entityMngr.Create<ProjectViewer>();
		editorWorld.systemMngr.RegisterAndActivate<LoggerSystem, SystemControllerSystem>();
	}
}

void Editor::Impl::LoadTextures() {
	auto tex2dGUIDs = AssetMngr::Instance().FindAssets(std::wregex{ LR"(\.\.\\assets\\_internal\\.*\.tex2d)" });
	for (const auto& guid : tex2dGUIDs) {
		const auto& path = AssetMngr::Instance().GUIDToAssetPath(guid);
		RsrcMngrDX12::Instance().RegisterTexture2D(
			*AssetMngr::Instance().LoadAsset<Texture2D>(path)
		);
	}

	auto texcubeGUIDs = AssetMngr::Instance().FindAssets(std::wregex{ LR"(\.\.\\assets\\_internal\\.*\.texcube)" });
	for (const auto& guid : texcubeGUIDs) {
		const auto& path = AssetMngr::Instance().GUIDToAssetPath(guid);
		RsrcMngrDX12::Instance().RegisterTextureCube(
			*AssetMngr::Instance().LoadAsset<TextureCube>(path)
		);
	}
}

void Editor::Impl::BuildShaders() {
	auto& assetMngr = AssetMngr::Instance();
	auto shaderGUIDs = assetMngr.FindAssets(std::wregex{ LR"(.*\.shader)" });
	for (const auto& guid : shaderGUIDs) {
		const auto& path = assetMngr.GUIDToAssetPath(guid);
		auto shader = assetMngr.LoadAsset<Shader>(path);
		RsrcMngrDX12::Instance().RegisterShader(*shader);
		ShaderMngr::Instance().Register(shader);
	}
}

void Editor::Impl::InspectMaterial(Material* material, InspectorRegistry::InspectContext ctx) {
	ImGui::Text("(*)");
	ImGui::SameLine();
	if (material->shader) {
		if (ImGui::Button(material->shader->name.c_str()))
			ImGui::OpenPopup("Meterial_Shader_Seletor");
	}
	else {
		if (ImGui::Button("nullptr"))
			ImGui::OpenPopup("Meterial_Shader_Seletor");
	}
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PlayloadType::GUID)) {
			IM_ASSERT(payload->DataSize == sizeof(xg::Guid));
			const auto& payload_guid = *(const xg::Guid*)payload->Data;
			const auto& path = AssetMngr::Instance().GUIDToAssetPath(payload_guid);
			assert(!path.empty());
			if (auto shader = AssetMngr::Instance().LoadAsset<Shader>(path)) {
				material->shader = shader;
				material->properties = shader->properties;
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
		ShaderMngr::Instance().Refresh();
		size_t N = ShaderMngr::Instance().GetShaderMap().size();
		for (const auto& [name, shader] : ShaderMngr::Instance().GetShaderMap()) {
			auto shader_s = shader.lock();
			if (shader_s != material->shader && filter.PassFilter(name.c_str())) {
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

	bool changed = false;
	USRefl::TypeInfo<Material>::ForEachVarOf(*material, [ctx, &changed](auto field, auto& var) {
		if (field.name == "shader")
			return;
		if (detail::InspectVar1(field, var, ctx))
			changed = true;
	});
	if (changed) {
		const auto& path = AssetMngr::Instance().GetAssetPath(*material);
		AssetMngr::Instance().ReserializeAsset(path);
	}
}
