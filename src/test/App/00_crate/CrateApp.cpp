#include "../common/d3dApp.h"
#include "../common/MathHelper.h"
#include "../common/GeometryGenerator.h"

#include <DustEngine/Asset/AssetMngr.h>

#include <DustEngine/Core/Texture2D.h>
#include <DustEngine/Core/Image.h>
#include <DustEngine/Core/HLSLFile.h>
#include <DustEngine/Core/Shader.h>
#include <DustEngine/Core/Mesh.h>

#include <UDX12/UploadBuffer.h>

#include <UGM/UGM.h>
#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

const int gNumFrameResources = 3;
constexpr size_t ID_RootSignature_default = 0;

struct ObjectConstants
{
	Ubpa::transformf World = Ubpa::transformf::eye();
	Ubpa::transformf TexTransform = Ubpa::transformf::eye();
};

struct PassConstants
{
	Ubpa::transformf View = Ubpa::transformf::eye();
	Ubpa::transformf InvView = Ubpa::transformf::eye();
	Ubpa::transformf Proj = Ubpa::transformf::eye();
	Ubpa::transformf InvProj = Ubpa::transformf::eye();
	Ubpa::transformf ViewProj = Ubpa::transformf::eye();
	Ubpa::transformf InvViewProj = Ubpa::transformf::eye();
	Ubpa::pointf3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;
	Ubpa::valf2 RenderTargetSize = { 0.0f, 0.0f };
	Ubpa::valf2 InvRenderTargetSize = { 0.0f, 0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

	Ubpa::vecf4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	// Indices [0, NUM_DIR_LIGHTS) are directional lights;
	// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
	// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
	// are spot lights for a maximum of MaxLights per object.
	Light Lights[MaxLights];
};

struct Vertex
{
	Ubpa::pointf3 Pos;
	Ubpa::normalf Normal;
	Ubpa::pointf2 TexC;
};

// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
struct RenderItem
{
	RenderItem() = default;

    // World matrix of the shape that describes the object's local space
    // relative to the world space, which defines the position, orientation,
    // and scale of the object in the world.
    Ubpa::transformf World = Ubpa::transformf::eye();

	Ubpa::transformf TexTransform = Ubpa::transformf::eye();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	Ubpa::UDX12::MeshGPUBuffer* Geo = nullptr;
	//std::string Geo;

    // Primitive topology.
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // DrawIndexedInstanced parameters.
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
};

class DeferApp : public D3DApp
{
public:
    DeferApp(HINSTANCE hInstance);
    DeferApp(const DeferApp& rhs) = delete;
    DeferApp& operator=(const DeferApp& rhs) = delete;
    ~DeferApp();

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
	void AnimateMaterials();
	void UpdateObjectCBs();
	void UpdateMaterialCBs();
	void UpdateMainPassCB();

	void LoadTextures();
    void BuildRootSignature();
	void BuildDescriptorHeaps();
    void BuildShadersAndInputLayout();
    void BuildShapeGeometry();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildMaterials();
    void BuildRenderItems();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

private:
    //UINT mCbvSrvDescriptorSize = 0;

    //ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	//ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
	Ubpa::UDX12::DescriptorHeapAllocation mSrvDescriptorHeap;

	//std::unordered_map<std::string, std::unique_ptr<Ubpa::UDX12::MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	//std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    //ComPtr<ID3D12PipelineState> mOpaquePSO = nullptr;
 
	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mOpaqueRitems;

    PassConstants mMainPassCB;

	Ubpa::pointf3 mEyePos = { 0.0f, 0.0f, 0.0f };
	Ubpa::transformf mView = Ubpa::transformf::eye();
	Ubpa::transformf mProj = Ubpa::transformf::eye();

	float mTheta = 0.5f * XM_PI;// 1.3f * XM_PI;
	float mPhi = 0.f;// 0.4f * XM_PI;
	float mRadius = 10.0f;

    POINT mLastMousePos;

	// frame graph
	//Ubpa::UDX12::FG::RsrcMngr fgRsrcMngr;
	Ubpa::UDX12::FG::Executor fgExecutor;
	Ubpa::UFG::Compiler fgCompiler;
	Ubpa::UFG::FrameGraph fg;

	// resources
	Ubpa::DustEngine::Texture2D* chessboardTex2D;
	Ubpa::DustEngine::Shader* shader;
	Ubpa::DustEngine::Mesh* mesh;

	std::unique_ptr<Ubpa::UDX12::FrameResourceMngr> frameRsrcMngr;

	size_t ID_PSO_opaque;
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
        DeferApp theApp(hInstance);
        if(!theApp.Initialize())
            return 0;

        int rst = theApp.Run();
		Ubpa::DustEngine::RsrcMngrDX12::Instance().Clear();
		return rst;
    }
    catch(Ubpa::UDX12::Util::Exception& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }

}

DeferApp::DeferApp(HINSTANCE hInstance)
	: D3DApp(hInstance), fg{"frame graph"}
{
}

DeferApp::~DeferApp()
{
    if(!uDevice.IsNull())
        FlushCommandQueue();
}

bool DeferApp::Initialize()
{
    if(!D3DApp::Initialize())
        return false;

	frameRsrcMngr = std::make_unique<Ubpa::UDX12::FrameResourceMngr>(gNumFrameResources, uDevice.raw.Get());

	Ubpa::DustEngine::RsrcMngrDX12::Instance().Init(uDevice.raw.Get());

	Ubpa::UDX12::DescriptorHeapMngr::Instance().Init(uDevice.raw.Get(), 1024, 1024, 1024, 1024, 1024);

    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(uGCmdList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    // Get the increment size of a descriptor in this heap type.  This is hardware specific, 
	// so we have to query this information.
    //mCbvSrvDescriptorSize = uDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	Ubpa::DustEngine::RsrcMngrDX12::Instance().GetUpload().Begin();
 
	LoadTextures();
    BuildRootSignature();
	BuildDescriptorHeaps();
    BuildShadersAndInputLayout();
    BuildShapeGeometry();
	BuildMaterials();
    BuildRenderItems();
    BuildFrameResources();
    BuildPSOs();

    // Execute the initialization commands.
    ThrowIfFailed(uGCmdList->Close());
	uCmdQueue.Execute(uGCmdList.raw.Get());

	Ubpa::DustEngine::RsrcMngrDX12::Instance().GetUpload().End(uCmdQueue.raw.Get());

    // Wait until initialization is complete.
    FlushCommandQueue();

    return true;
}

void DeferApp::OnResize()
{
    D3DApp::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix
	mProj = Ubpa::transformf::perspective(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f, 0.f);

	auto clearFGRsrcMngr = [](std::shared_ptr<Ubpa::UDX12::FG::RsrcMngr> rsrcMngr) {
		rsrcMngr->Clear();
	};

	if (frameRsrcMngr) {
		for (auto& frsrc : frameRsrcMngr->GetFrameResources())
			frsrc->DelayUpdateResource("FrameGraphRsrcMngr", clearFGRsrcMngr);
	}
}

void DeferApp::Update()
{
    OnKeyboardInput();
	UpdateCamera();

	// Cycle through the circular frame resource array.
	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	frameRsrcMngr->BeginFrame();

	AnimateMaterials();
	UpdateObjectCBs();
	UpdateMaterialCBs();
	UpdateMainPassCB();
}

void DeferApp::Draw()
{
	auto cmdAlloc = frameRsrcMngr->GetCurrentFrameResource()->GetResource<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>("CommandAllocator");

    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(cmdAlloc->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    //ThrowIfFailed(uGCmdList->Reset(cmdAlloc.Get(), Ubpa::DustEngine::RsrcMngrDX12::Instance().GetPSO(ID_PSO_opaque)));

	/*uGCmdList.SetDescriptorHeaps(Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap());
	uGCmdList->RSSetViewports(1, &mScreenViewport);
	uGCmdList->RSSetScissorRects(1, &mScissorRect);*/

	fg.Clear();
	auto fgRsrcMngr = frameRsrcMngr->GetCurrentFrameResource()->GetResource<std::shared_ptr<Ubpa::UDX12::FG::RsrcMngr>>("FrameGraphRsrcMngr");
	fgRsrcMngr->NewFrame();
	fgExecutor.NewFrame();;

	auto backbuffer = fg.RegisterResourceNode("Back Buffer");
	auto depthstencil = fg.RegisterResourceNode("Depth Stencil");
	auto pass = fg.RegisterPassNode(
		"Pass",
		{},
		{ backbuffer,depthstencil }
	);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = mDepthStencilFormat;
	dsvDesc.Texture2D.MipSlice = 0;

	(*fgRsrcMngr)
		.RegisterImportedRsrc(backbuffer, { CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT })
		.RegisterImportedRsrc(depthstencil, { mDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE })
		.RegisterPassRsrcs(pass, backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET,
			Ubpa::UDX12::FG::RsrcImplDesc_RTV_Null{})
		.RegisterPassRsrcs(pass, depthstencil, D3D12_RESOURCE_STATE_DEPTH_WRITE, dsvDesc);

	fgExecutor.RegisterPassFunc(
		pass,
		[&](ID3D12GraphicsCommandList* cmdList, const Ubpa::UDX12::FG::PassRsrcs& rsrcs) {
			auto heap = Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);
			cmdList->RSSetViewports(1, &mScreenViewport);
			cmdList->RSSetScissorRects(1, &mScissorRect);

			auto bb = rsrcs.find(backbuffer)->second;
			auto ds = rsrcs.find(depthstencil)->second;

			cmdList->SetPipelineState(Ubpa::DustEngine::RsrcMngrDX12::Instance().GetPSO(ID_PSO_opaque));

			// Clear the back buffer and depth buffer.
			cmdList->ClearRenderTargetView(bb.cpuHandle, Colors::LightSteelBlue, 0, nullptr);
			cmdList->ClearDepthStencilView(ds.cpuHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

			// Specify the buffers we are going to render to.
			std::array rts{ bb.cpuHandle };
			cmdList->OMSetRenderTargets(rts.size(), rts.data(), false, &ds.cpuHandle);

			//uGCmdList.SetDescriptorHeaps(mSrvDescriptorHeap.Get());

			cmdList->SetGraphicsRootSignature(Ubpa::DustEngine::RsrcMngrDX12::Instance().GetRootSignature(ID_RootSignature_default));

			auto passCB = frameRsrcMngr->GetCurrentFrameResource()
				->GetResource<Ubpa::UDX12::ArrayUploadBuffer<PassConstants>>("ArrayUploadBuffer<PassConstants>")
				.GetResource();
			cmdList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

			DrawRenderItems(cmdList, mOpaqueRitems);
		}
	);

	auto [success, crst] = fgCompiler.Compile(fg);
	fgExecutor.Execute(
		uDevice.raw.Get(),
		uCmdQueue.raw.Get(),
		cmdAlloc.Get(),
		crst,
		*fgRsrcMngr
	);

    // Done recording commands.
 //   ThrowIfFailed(uGCmdList->Close());

 //   // Add the command list to the queue for execution.
	//uCmdQueue.Execute(uGCmdList.raw.Get());

    // Swap the back and front buffers
    ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    //// Advance the fence value to mark commands up to this fence point.
    //// Add an instruction to the command queue to set a new fence point. 
    //// Because we are on the GPU timeline, the new fence point won't be 
    //// set until the GPU finishes processing all the commands prior to this Signal().
	frameRsrcMngr->EndFrame(uCmdQueue.raw.Get());
}

void DeferApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void DeferApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void DeferApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

        // Update angles based on input to orbit camera around box.
        mTheta += dy;
        mPhi += dx;

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
        mRadius = MathHelper::Clamp(mRadius, 1.0f, 150.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}
 
void DeferApp::OnKeyboardInput()
{
}
 
void DeferApp::UpdateCamera()
{
	// Convert Spherical to Cartesian coordinates.
	mEyePos[0] = mRadius * sinf(mTheta) * sinf(mPhi);
	mEyePos[1] = mRadius * cosf(mTheta);
	mEyePos[2] = mRadius * sinf(mTheta) * cosf(mPhi);
	mView = Ubpa::transformf::look_at(mEyePos, { 0.f });
}

void DeferApp::AnimateMaterials()
{
	
}

void DeferApp::UpdateObjectCBs()
{
	auto& currObjectCB = frameRsrcMngr->GetCurrentFrameResource()
		->GetResource<Ubpa::UDX12::ArrayUploadBuffer<ObjectConstants>>("ArrayUploadBuffer<ObjectConstants>");
	for(auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if(e->NumFramesDirty > 0)
		{
			ObjectConstants objConstants;
			/*XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));*/
			objConstants.World = e->World;
			objConstants.TexTransform = e->TexTransform;

			currObjectCB.Set(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void DeferApp::UpdateMaterialCBs()
{
	auto& currMaterialCB = frameRsrcMngr->GetCurrentFrameResource()
		->GetResource<Ubpa::UDX12::ArrayUploadBuffer<MaterialConstants>>("ArrayUploadBuffer<MaterialConstants>");
	for(auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if(mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB.Set(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void DeferApp::UpdateMainPassCB()
{
	mMainPassCB.View = mView;
	mMainPassCB.InvView = mMainPassCB.View.inverse();
	mMainPassCB.Proj = mProj;
	mMainPassCB.InvProj = mMainPassCB.Proj.inverse();
	mMainPassCB.ViewProj = mMainPassCB.Proj * mMainPassCB.View;
	mMainPassCB.InvViewProj = mMainPassCB.InvView * mMainPassCB.InvProj;
	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = { mClientWidth, mClientHeight };
	mMainPassCB.InvRenderTargetSize = { 1.0f / mClientWidth, 1.0f / mClientHeight };

	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = Ubpa::DustEngine::GameTimer::Instance().TotalTime();
	mMainPassCB.DeltaTime = Ubpa::DustEngine::GameTimer::Instance().DeltaTime();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

	auto& currPassCB = frameRsrcMngr->GetCurrentFrameResource()
		->GetResource<Ubpa::UDX12::ArrayUploadBuffer<PassConstants>>("ArrayUploadBuffer<PassConstants>");
	currPassCB.Set(0, mMainPassCB);
}

void DeferApp::LoadTextures()
{
	auto chessboardImg = Ubpa::DustEngine::AssetMngr::Instance()
		.LoadAsset<Ubpa::DustEngine::Image>("../assets/textures/chessboard.png");
	chessboardTex2D = new Ubpa::DustEngine::Texture2D;
	chessboardTex2D->image = chessboardImg;
	if (!Ubpa::DustEngine::AssetMngr::Instance().CreateAsset(chessboardTex2D, "../assets/textures/chessboard.tex2d")) {
		delete chessboardTex2D;
		chessboardTex2D = Ubpa::DustEngine::AssetMngr::Instance()
			.LoadAsset<Ubpa::DustEngine::Texture2D>("../assets/textures/chessboard.tex2d");
	}

	Ubpa::DustEngine::RsrcMngrDX12::Instance().RegisterTexture2D(
		Ubpa::DustEngine::RsrcMngrDX12::Instance().GetUpload(),
		chessboardTex2D
	);
}

void DeferApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	//slotRootParameter[0].InitAsShaderResourceView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[1].InitAsConstantBufferView(0);
    slotRootParameter[2].InitAsConstantBufferView(1);
    slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = Ubpa::DustEngine::RsrcMngrDX12::Instance().GetStaticSamplers();

    // A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	Ubpa::DustEngine::RsrcMngrDX12::Instance().RegisterRootSignature(ID_RootSignature_default, &rootSigDesc);
}

void DeferApp::BuildDescriptorHeaps()
{
}

void DeferApp::BuildShadersAndInputLayout()
{
	std::filesystem::path hlslPath = "../assets/shaders/Default.hlsl";
	std::filesystem::path shaderPath = "../assets/shaders/Default.shader";

	if (!std::filesystem::is_directory("../assets/shaders"))
		std::filesystem::create_directories("../assets/shaders");

	auto& assetMngr = Ubpa::DustEngine::AssetMngr::Instance();
	assetMngr.ImportAsset(hlslPath);
	auto hlslFile = assetMngr.LoadAsset<Ubpa::DustEngine::HLSLFile>(hlslPath);

	shader = new Ubpa::DustEngine::Shader;
	shader->hlslFile = hlslFile;
	shader->vertexName = "vert";
	shader->fragmentName = "frag";
	shader->targetName = "5_0";
	shader->shaderName = "Default";

	if (!assetMngr.CreateAsset(shader, shaderPath)) {
		delete shader;
		shader = assetMngr.LoadAsset<Ubpa::DustEngine::Shader>(shaderPath);
	}

	Ubpa::DustEngine::RsrcMngrDX12::Instance().RegisterShader(shader);
	
    mInputLayout =
    {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
}

void DeferApp::BuildShapeGeometry()
{
	mesh = Ubpa::DustEngine::AssetMngr::Instance().LoadAsset<Ubpa::DustEngine::Mesh>("../assets/models/cube.obj");
	Ubpa::DustEngine::RsrcMngrDX12::Instance().RegisterMesh(
		Ubpa::DustEngine::RsrcMngrDX12::Instance().GetUpload(),
		Ubpa::DustEngine::RsrcMngrDX12::Instance().GetDeleteBatch(),
		uGCmdList.raw.Get(),
		mesh
	);
}

void DeferApp::BuildPSOs()
{
	auto opaquePsoDesc = Ubpa::UDX12::Desc::PSO::Basic(
		Ubpa::DustEngine::RsrcMngrDX12::Instance().GetRootSignature(ID_RootSignature_default),
		mInputLayout.data(), (UINT)mInputLayout.size(),
		Ubpa::DustEngine::RsrcMngrDX12::Instance().GetShaderByteCode_vs(shader),
		Ubpa::DustEngine::RsrcMngrDX12::Instance().GetShaderByteCode_ps(shader),
		mBackBufferFormat,
		mDepthStencilFormat
	);
	opaquePsoDesc.RasterizerState.FrontCounterClockwise = TRUE;
	ID_PSO_opaque = Ubpa::DustEngine::RsrcMngrDX12::Instance().RegisterPSO(&opaquePsoDesc);
}

void DeferApp::BuildFrameResources()
{
	for(const auto& fr : frameRsrcMngr->GetFrameResources()) {
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
		ThrowIfFailed(uDevice->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&allocator)));

		fr->RegisterResource("CommandAllocator", std::move(allocator));

		fr->RegisterResource("ArrayUploadBuffer<PassConstants>",
			Ubpa::UDX12::ArrayUploadBuffer<PassConstants>{ uDevice.raw.Get(), 1, true });

		fr->RegisterResource("ArrayUploadBuffer<MaterialConstants>",
			Ubpa::UDX12::ArrayUploadBuffer<MaterialConstants>{ uDevice.raw.Get(), mMaterials.size(), true });

		fr->RegisterResource("ArrayUploadBuffer<ObjectConstants>",
			Ubpa::UDX12::ArrayUploadBuffer<ObjectConstants>{ uDevice.raw.Get(), mAllRitems.size(), true });

		auto fgRsrcMngr = std::make_shared<Ubpa::UDX12::FG::RsrcMngr>();
		fr->RegisterResource("FrameGraphRsrcMngr", std::move(fgRsrcMngr));
    }
}

void DeferApp::BuildMaterials()
{
	auto woodCrate = std::make_unique<Material>();
	woodCrate->Name = "woodCrate";
	woodCrate->MatCBIndex = 0;
	woodCrate->DiffuseSrvGpuHandle = Ubpa::DustEngine::RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(chessboardTex2D);
	woodCrate->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	woodCrate->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	woodCrate->Roughness = 0.2f;

	mMaterials["woodCrate"] = std::move(woodCrate);
}

void DeferApp::BuildRenderItems()
{
	auto boxRitem = std::make_unique<RenderItem>();
	boxRitem->ObjCBIndex = 0;
	boxRitem->Mat = mMaterials["woodCrate"].get();
	boxRitem->Geo = &Ubpa::DustEngine::RsrcMngrDX12::Instance().GetMeshGPUBuffer(mesh);
	boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->IndexBufferView().SizeInBytes
		/ (boxRitem->Geo->IndexBufferView().Format == DXGI_FORMAT_R16_UINT ? 2 : 4);
	boxRitem->StartIndexLocation = 0;
	boxRitem->BaseVertexLocation = 0;
	mAllRitems.push_back(std::move(boxRitem));

	// All the render items are opaque.
	for(auto& e : mAllRitems)
		mOpaqueRitems.push_back(e.get());
}

void DeferApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
    UINT objCBByteSize = Ubpa::UDX12::Util::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    UINT matCBByteSize = Ubpa::UDX12::Util::CalcConstantBufferByteSize(sizeof(MaterialConstants));
 
	auto objectCB = frameRsrcMngr->GetCurrentFrameResource()
		->GetResource<Ubpa::UDX12::ArrayUploadBuffer<ObjectConstants>>("ArrayUploadBuffer<ObjectConstants>")
		.GetResource();
	auto matCB = frameRsrcMngr->GetCurrentFrameResource()
		->GetResource<Ubpa::UDX12::ArrayUploadBuffer<MaterialConstants>>("ArrayUploadBuffer<MaterialConstants>")
		.GetResource();

    // For each render item...
    for(size_t i = 0; i < ritems.size(); ++i)
    {
        auto ri = ritems[i];

        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex*matCBByteSize;

		cmdList->SetGraphicsRootDescriptorTable(0, ri->Mat->DiffuseSrvGpuHandle);
		//cmdList->SetGraphicsRootShaderResourceView(0, mTextures["woodCrate"]->Resource->GetGPUVirtualAddress());
        cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
        cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}
