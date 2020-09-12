#include "../common/d3dApp.h"

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
#include <DustEngine/Core/ShaderMngr.h>

#include <UDX12/UploadBuffer.h>

#include <UGM/UGM.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

const int gNumFrameResources = 3;

struct RotateSystem : Ubpa::UECS::System {
	using Ubpa::UECS::System::System;
	virtual void OnUpdate(Ubpa::UECS::Schedule& schedule) override {
		Ubpa::UECS::ArchetypeFilter filter;
		filter.all = { Ubpa::UECS::CmptAccessType::Of<Ubpa::DustEngine::MeshFilter> };
		schedule.RegisterEntityJob([](Ubpa::DustEngine::Rotation* rot, Ubpa::DustEngine::Translation* trans) {
			rot->value = rot->value * Ubpa::quatf{ Ubpa::vecf3{0,1,0}, Ubpa::to_radian(2.f) };
			trans->value += 0.2f * (Ubpa::vecf3{ Ubpa::rand01<float>(), Ubpa::rand01<float>(), Ubpa::rand01<float>() } - Ubpa::vecf3{ 0.5f });
		}, "rotate", true, filter);
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

	Ubpa::DustEngine::Texture2D* albedoTex2D;
	Ubpa::DustEngine::Texture2D* roughnessTex2D;
	Ubpa::DustEngine::Texture2D* metalnessTex2D;

	Ubpa::UECS::World world;
	Ubpa::UECS::Entity cam{ Ubpa::UECS::Entity::Invalid() };

	std::unique_ptr<Ubpa::DustEngine::IPipeline> pipeline;
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
        WorldApp theApp(hInstance);
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

WorldApp::WorldApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

WorldApp::~WorldApp()
{
    if(!uDevice.IsNull())
        FlushCommandQueue();
}

bool WorldApp::Initialize() {
	if (!InitMainWindow())
		return false;

	if (!InitDirect3D())
		return false;

	Ubpa::DustEngine::RsrcMngrDX12::Instance().Init(uDevice.raw.Get());

	Ubpa::UDX12::DescriptorHeapMngr::Instance().Init(uDevice.raw.Get(), 1024, 1024, 1024, 1024, 1024);

	Ubpa::DustEngine::MeshLayoutMngr::Instance().Init();

	Ubpa::DustEngine::AssetMngr::Instance().ImportAssetRecursively(L"..\\assets");

	BuildWorld();

	ThrowIfFailed(uGCmdList->Reset(mDirectCmdListAlloc.Get(), nullptr));
	auto& upload = Ubpa::DustEngine::RsrcMngrDX12::Instance().GetUpload();
	auto& deleteBatch = Ubpa::DustEngine::RsrcMngrDX12::Instance().GetDeleteBatch();

	upload.Begin();

	LoadTextures();
	BuildShaders();
	BuildMaterials();

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
	upload.End(uCmdQueue.raw.Get());
	uGCmdList->Close();
	uCmdQueue.Execute(uGCmdList.raw.Get());
	deleteBatch.Commit(uDevice.raw.Get(), uCmdQueue.raw.Get());

	Ubpa::DustEngine::IPipeline::InitDesc initDesc;
	initDesc.device = uDevice.raw.Get();
	initDesc.rtFormat = mBackBufferFormat;
	initDesc.cmdQueue = uCmdQueue.raw.Get();
	initDesc.numFrame = gNumFrameResources;
	pipeline = std::make_unique<Ubpa::DustEngine::StdPipeline>(initDesc);

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
    OnKeyboardInput();
	UpdateCamera();

	world.Update();

	std::vector<Ubpa::DustEngine::IPipeline::CameraData> gameCameras;
	Ubpa::UECS::ArchetypeFilter camFilter{ {Ubpa::UECS::CmptAccessType::Of<Ubpa::DustEngine::Camera>} };
	world.RunEntityJob([&](Ubpa::UECS::Entity e) {
		gameCameras.emplace_back(e, world);
		}, false, camFilter);
	pipeline->BeginFrame(world, gameCameras);
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

void WorldApp::OnKeyboardInput()
{
}
 
void WorldApp::UpdateCamera()
{
	Ubpa::vecf3 eye = {
		mRadius * sinf(mTheta) * sinf(mPhi),
		mRadius * cosf(mTheta),
		mRadius * sinf(mTheta) * cosf(mPhi)
	};
	auto camera = world.entityMngr.Get<Ubpa::DustEngine::Camera>(cam);
	camera->fov = 60.f;
	camera->aspect = AspectRatio();
	camera->clippingPlaneMin = 1.0f;
	camera->clippingPlaneMax = 1000.0f;
	auto view = Ubpa::transformf::look_at(eye.as<Ubpa::pointf3>(), { 0.f }); // world to camera
	auto c2w = view.inverse();
	world.entityMngr.Get<Ubpa::DustEngine::Translation>(cam)->value = eye;
	world.entityMngr.Get<Ubpa::DustEngine::Rotation>(cam)->value = c2w.decompose_quatenion();
}

void WorldApp::BuildWorld() {
	world.systemMngr.Register<
		Ubpa::DustEngine::CameraSystem,
		Ubpa::DustEngine::LocalToParentSystem,
		Ubpa::DustEngine::RotationEulerSystem,
		Ubpa::DustEngine::TRSToLocalToParentSystem,
		Ubpa::DustEngine::TRSToLocalToWorldSystem,
		Ubpa::DustEngine::WorldToLocalSystem,
		RotateSystem
	>();

	auto e0 = world.entityMngr.Create<
		Ubpa::DustEngine::LocalToWorld,
		Ubpa::DustEngine::WorldToLocal,
		Ubpa::DustEngine::Camera,
		Ubpa::DustEngine::Translation,
		Ubpa::DustEngine::Rotation
	>();
	cam = std::get<Ubpa::UECS::Entity>(e0);

	int num = 11;
	auto cubeMesh = Ubpa::DustEngine::AssetMngr::Instance().LoadAsset<Ubpa::DustEngine::Mesh>(L"..\\assets\\models\\cube.obj");
	for (int i = 0; i < num; i++) {
		for (int j = 0; j < num; j++) {
			auto cube = world.entityMngr.Create<
				Ubpa::DustEngine::LocalToWorld,
				Ubpa::DustEngine::MeshFilter,
				Ubpa::DustEngine::MeshRenderer,
				Ubpa::DustEngine::Translation,
				Ubpa::DustEngine::Rotation,
				Ubpa::DustEngine::Scale
			>();
			auto t = std::get<Ubpa::DustEngine::Translation*>(cube);
			auto s = std::get<Ubpa::DustEngine::Scale*>(cube);
			s->value = 0.2f;
			t->value = {
				0.5f * (i - num / 2),
				0.5f * (j - num / 2),
				0
			};
			std::get<Ubpa::DustEngine::MeshFilter*>(cube)->mesh = cubeMesh;
		}
	}
}

void WorldApp::LoadTextures() {
	auto tex2dGUIDs = Ubpa::DustEngine::AssetMngr::Instance().FindAssets(std::wregex{ LR"(.*\.tex2d)" });
	for (const auto& guid : tex2dGUIDs) {
		const auto& path = Ubpa::DustEngine::AssetMngr::Instance().GUIDToAssetPath(guid);
		Ubpa::DustEngine::RsrcMngrDX12::Instance().RegisterTexture2D(
			Ubpa::DustEngine::RsrcMngrDX12::Instance().GetUpload(),
			Ubpa::DustEngine::AssetMngr::Instance().LoadAsset<Ubpa::DustEngine::Texture2D>(path)
		);
	}
}

void WorldApp::BuildShaders() {
	auto& assetMngr = Ubpa::DustEngine::AssetMngr::Instance();
	auto shaderGUIDs = assetMngr.FindAssets(std::wregex{ LR"(.*\.shader)" });
	for (const auto& guid : shaderGUIDs) {
		const auto& path = assetMngr.GUIDToAssetPath(guid);
		auto shader = assetMngr.LoadAsset<Ubpa::DustEngine::Shader>(path);
		Ubpa::DustEngine::RsrcMngrDX12::Instance().RegisterShader(shader);
		Ubpa::DustEngine::ShaderMngr::Instance().Register(shader);
	}
}

void WorldApp::BuildMaterials() {
	auto material = Ubpa::DustEngine::AssetMngr::Instance()
		.LoadAsset<Ubpa::DustEngine::Material>(L"..\\assets\\materials\\iron.mat");
	world.RunEntityJob([=](Ubpa::DustEngine::MeshRenderer* meshRenderer) {
		meshRenderer->materials.push_back(material);
	});
}
