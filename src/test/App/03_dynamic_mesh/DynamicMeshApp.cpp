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
				if(!meshFilter->mesh)
					return;
				if (time->elapsedTime < 5.f) {
					if (meshFilter->mesh->IsEditable()) {
						auto positions = meshFilter->mesh->GetPositions();
						for (auto& pos : positions)
							pos[1] = 0.2f * (Ubpa::rand01<float>() - 0.5f);
						meshFilter->mesh->SetPositions(positions);
					}
				}
				else if (5.f < time->elapsedTime && time->elapsedTime < 7.f)
					meshFilter->mesh->SetToNonEditable();
				else if (7.f < time->elapsedTime && time->elapsedTime < 9.f) {
					meshFilter->mesh->SetToEditable();
					auto positions = meshFilter->mesh->GetPositions();
					for (auto& pos : positions)
						pos[1] = 0.2f * (Ubpa::rand01<float>() - 0.5f);
					meshFilter->mesh->SetPositions(positions);
				}
				else
					meshFilter->mesh->SetToNonEditable();
			},
			"AnimateMesh"
		);
		schedule.RegisterEntityJob(
			[](
				Ubpa::Utopia::MeshFilter* meshFilter,
				Ubpa::UECS::Latest<Ubpa::UECS::Singleton<Ubpa::Utopia::WorldTime>> time
			) {
				if(!meshFilter->mesh)
					return;
				if (time->elapsedTime > 10.f) {
					Ubpa::Utopia::RsrcMngrDX12::Instance().UnregisterMesh(*meshFilter->mesh);
					meshFilter->mesh = nullptr;
				}
			},
			"DeleteMesh"
		);
		schedule.RegisterCommand([](Ubpa::UECS::World* w) {
			auto time = w->entityMngr.GetSingleton<Ubpa::Utopia::WorldTime>();
			if (!time)
				return;

			if (time->elapsedTime < 12.f)
				return;

			w->systemMngr.Deactivate<AnimateMeshSystem>();
		});
	}
};

class DynamicMeshApp : public D3DApp
{
public:
    DynamicMeshApp(HINSTANCE hInstance);
    DynamicMeshApp(const DynamicMeshApp& rhs) = delete;
    DynamicMeshApp& operator=(const DynamicMeshApp& rhs) = delete;
    ~DynamicMeshApp();

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

	Ubpa::UECS::World world;
	Ubpa::UECS::Entity cam{ Ubpa::UECS::Entity::Invalid() };

	std::unique_ptr<Ubpa::Utopia::PipelineBase> pipeline;
	std::shared_ptr<Ubpa::Utopia::Mesh> dynamicMesh;

	std::unique_ptr<Ubpa::UDX12::FrameResourceMngr> frameRsrcMngr;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        DynamicMeshApp theApp(hInstance);
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

DynamicMeshApp::DynamicMeshApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

DynamicMeshApp::~DynamicMeshApp()
{
	Ubpa::Utopia::RsrcMngrDX12::Instance().Clear(uCmdQueue.Get());
    if(!uDevice.IsNull())
		FlushCommandQueue();
}

bool DynamicMeshApp::Initialize()
{
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
 
void DynamicMeshApp::OnResize()
{
    D3DApp::OnResize();

	if (pipeline)
		pipeline->Resize(mClientWidth, mClientHeight, mScreenViewport, mScissorRect);
}

void DynamicMeshApp::Update() {
	UpdateCamera();

	world.Update();

	// update mesh, texture ...
	frameRsrcMngr->BeginFrame();

	auto cmdAlloc = frameRsrcMngr->GetCurrentFrameResource()->GetResource<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>("CommandAllocator");
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
	frameRsrcMngr->EndFrame(uCmdQueue.raw.Get());

	std::vector<Ubpa::Utopia::PipelineBase::CameraData> gameCameras;
	Ubpa::UECS::ArchetypeFilter camFilter{ {Ubpa::UECS::CmptAccessType::Of<Ubpa::Utopia::Camera>} };
	world.RunEntityJob([&](Ubpa::UECS::Entity e) {
		gameCameras.emplace_back(e, world);
	}, false, camFilter);
	assert(gameCameras.size() == 1); // now only support 1 camera
	pipeline->BeginFrame({ &world }, gameCameras.front());

}

void DynamicMeshApp::Draw()
{
	pipeline->Render(CurrentBackBuffer());
    // Swap the back and front buffers
    ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
	pipeline->EndFrame();
}

void DynamicMeshApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void DynamicMeshApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void DynamicMeshApp::OnMouseMove(WPARAM btnState, int x, int y)
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
 
void DynamicMeshApp::UpdateCamera()
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

void DynamicMeshApp::BuildWorld() {
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

void DynamicMeshApp::LoadTextures() {
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

void DynamicMeshApp::BuildShaders() {
	auto& assetMngr = Ubpa::Utopia::AssetMngr::Instance();
	auto shaderGUIDs = assetMngr.FindAssets(std::wregex{ LR"(.*\.shader)" });
	for (const auto& guid : shaderGUIDs) {
		const auto& path = assetMngr.GUIDToAssetPath(guid);
		auto shader = assetMngr.LoadAsset<Ubpa::Utopia::Shader>(path);
		Ubpa::Utopia::RsrcMngrDX12::Instance().RegisterShader(*shader);
		Ubpa::Utopia::ShaderMngr::Instance().Register(shader);
	}
}

void DynamicMeshApp::BuildMaterials() {
	auto material = Ubpa::Utopia::AssetMngr::Instance()
		.LoadAsset<Ubpa::Utopia::Material>(L"..\\assets\\materials\\iron.mat");
	world.RunEntityJob([=](Ubpa::Utopia::MeshRenderer* meshRenderer) {
		meshRenderer->materials.push_back(material);
	});
}
