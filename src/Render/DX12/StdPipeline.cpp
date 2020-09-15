#include <DustEngine/Render/DX12/StdPipeline.h>

#include <DustEngine/Render/DX12/RsrcMngrDX12.h>
#include <DustEngine/Render/DX12/MeshLayoutMngr.h>
#include <DustEngine/Render/DX12/ShaderCBMngrDX12.h>
#include <DustEngine/Core/ShaderMngr.h>

#include <DustEngine/Asset/AssetMngr.h>

#include <DustEngine/Core/Texture2D.h>
#include <DustEngine/Core/TextureCube.h>
#include <DustEngine/Core/Image.h>
#include <DustEngine/Core/HLSLFile.h>
#include <DustEngine/Core/Shader.h>
#include <DustEngine/Core/Mesh.h>
#include <DustEngine/Core/Components/Camera.h>
#include <DustEngine/Core/Components/MeshFilter.h>
#include <DustEngine/Core/Components/MeshRenderer.h>
#include <DustEngine/Core/Components/Skybox.h>
#include <DustEngine/Core/Systems/CameraSystem.h>
#include <DustEngine/Core/GameTimer.h>

#include <DustEngine/Transform/Transform.h>

#include <DustEngine/_deps/imgui/imgui.h>
#include <DustEngine/_deps/imgui/imgui_impl_win32.h>
#include <DustEngine/_deps/imgui/imgui_impl_dx12.h>

#include <UDX12/FrameResourceMngr.h>

using namespace Ubpa::DustEngine;
using namespace Ubpa;

struct StdPipeline::Impl {
	Impl(InitDesc initDesc)
		:
		initDesc{ initDesc },
		frameRsrcMngr{ initDesc.numFrame, initDesc.device },
		fg { "Standard Pipeline" }
	{
		BuildFrameResources();
		BuildShadersAndInputLayout();
		BuildRootSignature();
		BuildPSOs();
	}

	size_t ID_PSO_defer_light;
	size_t ID_PSO_screen;
	size_t ID_PSO_skybox;
	size_t ID_PSO_postprocess;

	static constexpr size_t ID_RootSignature_geometry = 0;
	static constexpr size_t ID_RootSignature_screen = 1;
	static constexpr size_t ID_RootSignature_defer_light = 2;
	static constexpr size_t ID_RootSignature_skybox = 3;
	static constexpr size_t ID_RootSignature_postprocess = 4;

	struct GeometryObjectConstants {
		Ubpa::transformf World;
	};
	struct CameraConstants {
		Ubpa::transformf View;

		Ubpa::transformf InvView;

		Ubpa::transformf Proj;

		Ubpa::transformf InvProj;

		Ubpa::transformf ViewProj;

		Ubpa::transformf InvViewProj;

		Ubpa::pointf3 EyePosW;
		float _pad0;

		Ubpa::valf2 RenderTargetSize;
		Ubpa::valf2 InvRenderTargetSize;

		float NearZ;
		float FarZ;
		float TotalTime;
		float DeltaTime;

	};
	struct GeometryMaterialConstants {
		Ubpa::rgbf albedoFactor;
		float roughnessFactor;
		float metalnessFactor;
	};

	struct DirectionalLight {
		rgbf L;
		float _pad0;
		vecf3 dir;
		float _pad1;
	};
	struct LightingLights {
		UINT diectionalLightNum;
		UINT _pad0;
		UINT _pad1;
		UINT _pad2;
		DirectionalLight directionalLights[4];
	};

	struct RenderContext {
		struct Object {
			const Mesh* mesh{ nullptr };
			size_t submeshIdx{ static_cast<size_t>(-1) };

			valf<16> l2w;
		};
		std::unordered_map<const Shader*,
			std::unordered_map<const Material*,
			std::vector<Object>>> objectMap;

		D3D12_GPU_DESCRIPTOR_HANDLE skybox;
	};

	const InitDesc initDesc;

	RenderContext renderContext;

	UDX12::FrameResourceMngr frameRsrcMngr;

	Ubpa::UDX12::FG::Executor fgExecutor;
	Ubpa::UFG::Compiler fgCompiler;
	Ubpa::UFG::FrameGraph fg;

	Ubpa::DustEngine::Shader* screenShader;
	Ubpa::DustEngine::Shader* geomrtryShader;
	Ubpa::DustEngine::Shader* deferShader;
	Ubpa::DustEngine::Shader* skyboxShader;
	Ubpa::DustEngine::Shader* postprocessShader;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	const DXGI_FORMAT dsFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	void BuildFrameResources();
	void BuildShadersAndInputLayout();
	void BuildRootSignature();
	void BuildPSOs();

	size_t GetGeometryPSO_ID(const Mesh* mesh);
	std::unordered_map<size_t, size_t> PSOIDMap;

	void UpdateRenderContext(const UECS::World& world);
	void UpdateShaderCBs(const ResizeData& resizeData, const UECS::World& world, const CameraData& cameraData);
	void Render(const ResizeData& resizeData, ID3D12Resource* rtb);
	void DrawObjects(ID3D12GraphicsCommandList*);
};

void StdPipeline::Impl::BuildFrameResources() {
	for (const auto& fr : frameRsrcMngr.GetFrameResources()) {
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
		ThrowIfFailed(initDesc.device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&allocator)));

		fr->RegisterResource("CommandAllocator", allocator);

		fr->RegisterResource("ShaderCBMngrDX12", ShaderCBMngrDX12{ initDesc.device });

		auto fgRsrcMngr = std::make_shared<Ubpa::UDX12::FG::RsrcMngr>();
		fr->RegisterResource("FrameGraphRsrcMngr", fgRsrcMngr);
	}
}

void StdPipeline::Impl::BuildShadersAndInputLayout() {
	screenShader = ShaderMngr::Instance().Get("StdPipeline/Screen");
	geomrtryShader = ShaderMngr::Instance().Get("StdPipeline/Geometry");
	deferShader = ShaderMngr::Instance().Get("StdPipeline/Defer Lighting");
	skyboxShader = ShaderMngr::Instance().Get("StdPipeline/Skybox");
	postprocessShader = ShaderMngr::Instance().Get("StdPipeline/Post Process");
}

void StdPipeline::Impl::BuildRootSignature() {
	{ // geometry
		CD3DX12_DESCRIPTOR_RANGE texRange0;
		texRange0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		CD3DX12_DESCRIPTOR_RANGE texRange1;
		texRange1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
		CD3DX12_DESCRIPTOR_RANGE texRange2;
		texRange2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);

		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[6];

		// Perfomance TIP: Order from most frequent to least frequent.
		slotRootParameter[0].InitAsDescriptorTable(1, &texRange0);
		slotRootParameter[1].InitAsDescriptorTable(1, &texRange1);
		slotRootParameter[2].InitAsDescriptorTable(1, &texRange2);
		slotRootParameter[3].InitAsConstantBufferView(0); // object
		slotRootParameter[4].InitAsConstantBufferView(1); // material
		slotRootParameter[5].InitAsConstantBufferView(2); // camera

		auto staticSamplers = RsrcMngrDX12::Instance().GetStaticSamplers();

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(6, slotRootParameter,
			(UINT)staticSamplers.size(), staticSamplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		RsrcMngrDX12::Instance().RegisterRootSignature(ID_RootSignature_geometry, &rootSigDesc);
	}

	{ // screen
		CD3DX12_DESCRIPTOR_RANGE texTable;
		texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[1];

		// Perfomance TIP: Order from most frequent to least frequent.
		slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);

		auto staticSamplers = RsrcMngrDX12::Instance().GetStaticSamplers();

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter,
			(UINT)staticSamplers.size(), staticSamplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		RsrcMngrDX12::Instance().RegisterRootSignature(ID_RootSignature_screen, &rootSigDesc);
	}

	{ // skybox
		CD3DX12_DESCRIPTOR_RANGE texTable;
		texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[2];

		// Perfomance TIP: Order from most frequent to least frequent.
		slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
		slotRootParameter[1].InitAsConstantBufferView(0); // camera

		auto staticSamplers = RsrcMngrDX12::Instance().GetStaticSamplers();

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter,
			(UINT)staticSamplers.size(), staticSamplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		RsrcMngrDX12::Instance().RegisterRootSignature(ID_RootSignature_skybox, &rootSigDesc);
	}

	{ // defer lighting
		CD3DX12_DESCRIPTOR_RANGE texTable;
		texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0); // gbuffers

		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[3];

		// Perfomance TIP: Order from most frequent to least frequent.
		slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
		slotRootParameter[1].InitAsConstantBufferView(0); // lights
		slotRootParameter[2].InitAsConstantBufferView(1); // camera

		auto staticSamplers = RsrcMngrDX12::Instance().GetStaticSamplers();

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter,
			(UINT)staticSamplers.size(), staticSamplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		RsrcMngrDX12::Instance().RegisterRootSignature(ID_RootSignature_defer_light, &rootSigDesc);
	}

	{ // post process
		CD3DX12_DESCRIPTOR_RANGE texTable;
		texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0); // gbuffers

		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[1];

		// Perfomance TIP: Order from most frequent to least frequent.
		slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);

		auto staticSamplers = RsrcMngrDX12::Instance().GetStaticSamplers();

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter,
			(UINT)staticSamplers.size(), staticSamplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		RsrcMngrDX12::Instance().RegisterRootSignature(ID_RootSignature_postprocess, &rootSigDesc);
	}
}

void StdPipeline::Impl::BuildPSOs() {
	auto screenPsoDesc = Ubpa::UDX12::Desc::PSO::Basic(
		RsrcMngrDX12::Instance().GetRootSignature(ID_RootSignature_screen),
		nullptr, 0,
		RsrcMngrDX12::Instance().GetShaderByteCode_vs(screenShader),
		RsrcMngrDX12::Instance().GetShaderByteCode_ps(screenShader),
		initDesc.rtFormat,
		DXGI_FORMAT_UNKNOWN
	);
	screenPsoDesc.RasterizerState.FrontCounterClockwise = TRUE;
	screenPsoDesc.DepthStencilState.DepthEnable = false;
	screenPsoDesc.DepthStencilState.StencilEnable = false;
	ID_PSO_screen = RsrcMngrDX12::Instance().RegisterPSO(&screenPsoDesc);

	auto skyboxPsoDesc = Ubpa::UDX12::Desc::PSO::Basic(
		RsrcMngrDX12::Instance().GetRootSignature(ID_RootSignature_skybox),
		nullptr, 0,
		RsrcMngrDX12::Instance().GetShaderByteCode_vs(skyboxShader),
		RsrcMngrDX12::Instance().GetShaderByteCode_ps(skyboxShader),
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_D24_UNORM_S8_UINT
	);
	skyboxPsoDesc.RasterizerState.FrontCounterClockwise = TRUE;
	skyboxPsoDesc.DepthStencilState.DepthEnable = true;
	skyboxPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	skyboxPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skyboxPsoDesc.DepthStencilState.StencilEnable = false;
	ID_PSO_skybox = RsrcMngrDX12::Instance().RegisterPSO(&skyboxPsoDesc);

	auto deferLightingPsoDesc = Ubpa::UDX12::Desc::PSO::Basic(
		RsrcMngrDX12::Instance().GetRootSignature(ID_RootSignature_defer_light),
		nullptr, 0,
		RsrcMngrDX12::Instance().GetShaderByteCode_vs(deferShader),
		RsrcMngrDX12::Instance().GetShaderByteCode_ps(deferShader),
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_UNKNOWN
	);
	deferLightingPsoDesc.RasterizerState.FrontCounterClockwise = TRUE;
	deferLightingPsoDesc.DepthStencilState.DepthEnable = false;
	deferLightingPsoDesc.DepthStencilState.StencilEnable = false;
	ID_PSO_defer_light = RsrcMngrDX12::Instance().RegisterPSO(&deferLightingPsoDesc);

	auto postprocessPsoDesc = Ubpa::UDX12::Desc::PSO::Basic(
		RsrcMngrDX12::Instance().GetRootSignature(ID_RootSignature_postprocess),
		nullptr, 0,
		RsrcMngrDX12::Instance().GetShaderByteCode_vs(postprocessShader),
		RsrcMngrDX12::Instance().GetShaderByteCode_ps(postprocessShader),
		initDesc.rtFormat,
		DXGI_FORMAT_UNKNOWN
	);
	postprocessPsoDesc.RasterizerState.FrontCounterClockwise = TRUE;
	postprocessPsoDesc.DepthStencilState.DepthEnable = false;
	postprocessPsoDesc.DepthStencilState.StencilEnable = false;
	ID_PSO_postprocess = RsrcMngrDX12::Instance().RegisterPSO(&postprocessPsoDesc);
}

size_t StdPipeline::Impl::GetGeometryPSO_ID(const Mesh* mesh) {
	size_t layoutID = MeshLayoutMngr::Instance().GetMeshLayoutID(mesh);
	auto target = PSOIDMap.find(layoutID);
	if (target == PSOIDMap.end()) {
		//auto [uv, normal, tangent, color] = MeshLayoutMngr::Instance().DecodeMeshLayoutID(layoutID);
		//if (!uv || !normal)
		//	return static_cast<size_t>(-1); // not support

		const auto& layout = MeshLayoutMngr::Instance().GetMeshLayoutValue(layoutID);
		auto geometryPsoDesc = Ubpa::UDX12::Desc::PSO::MRT(
			RsrcMngrDX12::Instance().GetRootSignature(ID_RootSignature_geometry),
			layout.data(), (UINT)layout.size(),
			RsrcMngrDX12::Instance().GetShaderByteCode_vs(geomrtryShader),
			RsrcMngrDX12::Instance().GetShaderByteCode_ps(geomrtryShader),
			3,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			DXGI_FORMAT_D24_UNORM_S8_UINT
		);
		geometryPsoDesc.RasterizerState.FrontCounterClockwise = TRUE;
		size_t ID_PSO_geometry = RsrcMngrDX12::Instance().RegisterPSO(&geometryPsoDesc);
		target = PSOIDMap.emplace_hint(target, std::pair{ layoutID, ID_PSO_geometry });
	}
	return target->second;
}

void StdPipeline::Impl::UpdateRenderContext(const UECS::World& world) {
	renderContext.objectMap.clear();

	Ubpa::UECS::ArchetypeFilter objectFilter;
	objectFilter.all = {
		Ubpa::UECS::CmptAccessType::Of<MeshFilter>,
		Ubpa::UECS::CmptAccessType::Of<MeshRenderer>
	};

	UECS::ArchetypeFilter filter;
	filter.all = { UECS::CmptAccessType::Of<UECS::Latest<MeshFilter>>, UECS::CmptAccessType::Of<UECS::Latest<MeshRenderer>> };

	const_cast<UECS::World&>(world).RunChunkJob(
		[&](UECS::ChunkView chunk) {
			auto meshFilterArr = chunk.GetCmptArray<MeshFilter>();
			auto meshRendererArr = chunk.GetCmptArray<MeshRenderer>();
			auto l2wArr = chunk.GetCmptArray<LocalToWorld>();
			size_t num = chunk.EntityNum();
			for (size_t i = 0; i < num; i++) {
				RenderContext::Object obj;
				obj.mesh = meshFilterArr[i].mesh;
				obj.l2w = l2wArr ? l2wArr[i].value.as<valf<16>>() : transformf::eye().as<valf<16>>();
				for (size_t j = 0; j < std::min(meshRendererArr[i].materials.size(), obj.mesh->GetSubMeshes().size()); j++) {
					auto material = meshRendererArr[i].materials[j];
					obj.submeshIdx = j;
					renderContext.objectMap[material->shader][material].push_back(obj);
				}
			}
		},
		filter,
		false
	);

	renderContext.skybox.ptr = 0;
	if (auto ptr = world.entityMngr.GetSingleton<Skybox>(); ptr && ptr->material && ptr->material->shader == skyboxShader) {
		auto target = ptr->material->textureCubes.find("gSkybox");
		if(target != ptr->material->textureCubes.end())
			renderContext.skybox = RsrcMngrDX12::Instance().GetTextureCubeSrvGpuHandle(target->second);
	}
}

void StdPipeline::Impl::UpdateShaderCBs(
	const ResizeData& resizeData,
	const UECS::World& world,
	const CameraData& cameraData
) {
	auto& shaderCBMngr = frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<Ubpa::DustEngine::ShaderCBMngrDX12>("ShaderCBMngrDX12");

	{ // defer lighting
		LightingLights lights;
		lights.diectionalLightNum = 3;
		lights.directionalLights[0].L = { 6.f };
		lights.directionalLights[0].dir = { 0.57735f, -0.57735f, 0.57735f };
		lights.directionalLights[1].L = { 3.f };
		lights.directionalLights[1].dir = { -0.57735f, -0.57735f, 0.57735f };
		lights.directionalLights[2].L = { 1.5f };
		lights.directionalLights[2].dir = { 0.0f, -0.707f, -0.707f };

		auto buffer = shaderCBMngr.GetBuffer(deferShader);
		buffer->FastReserve(Ubpa::UDX12::Util::CalcConstantBufferByteSize(sizeof(LightingLights)));
		buffer->Set(0, &lights, sizeof(LightingLights));
	}

	{ // camera
		auto buffer = shaderCBMngr.GetCommonBuffer();
		buffer->FastReserve(Ubpa::UDX12::Util::CalcConstantBufferByteSize(sizeof(CameraConstants)));

		auto cmptCamera = cameraData.world.entityMngr.Get<Camera>(cameraData.entity);
		auto cmptW2L = cameraData.world.entityMngr.Get<WorldToLocal>(cameraData.entity);
		auto cmptTranslation = cameraData.world.entityMngr.Get<Translation>(cameraData.entity);
		CameraConstants cbPerCamera;
		cbPerCamera.View = cmptW2L->value;
		cbPerCamera.InvView = cbPerCamera.View.inverse();
		cbPerCamera.Proj = cmptCamera->prjectionMatrix;
		cbPerCamera.InvProj = cbPerCamera.Proj.inverse();
		cbPerCamera.ViewProj = cbPerCamera.Proj * cbPerCamera.View;
		cbPerCamera.InvViewProj = cbPerCamera.InvView * cbPerCamera.InvProj;
		cbPerCamera.EyePosW = cmptTranslation->value.as<pointf3>();
		cbPerCamera.RenderTargetSize = { resizeData.width, resizeData.height };
		cbPerCamera.InvRenderTargetSize = { 1.0f / resizeData.width, 1.0f / resizeData.height };

		cbPerCamera.NearZ = cmptCamera->clippingPlaneMin;
		cbPerCamera.FarZ = cmptCamera->clippingPlaneMax;
		cbPerCamera.TotalTime = Ubpa::DustEngine::GameTimer::Instance().TotalTime();
		cbPerCamera.DeltaTime = Ubpa::DustEngine::GameTimer::Instance().DeltaTime();

		buffer->Set(0, &cbPerCamera, sizeof(CameraConstants));
	}

	// geometry
	for (const auto& [shader, mat2objects] : renderContext.objectMap) {
		size_t objectNum = 0;
		for (const auto& [mat, objects] : mat2objects)
			objectNum += objects.size();
		if (shader->shaderName == "StdPipeline/Geometry") {
			auto buffer = shaderCBMngr.GetBuffer(shader);
			buffer->FastReserve(
				mat2objects.size() * Ubpa::UDX12::Util::CalcConstantBufferByteSize(sizeof(GeometryMaterialConstants))
				+ objectNum * Ubpa::UDX12::Util::CalcConstantBufferByteSize(sizeof(GeometryObjectConstants))
			);
			size_t offset = 0;
			for (const auto& [mat, objects] : mat2objects) {
				GeometryMaterialConstants matC;
				matC.albedoFactor = { 1.f };
				matC.roughnessFactor = 1.f;
				matC.metalnessFactor = 1.f;
				buffer->Set(offset, &matC, sizeof(GeometryMaterialConstants));
				offset += Ubpa::UDX12::Util::CalcConstantBufferByteSize(sizeof(GeometryMaterialConstants));
				for (const auto& object : objects) {
					GeometryObjectConstants objectConstants;
					objectConstants.World = object.l2w;
					buffer->Set(offset, &objectConstants, sizeof(GeometryObjectConstants));
					offset += Ubpa::UDX12::Util::CalcConstantBufferByteSize(sizeof(GeometryObjectConstants));
				}
			}
		}
	}
}

void StdPipeline::Impl::Render(const ResizeData& resizeData, ID3D12Resource* rtb) {
	size_t width = resizeData.width;
	size_t height = resizeData.height;

	auto cmdAlloc = frameRsrcMngr.GetCurrentFrameResource()->GetResource<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>("CommandAllocator");
	cmdAlloc->Reset();

	fg.Clear();
	auto fgRsrcMngr = frameRsrcMngr.GetCurrentFrameResource()->GetResource<std::shared_ptr<Ubpa::UDX12::FG::RsrcMngr>>("FrameGraphRsrcMngr");
	fgRsrcMngr->NewFrame();
	fgExecutor.NewFrame();;

	auto gbuffer0 = fg.RegisterResourceNode("GBuffer0");
	auto gbuffer1 = fg.RegisterResourceNode("GBuffer1");
	auto gbuffer2 = fg.RegisterResourceNode("GBuffer2");
	auto lightedRT = fg.RegisterResourceNode("lighted RT");
	auto fullRT = fg.RegisterResourceNode("full RT");
	auto presentedRT = fg.RegisterResourceNode("Present RT");
	fg.RegisterMoveNode(fullRT, lightedRT);
	auto depthstencil = fg.RegisterResourceNode("Depth Stencil");
	auto gbPass = fg.RegisterPassNode(
		"GBuffer Pass",
		{},
		{ gbuffer0,gbuffer1,gbuffer2,depthstencil }
	);
	auto deferLightingPass = fg.RegisterPassNode(
		"Defer Lighting",
		{ gbuffer0,gbuffer1,gbuffer2 },
		{ lightedRT }
	);
	auto skyboxPass = fg.RegisterPassNode(
		"Skybox",
		{ depthstencil },
		{ fullRT }
	);
	auto postprocessPass = fg.RegisterPassNode(
		"Post Process",
		{ fullRT },
		{ presentedRT }
	);

	D3D12_RESOURCE_DESC dsDesc = UDX12::Desc::RSRC::Basic(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		width,
		(UINT)height,
		DXGI_FORMAT_R24G8_TYPELESS,
		// Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to read from 
		// the depth buffer.  Therefore, because we need to create two views to the same resource:
		//   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
		//   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
		// we need to create the depth buffer resource with a typeless format.  
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	);

	D3D12_CLEAR_VALUE dsClear;
	dsClear.Format = dsFormat;
	dsClear.DepthStencil.Depth = 1.0f;
	dsClear.DepthStencil.Stencil = 0;

	(*fgRsrcMngr)
		.RegisterTemporalRsrc(gbuffer0,
			Ubpa::UDX12::FG::RsrcType::RT2D(DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, DirectX::Colors::Black))
		.RegisterTemporalRsrc(gbuffer1,
			Ubpa::UDX12::FG::RsrcType::RT2D(DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, DirectX::Colors::Black))
		.RegisterTemporalRsrc(gbuffer2,
			Ubpa::UDX12::FG::RsrcType::RT2D(DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, DirectX::Colors::Black))
		.RegisterTemporalRsrc(depthstencil, { dsClear, dsDesc })
		.RegisterTemporalRsrc(lightedRT,
			Ubpa::UDX12::FG::RsrcType::RT2D(DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, DirectX::Colors::Black))
		.RegisterTemporalRsrc(fullRT,
			Ubpa::UDX12::FG::RsrcType::RT2D(DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, DirectX::Colors::Black))

		.RegisterRsrcTable({
			{gbuffer0,Ubpa::UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT)},
			{gbuffer1,Ubpa::UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT)},
			{gbuffer2,Ubpa::UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT)}
		})

		.RegisterImportedRsrc(presentedRT, { rtb, D3D12_RESOURCE_STATE_PRESENT })

		.RegisterPassRsrcs(gbPass, gbuffer0, D3D12_RESOURCE_STATE_RENDER_TARGET,
			Ubpa::UDX12::FG::RsrcImplDesc_RTV_Null{})
		.RegisterPassRsrcs(gbPass, gbuffer1, D3D12_RESOURCE_STATE_RENDER_TARGET,
			Ubpa::UDX12::FG::RsrcImplDesc_RTV_Null{})
		.RegisterPassRsrcs(gbPass, gbuffer2, D3D12_RESOURCE_STATE_RENDER_TARGET,
			Ubpa::UDX12::FG::RsrcImplDesc_RTV_Null{})
		.RegisterPassRsrcs(gbPass, depthstencil,
			D3D12_RESOURCE_STATE_DEPTH_WRITE, Ubpa::UDX12::Desc::DSV::Basic(dsFormat))

		.RegisterPassRsrcs(deferLightingPass, gbuffer0, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			Ubpa::UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT))
		.RegisterPassRsrcs(deferLightingPass, gbuffer1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			Ubpa::UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT))
		.RegisterPassRsrcs(deferLightingPass, gbuffer2, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			Ubpa::UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT))
		.RegisterPassRsrcs(deferLightingPass, lightedRT, D3D12_RESOURCE_STATE_RENDER_TARGET,
			Ubpa::UDX12::FG::RsrcImplDesc_RTV_Null{})

		.RegisterPassRsrcs(skyboxPass, depthstencil,
			D3D12_RESOURCE_STATE_DEPTH_READ, Ubpa::UDX12::Desc::DSV::Basic(dsFormat))
		.RegisterPassRsrcs(skyboxPass, fullRT, D3D12_RESOURCE_STATE_RENDER_TARGET,
			Ubpa::UDX12::FG::RsrcImplDesc_RTV_Null{})

		.RegisterPassRsrcs(postprocessPass, fullRT, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			Ubpa::UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT))
		.RegisterPassRsrcs(postprocessPass, presentedRT, D3D12_RESOURCE_STATE_RENDER_TARGET,
			Ubpa::UDX12::FG::RsrcImplDesc_RTV_Null{})
		;

	fgExecutor.RegisterPassFunc(
		gbPass,
		[&](ID3D12GraphicsCommandList* cmdList, const Ubpa::UDX12::FG::PassRsrcs& rsrcs) {
			auto heap = Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);
			cmdList->RSSetViewports(1, &resizeData.screenViewport);
			cmdList->RSSetScissorRects(1, &resizeData.scissorRect);

			auto gb0 = rsrcs.find(gbuffer0)->second;
			auto gb1 = rsrcs.find(gbuffer1)->second;
			auto gb2 = rsrcs.find(gbuffer2)->second;
			auto ds = rsrcs.find(depthstencil)->second;

			// Clear the render texture and depth buffer.
			cmdList->ClearRenderTargetView(gb0.cpuHandle, DirectX::Colors::Black, 0, nullptr);
			cmdList->ClearRenderTargetView(gb1.cpuHandle, DirectX::Colors::Black, 0, nullptr);
			cmdList->ClearRenderTargetView(gb2.cpuHandle, DirectX::Colors::Black, 0, nullptr);
			cmdList->ClearDepthStencilView(ds.cpuHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

			// Specify the buffers we are going to render to.
			std::array rts{ gb0.cpuHandle,gb1.cpuHandle,gb2.cpuHandle };
			cmdList->OMSetRenderTargets(rts.size(), rts.data(), false, &ds.cpuHandle);

			cmdList->SetGraphicsRootSignature(RsrcMngrDX12::Instance().GetRootSignature(Impl::ID_RootSignature_geometry));

			auto cbPerCamera = frameRsrcMngr.GetCurrentFrameResource()
				->GetResource<ShaderCBMngrDX12>("ShaderCBMngrDX12")
				.GetCommonBuffer()->GetResource();

			cmdList->SetGraphicsRootConstantBufferView(5, cbPerCamera->GetGPUVirtualAddress());

			DrawObjects(cmdList);
		}
	);

	fgExecutor.RegisterPassFunc(
		deferLightingPass,
		[&](ID3D12GraphicsCommandList* cmdList, const Ubpa::UDX12::FG::PassRsrcs& rsrcs) {
			auto heap = Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);
			cmdList->RSSetViewports(1, &resizeData.screenViewport);
			cmdList->RSSetScissorRects(1, &resizeData.scissorRect);

			auto gb0 = rsrcs.find(gbuffer0)->second;
			auto gb1 = rsrcs.find(gbuffer1)->second;
			auto gb2 = rsrcs.find(gbuffer2)->second;

			auto rt = rsrcs.find(lightedRT)->second;

			//cmdList->CopyResource(bb.resource, rt.resource);

			// Clear the render texture and depth buffer.
			cmdList->ClearRenderTargetView(rt.cpuHandle, DirectX::Colors::Black, 0, nullptr);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &rt.cpuHandle, false, nullptr);

			cmdList->SetGraphicsRootSignature(RsrcMngrDX12::Instance().GetRootSignature(Impl::ID_RootSignature_defer_light));
			cmdList->SetPipelineState(RsrcMngrDX12::Instance().GetPSO(ID_PSO_defer_light));

			cmdList->SetGraphicsRootDescriptorTable(0, gb0.gpuHandle);

			auto cbLights = frameRsrcMngr.GetCurrentFrameResource()
				->GetResource<ShaderCBMngrDX12>("ShaderCBMngrDX12")
				.GetBuffer(deferShader)->GetResource();
			cmdList->SetGraphicsRootConstantBufferView(1, cbLights->GetGPUVirtualAddress());

			auto cbPerCamera = frameRsrcMngr.GetCurrentFrameResource()
				->GetResource<ShaderCBMngrDX12>("ShaderCBMngrDX12")
				.GetCommonBuffer()->GetResource();
			cmdList->SetGraphicsRootConstantBufferView(2, cbPerCamera->GetGPUVirtualAddress());

			cmdList->IASetVertexBuffers(0, 0, nullptr);
			cmdList->IASetIndexBuffer(nullptr);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(6, 1, 0, 0);
		}
	);

	fgExecutor.RegisterPassFunc(
		skyboxPass,
		[&](ID3D12GraphicsCommandList* cmdList, const Ubpa::UDX12::FG::PassRsrcs& rsrcs) {
			if (!renderContext.skybox.ptr)
				return;

			cmdList->SetGraphicsRootSignature(RsrcMngrDX12::Instance().GetRootSignature(Impl::ID_RootSignature_skybox));
			cmdList->SetPipelineState(RsrcMngrDX12::Instance().GetPSO(ID_PSO_skybox));

			auto heap = Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);
			cmdList->RSSetViewports(1, &resizeData.screenViewport);
			cmdList->RSSetScissorRects(1, &resizeData.scissorRect);

			auto rt = rsrcs.find(fullRT)->second;
			auto ds = rsrcs.find(depthstencil)->second;

			//cmdList->CopyResource(bb.resource, rt.resource);

			// Clear the render texture and depth buffer.
			//cmdList->ClearRenderTargetView(rt.cpuHandle, DirectX::Colors::Black, 0, nullptr);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &rt.cpuHandle, false, &ds.cpuHandle);

			cmdList->SetGraphicsRootDescriptorTable(0, renderContext.skybox);

			auto cbPerCamera = frameRsrcMngr.GetCurrentFrameResource()
				->GetResource<ShaderCBMngrDX12>("ShaderCBMngrDX12")
				.GetCommonBuffer()->GetResource();
			cmdList->SetGraphicsRootConstantBufferView(1, cbPerCamera->GetGPUVirtualAddress());

			cmdList->IASetVertexBuffers(0, 0, nullptr);
			cmdList->IASetIndexBuffer(nullptr);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(36, 1, 0, 0);
		}
	);

	fgExecutor.RegisterPassFunc(
		postprocessPass,
		[&](ID3D12GraphicsCommandList* cmdList, const Ubpa::UDX12::FG::PassRsrcs& rsrcs) {
			cmdList->SetGraphicsRootSignature(RsrcMngrDX12::Instance().GetRootSignature(Impl::ID_RootSignature_postprocess));
			cmdList->SetPipelineState(RsrcMngrDX12::Instance().GetPSO(ID_PSO_postprocess));

			auto heap = Ubpa::UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);
			cmdList->RSSetViewports(1, &resizeData.screenViewport);
			cmdList->RSSetScissorRects(1, &resizeData.scissorRect);

			auto rt = rsrcs.find(presentedRT)->second;
			auto img = rsrcs.find(fullRT)->second;

			//cmdList->CopyResource(bb.resource, rt.resource);

			// Clear the render texture and depth buffer.
			cmdList->ClearRenderTargetView(rt.cpuHandle, DirectX::Colors::Black, 0, nullptr);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &rt.cpuHandle, false, nullptr);

			cmdList->SetGraphicsRootDescriptorTable(0, img.gpuHandle);

			cmdList->IASetVertexBuffers(0, 0, nullptr);
			cmdList->IASetIndexBuffer(nullptr);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(6, 1, 0, 0);
		}
	);

	static bool flag{ false };
	if (!flag) {
		OutputDebugStringA(fg.ToGraphvizGraph().Dump().c_str());
		flag = true;
	}

	auto [success, crst] = fgCompiler.Compile(fg);
	fgExecutor.Execute(
		initDesc.device,
		initDesc.cmdQueue,
		cmdAlloc.Get(),
		crst,
		*fgRsrcMngr
	);
}

void StdPipeline::Impl::DrawObjects(ID3D12GraphicsCommandList* cmdList) {
	constexpr UINT matCBByteSize = Ubpa::UDX12::Util::CalcConstantBufferByteSize(sizeof(GeometryMaterialConstants));
	constexpr UINT objCBByteSize = Ubpa::UDX12::Util::CalcConstantBufferByteSize(sizeof(GeometryObjectConstants));

	auto& shaderCBMngr = frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<Ubpa::DustEngine::ShaderCBMngrDX12>("ShaderCBMngrDX12");

	auto buffer = shaderCBMngr.GetBuffer(geomrtryShader);

	const auto& mat2objects = renderContext.objectMap.find(geomrtryShader)->second;

	size_t offset = 0;
	for (const auto& [mat, objects] : mat2objects) {
		// For each render item...
		size_t objIdx = 0;
		for (size_t i = 0; i < objects.size(); i++) {
			auto object = objects[i];
			auto& meshGPUBuffer = Ubpa::DustEngine::RsrcMngrDX12::Instance().GetMeshGPUBuffer(object.mesh);
			const auto& submesh = object.mesh->GetSubMeshes().at(object.submeshIdx);
			cmdList->IASetVertexBuffers(0, 1, &meshGPUBuffer.VertexBufferView());
			cmdList->IASetIndexBuffer(&meshGPUBuffer.IndexBufferView());
			// submesh.topology
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			D3D12_GPU_VIRTUAL_ADDRESS objCBAddress =
				buffer->GetResource()->GetGPUVirtualAddress()
				+ offset + matCBByteSize
				+ objIdx * objCBByteSize;
			D3D12_GPU_VIRTUAL_ADDRESS matCBAddress =
				buffer->GetResource()->GetGPUVirtualAddress()
				+ offset;

			auto albedo = mat->texture2Ds.find("gAlbedoMap")->second;
			auto roughness = mat->texture2Ds.find("gRoughnessMap")->second;
			auto metalness = mat->texture2Ds.find("gMetalnessMap")->second;
			auto albedoHandle = Ubpa::DustEngine::RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(albedo);
			auto roughnessHandle = Ubpa::DustEngine::RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(roughness);
			auto matalnessHandle = Ubpa::DustEngine::RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(metalness);
			cmdList->SetGraphicsRootDescriptorTable(0, albedoHandle);
			cmdList->SetGraphicsRootDescriptorTable(1, roughnessHandle);
			cmdList->SetGraphicsRootDescriptorTable(2, matalnessHandle);
			cmdList->SetGraphicsRootConstantBufferView(3, objCBAddress);
			cmdList->SetGraphicsRootConstantBufferView(4, matCBAddress);

			cmdList->SetPipelineState(RsrcMngrDX12::Instance().GetPSO(GetGeometryPSO_ID(object.mesh)));
			cmdList->DrawIndexedInstanced(submesh.indexCount, 1, submesh.indexStart, submesh.baseVertex, 0);
			objIdx++;
		}
		offset += matCBByteSize + objects.size() * objCBByteSize;
	}
}

StdPipeline::StdPipeline(InitDesc initDesc)
	:
	IPipeline{ initDesc },
	pImpl{ new Impl{ initDesc } }
{}

StdPipeline::~StdPipeline() {
	delete pImpl;
}

void StdPipeline::BeginFrame(const UECS::World& world, const CameraData& cameraData) {
	// collect some cpu data
	pImpl->UpdateRenderContext(world);

	// Cycle through the circular frame resource array.
	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	pImpl->frameRsrcMngr.BeginFrame();

	// cpu -> gpu
	pImpl->UpdateShaderCBs(GetResizeData(), world, cameraData);
}

void StdPipeline::Render(ID3D12Resource* rt) {
	pImpl->Render(GetResizeData(), rt);
}

void StdPipeline::EndFrame() {
	pImpl->frameRsrcMngr.EndFrame(initDesc.cmdQueue);
}

void StdPipeline::Impl_Resize() {
	for (auto& frsrc : pImpl->frameRsrcMngr.GetFrameResources()) {
		frsrc->DelayUpdateResource(
			"FrameGraphRsrcMngr",
			[](std::shared_ptr<Ubpa::UDX12::FG::RsrcMngr> rsrcMngr) {
				rsrcMngr->Clear();
			}
		);
	}
}
