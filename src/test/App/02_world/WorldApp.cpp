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

#ifndef NDEBUG
#include <dxgidebug.h>
#endif

using Microsoft::WRL::ComPtr;
using namespace DirectX;

const int gNumFrameResources = 3;

struct RotateSystem {
	static void OnUpdate(Ubpa::UECS::Schedule& schedule) {
		Ubpa::UECS::ArchetypeFilter filter;
		filter.all = { Ubpa::UECS::CmptAccessType::Of<Ubpa::Utopia::MeshFilter> };
		schedule.RegisterEntityJob(
			[](Ubpa::Utopia::Rotation* rot, Ubpa::Utopia::Translation* trans) {
				rot->value = rot->value * Ubpa::quatf{ Ubpa::vecf3{0,1,0}, Ubpa::to_radian(2.f) };
				trans->value += 0.2f * (Ubpa::vecf3{ Ubpa::rand01<float>(), Ubpa::rand01<float>(), Ubpa::rand01<float>() }
					- Ubpa::vecf3{ 0.5f });
			},
			"rotate",
			true,
			filter
		);
	}
};

class WorldApp : public D3DApp
{
public:
    WorldApp(HINSTANCE hInstance);
    WorldApp(const WorldApp& rhs) = delete;
    WorldApp& operator=(const WorldApp& rhs) = delete;
    ~WorldApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update()override;
    virtual void Draw()override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

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

	Ubpa::Utopia::Texture2D* albedoTex2D;
	Ubpa::Utopia::Texture2D* roughnessTex2D;
	Ubpa::Utopia::Texture2D* metalnessTex2D;

	Ubpa::UECS::World world;
	Ubpa::UECS::Entity cam{ Ubpa::UECS::Entity::Invalid() };

	std::unique_ptr<Ubpa::Utopia::PipelineBase> pipeline;

	std::unique_ptr<Ubpa::UDX12::FrameResourceMngr> frameRsrcMngr;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	int rst;

    try
    {
        WorldApp theApp(hInstance);
        if(!theApp.Initialize())
            return 0;

        rst = theApp.Run();
    }
    catch(Ubpa::UDX12::Util::Exception& e)
    {
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		rst = 1;
    }

#ifndef NDEBUG
	Microsoft::WRL::ComPtr<IDXGIDebug> debug;
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug));
	if (debug)
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
#endif

	return 1;
}

WorldApp::WorldApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

WorldApp::~WorldApp()
{
	Ubpa::Utopia::RsrcMngrDX12::Instance().Clear(uCmdQueue.Get());
    if(!uDevice.IsNull())
		FlushCommandQueue();
}

bool WorldApp::Initialize() {
	if (!InitMainWindow())
		return false;

	if (!InitDirect3D())
		return false;

	Ubpa::Utopia::RsrcMngrDX12::Instance().Init(uDevice.raw.Get());

	Ubpa::UDX12::DescriptorHeapMngr::Instance().Init(uDevice.raw.Get(), 1024, 1024, 1024, 1024, 1024);

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

	ThrowIfFailed(uGCmdList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	LoadTextures();
	BuildShaders();
	BuildMaterials();

	// update mesh
	world.RunEntityJob([&](Ubpa::Utopia::MeshFilter* meshFilter) {
		Ubpa::Utopia::RsrcMngrDX12::Instance().RegisterMesh(
			uGCmdList.Get(),
			*meshFilter->mesh
		);
	}, false);

	Ubpa::Utopia::PipelineBase::InitDesc initDesc;
	initDesc.device = uDevice.raw.Get();
	initDesc.rtFormat = mBackBufferFormat;
	initDesc.cmdQueue = uCmdQueue.raw.Get();
	initDesc.numFrame = gNumFrameResources;
	pipeline = std::make_unique<Ubpa::Utopia::StdPipeline>(initDesc);

	// commit upload, delete ...
	uGCmdList->Close();
	uCmdQueue.Execute(uGCmdList.raw.Get());

	Ubpa::Utopia::RsrcMngrDX12::Instance().CommitUploadAndDelete(uCmdQueue.raw.Get());

	// Do the initial resize code.
	OnResize();

	// Wait until initialization is complete.
	FlushCommandQueue();

    return true;
}
 
void WorldApp::OnResize() {
    D3DApp::OnResize();

	if (pipeline)
		pipeline->Resize(mClientWidth, mClientHeight, mScreenViewport, mScissorRect);
}

void WorldApp::Update()
{
	UpdateCamera();

	world.Update();

	// update mesh, texture ...
	frameRsrcMngr->BeginFrame();

	auto cmdAlloc = frameRsrcMngr->GetCurrentFrameResource()->GetResource<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>("CommandAllocator");
	cmdAlloc->Reset();
	ThrowIfFailed(uGCmdList->Reset(cmdAlloc.Get(), nullptr));

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
	frameRsrcMngr->EndFrame(uCmdQueue.raw.Get());

	std::vector<Ubpa::Utopia::PipelineBase::CameraData> gameCameras;
	Ubpa::UECS::ArchetypeFilter camFilter{ {Ubpa::UECS::CmptAccessType::Of<Ubpa::Utopia::Camera>} };
	world.RunEntityJob([&](Ubpa::UECS::Entity e) {
		gameCameras.emplace_back(e, world);
	}, false, camFilter);
	assert(gameCameras.size() == 1); // now only support 1 camera
	pipeline->BeginFrame({ &world }, gameCameras.front());
}

void WorldApp::Draw()
{
	pipeline->Render(CurrentBackBuffer());
    // Swap the back and front buffers
    ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
	pipeline->EndFrame();
}

void WorldApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void WorldApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void WorldApp::OnMouseMove(WPARAM btnState, int x, int y)
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
 
void WorldApp::UpdateCamera()
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

void WorldApp::BuildWorld() {
	auto systemIDs = world.systemMngr.systemTraits.Register<
		Ubpa::Utopia::CameraSystem,
		Ubpa::Utopia::LocalToParentSystem,
		Ubpa::Utopia::RotationEulerSystem,
		Ubpa::Utopia::TRSToLocalToParentSystem,
		Ubpa::Utopia::TRSToLocalToWorldSystem,
		Ubpa::Utopia::WorldToLocalSystem,
		RotateSystem
	>();
	for (auto ID : systemIDs)
		world.systemMngr.Activate(ID);

	{ // skybox
		auto [e, skybox] = world.entityMngr.Create<Ubpa::Utopia::Skybox>();
		const auto& path = Ubpa::Utopia::AssetMngr::Instance().GUIDToAssetPath(xg::Guid{ "bba13c3e-87d1-463a-974b-324d997349e3" });
		skybox->material = Ubpa::Utopia::AssetMngr::Instance().LoadAsset<Ubpa::Utopia::Material>(path);
	}

	auto e0 = world.entityMngr.Create<
		Ubpa::Utopia::LocalToWorld,
		Ubpa::Utopia::WorldToLocal,
		Ubpa::Utopia::Camera,
		Ubpa::Utopia::Translation,
		Ubpa::Utopia::Rotation
	>();
	cam = std::get<Ubpa::UECS::Entity>(e0);

	int num = 11;
	auto cubeMesh = Ubpa::Utopia::AssetMngr::Instance().LoadAsset<Ubpa::Utopia::Mesh>(L"..\\assets\\models\\cube.obj");
	for (int i = 0; i < num; i++) {
		for (int j = 0; j < num; j++) {
			auto cube = world.entityMngr.Create<
				Ubpa::Utopia::LocalToWorld,
				Ubpa::Utopia::MeshFilter,
				Ubpa::Utopia::MeshRenderer,
				Ubpa::Utopia::Translation,
				Ubpa::Utopia::Rotation,
				Ubpa::Utopia::Scale
			>();
			auto t = std::get<Ubpa::Utopia::Translation*>(cube);
			auto s = std::get<Ubpa::Utopia::Scale*>(cube);
			s->value = 0.2f;
			t->value = {
				0.5f * (i - num / 2),
				0.5f * (j - num / 2),
				0
			};
			std::get<Ubpa::Utopia::MeshFilter*>(cube)->mesh = cubeMesh;
		}
	}
}

void WorldApp::LoadTextures() {
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

void WorldApp::BuildShaders() {
	auto& assetMngr = Ubpa::Utopia::AssetMngr::Instance();
	auto shaderGUIDs = assetMngr.FindAssets(std::wregex{ LR"(.*\.shader)" });
	for (const auto& guid : shaderGUIDs) {
		const auto& path = assetMngr.GUIDToAssetPath(guid);
		auto shader = assetMngr.LoadAsset<Ubpa::Utopia::Shader>(path);
		Ubpa::Utopia::RsrcMngrDX12::Instance().RegisterShader(*shader);
		Ubpa::Utopia::ShaderMngr::Instance().Register(shader);
	}
}

void WorldApp::BuildMaterials() {
	auto material = Ubpa::Utopia::AssetMngr::Instance()
		.LoadAsset<Ubpa::Utopia::Material>(L"..\\assets\\materials\\iron.mat");
	world.RunEntityJob([=](Ubpa::Utopia::MeshRenderer* meshRenderer) {
		meshRenderer->materials.push_back(material);
	});
}
