#include "../common/d3dApp.h"
#include "../common/MathHelper.h"
#include "../common/GeometryGenerator.h"

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
		filter.all = { Ubpa::UECS::CmptType::Of<Ubpa::DustEngine::MeshFilter> };
		schedule.RegisterEntityJob([](Ubpa::DustEngine::Rotation* rot) {
			rot->value = rot->value * Ubpa::quatf{ Ubpa::vecf3{0,1,0}, Ubpa::to_radian(2.f) };
		}, "rotate", filter);
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
    void BuildShapeGeometry();
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

bool WorldApp::Initialize()
{
	if (!InitMainWindow())
		return false;

	if (!InitDirect3D())
		return false;

	Ubpa::DustEngine::RsrcMngrDX12::Instance().Init(uDevice.raw.Get());

	Ubpa::UDX12::DescriptorHeapMngr::Instance().Init(uDevice.raw.Get(), 1024, 1024, 1024, 1024, 1024);

	Ubpa::DustEngine::IPipeline::InitDesc initDesc;
	initDesc.device = uDevice.raw.Get();
	initDesc.backBufferFormat = mBackBufferFormat;
	initDesc.depthStencilFormat = mDepthStencilFormat;
	initDesc.cmdQueue = uCmdQueue.raw.Get();
	initDesc.numFrame = gNumFrameResources;
	pipeline = std::make_unique<Ubpa::DustEngine::StdPipeline>(initDesc);

	Ubpa::DustEngine::MeshLayoutMngr::Instance().Init();

	// Do the initial resize code.
	OnResize();

	BuildWorld();

	Ubpa::DustEngine::RsrcMngrDX12::Instance().GetUpload().Begin();
	LoadTextures();
    BuildShapeGeometry();
	BuildMaterials();
	Ubpa::DustEngine::RsrcMngrDX12::Instance().GetUpload().End(uCmdQueue.raw.Get());

    // Wait until initialization is complete.
    FlushCommandQueue();

    return true;
}
 
void WorldApp::OnResize()
{
    D3DApp::OnResize();

	if (pipeline)
		pipeline->Resize(mClientWidth, mClientHeight, mScreenViewport, mScissorRect, mDepthStencilBuffer.Get());
}

void WorldApp::Update()
{
    OnKeyboardInput();
	UpdateCamera();

	world.Update();
	pipeline->UpdateRenderContext(world);
}

void WorldApp::Draw()
{
	pipeline->BeginFrame(CurrentBackBuffer());
	pipeline->Render();
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
		mTheta = MathHelper::Clamp(mTheta, 0.1f, MathHelper::Pi - 0.1f);
    }
    else if((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.2 unit in the scene.
        float dx = 0.05f*static_cast<float>(x - mLastMousePos.x);
        float dy = 0.05f*static_cast<float>(y - mLastMousePos.y);

        // Update the camera radius based on input.
        mRadius += dx - dy;

        // Restrict the radius.
        mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
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
	camera->fov = 0.33f * Ubpa::PI<float>;
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
		}
	}
}

void WorldApp::LoadTextures() {
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

void WorldApp::BuildShapeGeometry() {
	auto mesh = Ubpa::DustEngine::AssetMngr::Instance().LoadAsset<Ubpa::DustEngine::Mesh>("../assets/models/cube.obj");
	Ubpa::DustEngine::RsrcMngrDX12::Instance().RegisterMesh(
		Ubpa::DustEngine::RsrcMngrDX12::Instance().GetUpload(),
		Ubpa::DustEngine::RsrcMngrDX12::Instance().GetDeleteBatch(),
		uGCmdList.raw.Get(),
		mesh
	);
	Ubpa::UECS::ArchetypeFilter filter;
	filter.all = { Ubpa::UECS::CmptType::Of<Ubpa::DustEngine::MeshFilter> };
	auto meshFilters = world.entityMngr.GetCmptArray<Ubpa::DustEngine::MeshFilter>(filter);
	for (auto meshFilter : meshFilters)
		meshFilter->mesh = mesh;
}

void WorldApp::BuildMaterials() {
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
	filter.all = { Ubpa::UECS::CmptType::Of<Ubpa::DustEngine::MeshRenderer> };
	auto meshRenderers = world.entityMngr.GetCmptArray<Ubpa::DustEngine::MeshRenderer>(filter);
	for (auto meshRenderer : meshRenderers)
		meshRenderer->material.push_back(material);
}
