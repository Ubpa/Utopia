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
		BuildTextures();
		BuildFrameResources();
		BuildShaders();
		BuildRootSignature();
		BuildPSOs();
	}

	~Impl();

	size_t ID_PSO_defer_light;
	size_t ID_PSO_screen;
	size_t ID_PSO_skybox;
	size_t ID_PSO_postprocess;
	size_t ID_PSO_irradiance;

	static constexpr size_t ID_RootSignature_geometry = 0;
	static constexpr size_t ID_RootSignature_screen = 1;
	static constexpr size_t ID_RootSignature_defer_light = 2;
	static constexpr size_t ID_RootSignature_skybox = 3;
	static constexpr size_t ID_RootSignature_postprocess = 4;
	static constexpr size_t ID_RootSignature_irradiance = 5;

	struct GeometryObjectConstants {
		transformf World;
	};
	struct CameraConstants {
		transformf View;

		transformf InvView;

		transformf Proj;

		transformf InvProj;

		transformf ViewProj;

		transformf InvViewProj;

		pointf3 EyePosW;
		float _pad0;

		valf2 RenderTargetSize;
		valf2 InvRenderTargetSize;

		float NearZ;
		float FarZ;
		float TotalTime;
		float DeltaTime;

	};
	struct GeometryMaterialConstants {
		rgbf albedoFactor;
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

	struct IrradiancePositionLs {
		valf4 positionL4x;
		valf4 positionL4y;
		valf4 positionL4z;
	};

	struct IrradianceData {
		D3D12_GPU_DESCRIPTOR_HANDLE lastSkybox{ 0 };

		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		UDX12::DescriptorHeapAllocation RTVsDH; // 6
		UDX12::DescriptorHeapAllocation SRVDH; // 1
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
	D3D12_GPU_DESCRIPTOR_HANDLE defaultSkybox;

	UDX12::FrameResourceMngr frameRsrcMngr;

	UDX12::FG::Executor fgExecutor;
	UFG::Compiler fgCompiler;
	UFG::FrameGraph fg;

	DustEngine::Shader* screenShader;
	DustEngine::Shader* geomrtryShader;
	DustEngine::Shader* deferShader;
	DustEngine::Shader* skyboxShader;
	DustEngine::Shader* postprocessShader;
	DustEngine::Shader* irradianceShader;

	static constexpr size_t IrradianceSize = 512;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	const DXGI_FORMAT dsFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	void BuildTextures();
	void BuildFrameResources();
	void BuildShaders();
	void BuildRootSignature();
	void BuildPSOs();

	size_t GetGeometryPSO_ID(const Mesh* mesh);
	std::unordered_map<size_t, size_t> PSOIDMap;

	void UpdateRenderContext(const UECS::World& world);
	void UpdateShaderCBs(const ResizeData& resizeData, const UECS::World& world, const CameraData& cameraData);
	void Render(const ResizeData& resizeData, ID3D12Resource* rtb);
	void DrawObjects(ID3D12GraphicsCommandList*);
};

StdPipeline::Impl::~Impl() {
	for (auto& fr : frameRsrcMngr.GetFrameResources()) {
		auto data = fr->GetResource<std::shared_ptr<IrradianceData>>("irradiance data");
		UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Free(std::move(data->RTVsDH));
		UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(data->SRVDH));
	}
}

void StdPipeline::Impl::BuildTextures() {
	auto skyboxBlack = AssetMngr::Instance().LoadAsset<Material>(LR"(..\assets\_internal\materials\skyBlack.mat)");
	defaultSkybox = RsrcMngrDX12::Instance().GetTextureCubeSrvGpuHandle(skyboxBlack->textureCubes.at("gSkybox"));
}

void StdPipeline::Impl::BuildFrameResources() {
	for (const auto& fr : frameRsrcMngr.GetFrameResources()) {
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
		ThrowIfFailed(initDesc.device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&allocator)));

		fr->RegisterResource("CommandAllocator", std::move(allocator));

		fr->RegisterResource("ShaderCBMngrDX12", ShaderCBMngrDX12{ initDesc.device });

		auto fgRsrcMngr = std::make_shared<UDX12::FG::RsrcMngr>();
		fr->RegisterResource("FrameGraphRsrcMngr", fgRsrcMngr);

		D3D12_CLEAR_VALUE clearColor;
		clearColor.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		clearColor.Color[0] = 0.f;
		clearColor.Color[1] = 0.f;
		clearColor.Color[2] = 0.f;
		clearColor.Color[3] = 1.f;
		auto irradianceData = std::make_shared<IrradianceData>();
		auto rsrcDesc = UDX12::Desc::RSRC::TextureCube(IrradianceSize, IrradianceSize, 1, DXGI_FORMAT_R32G32B32A32_FLOAT);
		initDesc.device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&rsrcDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearColor,
			IID_PPV_ARGS(&irradianceData->resource)
		);
		irradianceData->RTVsDH = UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Allocate(6);
		for (UINT i = 0; i < 6; i++) {
			auto rtvDesc = UDX12::Desc::RTV::Tex2DofTexCube(DXGI_FORMAT_R32G32B32A32_FLOAT, i);
			initDesc.device->CreateRenderTargetView(irradianceData->resource.Get(), &rtvDesc, irradianceData->RTVsDH.GetCpuHandle(i));
		}
		irradianceData->SRVDH = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(1);
		auto srvDesc = UDX12::Desc::SRV::TexCube(DXGI_FORMAT_R32G32B32A32_FLOAT);
		initDesc.device->CreateShaderResourceView(irradianceData->resource.Get(), &srvDesc, irradianceData->SRVDH.GetCpuHandle());
		fr->RegisterResource("irradiance data", irradianceData);
	}
}

void StdPipeline::Impl::BuildShaders() {
	screenShader = ShaderMngr::Instance().Get("StdPipeline/Screen");
	geomrtryShader = ShaderMngr::Instance().Get("StdPipeline/Geometry");
	deferShader = ShaderMngr::Instance().Get("StdPipeline/Defer Lighting");
	skyboxShader = ShaderMngr::Instance().Get("StdPipeline/Skybox");
	postprocessShader = ShaderMngr::Instance().Get("StdPipeline/Post Process");
	irradianceShader = ShaderMngr::Instance().Get("StdPipeline/Irradiance");

	vecf3 origin[6] = {
		{ 1,-1, 1}, // +x right
		{-1,-1,-1}, // -x left
		{-1, 1, 1}, // +y top
		{-1,-1,-1}, // -y buttom
		{-1,-1, 1}, // +z front
		{ 1,-1,-1}, // -z back
	};

	vecf3 right[6] = {
		{ 0, 0,-2}, // +x
		{ 0, 0, 2}, // -x
		{ 2, 0, 0}, // +y
		{ 2, 0, 0}, // -y
		{ 2, 0, 0}, // +z
		{-2, 0, 0}, // -z
	};

	vecf3 up[6] = {
		{ 0, 2, 0}, // +x
		{ 0, 2, 0}, // -x
		{ 0, 0,-2}, // +y
		{ 0, 0, 2}, // -y
		{ 0, 2, 0}, // +z
		{ 0, 2, 0}, // -z
	};
	for (const auto& fr : frameRsrcMngr.GetFrameResources()) {
		auto& shaderCBMngr = fr->GetResource<DustEngine::ShaderCBMngrDX12>("ShaderCBMngrDX12");
		auto buffer = shaderCBMngr.GetBuffer(irradianceShader);
		const auto size = UDX12::Util::CalcConstantBufferByteSize(sizeof(IrradiancePositionLs));
		buffer->FastReserve(6 * size);
		for (size_t i = 0; i < 6; i++) {
			IrradiancePositionLs positionLs;
			auto p0 = origin[i];
			auto p1 = origin[i] + right[i];
			auto p2 = origin[i] + right[i] + up[i];
			auto p3 = origin[i] + up[i];
			positionLs.positionL4x = { p0[0], p1[0], p2[0], p3[0] };
			positionLs.positionL4y = { p0[1], p1[1], p2[1], p3[1] };
			positionLs.positionL4z = { p0[2], p1[2], p2[2], p3[2] };
			buffer->Set(i * size, &positionLs, sizeof(IrradiancePositionLs));
		}
	}
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

	{ // irradiance
		CD3DX12_DESCRIPTOR_RANGE texTable;
		texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[2];

		// Perfomance TIP: Order from most frequent to least frequent.
		slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
		slotRootParameter[1].InitAsConstantBufferView(0); // positionLs

		auto staticSamplers = RsrcMngrDX12::Instance().GetStaticSamplers();

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter,
			(UINT)staticSamplers.size(), staticSamplers.data(),
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		RsrcMngrDX12::Instance().RegisterRootSignature(ID_RootSignature_irradiance, &rootSigDesc);
	}

	{ // defer lighting
		CD3DX12_DESCRIPTOR_RANGE gbufferRange;
		gbufferRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0); // gbuffers
		CD3DX12_DESCRIPTOR_RANGE irradianceMap;
		irradianceMap.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3); // gbuffers

		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[4];

		// Perfomance TIP: Order from most frequent to least frequent.
		slotRootParameter[0].InitAsDescriptorTable(1, &gbufferRange, D3D12_SHADER_VISIBILITY_PIXEL);
		slotRootParameter[1].InitAsDescriptorTable(1, &irradianceMap, D3D12_SHADER_VISIBILITY_PIXEL);
		slotRootParameter[2].InitAsConstantBufferView(0); // lights
		slotRootParameter[3].InitAsConstantBufferView(1); // camera

		auto staticSamplers = RsrcMngrDX12::Instance().GetStaticSamplers();

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
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
	auto screenPsoDesc = UDX12::Desc::PSO::Basic(
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

	auto skyboxPsoDesc = UDX12::Desc::PSO::Basic(
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

	auto deferLightingPsoDesc = UDX12::Desc::PSO::Basic(
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

	auto postprocessPsoDesc = UDX12::Desc::PSO::Basic(
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

	{
		auto desc = UDX12::Desc::PSO::Basic(
			RsrcMngrDX12::Instance().GetRootSignature(ID_RootSignature_irradiance),
			nullptr, 0,
			RsrcMngrDX12::Instance().GetShaderByteCode_vs(irradianceShader),
			RsrcMngrDX12::Instance().GetShaderByteCode_ps(irradianceShader),
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			DXGI_FORMAT_UNKNOWN
		);
		desc.RasterizerState.FrontCounterClockwise = TRUE;
		desc.DepthStencilState.DepthEnable = false;
		desc.DepthStencilState.StencilEnable = false;
		ID_PSO_irradiance = RsrcMngrDX12::Instance().RegisterPSO(&desc);
	}
}

size_t StdPipeline::Impl::GetGeometryPSO_ID(const Mesh* mesh) {
	size_t layoutID = MeshLayoutMngr::Instance().GetMeshLayoutID(mesh);
	auto target = PSOIDMap.find(layoutID);
	if (target == PSOIDMap.end()) {
		//auto [uv, normal, tangent, color] = MeshLayoutMngr::Instance().DecodeMeshLayoutID(layoutID);
		//if (!uv || !normal)
		//	return static_cast<size_t>(-1); // not support

		const auto& layout = MeshLayoutMngr::Instance().GetMeshLayoutValue(layoutID);
		auto geometryPsoDesc = UDX12::Desc::PSO::MRT(
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

	UECS::ArchetypeFilter objectFilter;
	objectFilter.all = {
		UECS::CmptAccessType::Of<MeshFilter>,
		UECS::CmptAccessType::Of<MeshRenderer>
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

	renderContext.skybox = defaultSkybox;
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
		->GetResource<DustEngine::ShaderCBMngrDX12>("ShaderCBMngrDX12");

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
		buffer->FastReserve(UDX12::Util::CalcConstantBufferByteSize(sizeof(LightingLights)));
		buffer->Set(0, &lights, sizeof(LightingLights));
	}

	{ // camera
		auto buffer = shaderCBMngr.GetCommonBuffer();
		buffer->FastReserve(UDX12::Util::CalcConstantBufferByteSize(sizeof(CameraConstants)));

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
		cbPerCamera.TotalTime = DustEngine::GameTimer::Instance().TotalTime();
		cbPerCamera.DeltaTime = DustEngine::GameTimer::Instance().DeltaTime();

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
				mat2objects.size() * UDX12::Util::CalcConstantBufferByteSize(sizeof(GeometryMaterialConstants))
				+ objectNum * UDX12::Util::CalcConstantBufferByteSize(sizeof(GeometryObjectConstants))
			);
			size_t offset = 0;
			for (const auto& [mat, objects] : mat2objects) {
				GeometryMaterialConstants matC;
				matC.albedoFactor = { 1.f };
				matC.roughnessFactor = 1.f;
				matC.metalnessFactor = 1.f;
				buffer->Set(offset, &matC, sizeof(GeometryMaterialConstants));
				offset += UDX12::Util::CalcConstantBufferByteSize(sizeof(GeometryMaterialConstants));
				for (const auto& object : objects) {
					GeometryObjectConstants objectConstants;
					objectConstants.World = object.l2w;
					buffer->Set(offset, &objectConstants, sizeof(GeometryObjectConstants));
					offset += UDX12::Util::CalcConstantBufferByteSize(sizeof(GeometryObjectConstants));
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
	auto fgRsrcMngr = frameRsrcMngr.GetCurrentFrameResource()->GetResource<std::shared_ptr<UDX12::FG::RsrcMngr>>("FrameGraphRsrcMngr");
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
	auto irradianceMap = fg.RegisterResourceNode("Irradiance Map");
	auto gbPass = fg.RegisterPassNode(
		"GBuffer Pass",
		{},
		{ gbuffer0,gbuffer1,gbuffer2,depthstencil }
	);
	auto irradiancePass = fg.RegisterPassNode(
		"Irradiance Integration",
		{ },
		{ irradianceMap }
	);
	auto deferLightingPass = fg.RegisterPassNode(
		"Defer Lighting",
		{ gbuffer0,gbuffer1,gbuffer2,irradianceMap },
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

	auto srvDesc = UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT);
	auto dsvDesc = UDX12::Desc::DSV::Basic(dsFormat);
	auto rsrcType = UDX12::FG::RsrcType::RT2D(DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, DirectX::Colors::Black);
	const UDX12::FG::RsrcImplDesc_RTV_Null rtvNull;
	// irradiance
	auto irradianceData = frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<std::shared_ptr<IrradianceData>>("irradiance data");

	(*fgRsrcMngr)
		.RegisterTemporalRsrc(gbuffer0, rsrcType)
		.RegisterTemporalRsrc(gbuffer1, rsrcType)
		.RegisterTemporalRsrc(gbuffer2, rsrcType)
		.RegisterTemporalRsrc(depthstencil, { dsClear, dsDesc })
		.RegisterTemporalRsrc(lightedRT, rsrcType)
		.RegisterTemporalRsrc(fullRT, rsrcType)

		.RegisterRsrcTable({
			{gbuffer0,srvDesc},
			{gbuffer1,srvDesc},
			{gbuffer2,srvDesc}
		})

		.RegisterImportedRsrc(presentedRT, { rtb, D3D12_RESOURCE_STATE_PRESENT })
		.RegisterImportedRsrc(irradianceMap, { irradianceData->resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })

		.RegisterPassRsrc(gbPass, gbuffer0, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)
		.RegisterPassRsrc(gbPass, gbuffer1, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)
		.RegisterPassRsrc(gbPass, gbuffer2, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)
		.RegisterPassRsrc(gbPass, depthstencil, D3D12_RESOURCE_STATE_DEPTH_WRITE, dsvDesc)

		.RegisterPassRsrcState(irradiancePass, irradianceMap, D3D12_RESOURCE_STATE_RENDER_TARGET)

		.RegisterPassRsrc(deferLightingPass, gbuffer0, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(deferLightingPass, gbuffer1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(deferLightingPass, gbuffer2, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrcState(deferLightingPass, irradianceMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
		.RegisterPassRsrc(deferLightingPass, lightedRT, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)

		.RegisterPassRsrc(skyboxPass, depthstencil, D3D12_RESOURCE_STATE_DEPTH_READ, dsvDesc)
		.RegisterPassRsrc(skyboxPass, fullRT, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)

		.RegisterPassRsrc(postprocessPass, fullRT, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(postprocessPass, presentedRT, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)
		;

	fgExecutor.RegisterPassFunc(
		gbPass,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);
			cmdList->RSSetViewports(1, &resizeData.screenViewport);
			cmdList->RSSetScissorRects(1, &resizeData.scissorRect);

			auto gb0 = rsrcs.find(gbuffer0)->second;
			auto gb1 = rsrcs.find(gbuffer1)->second;
			auto gb2 = rsrcs.find(gbuffer2)->second;
			auto ds = rsrcs.find(depthstencil)->second;

			std::array rtHandles{
				gb0.info.null_info_rtv.cpuHandle,
				gb1.info.null_info_rtv.cpuHandle,
				gb2.info.null_info_rtv.cpuHandle
			};
			auto dsHandle = ds.info.desc2info_dsv.at(dsvDesc).cpuHandle;
			// Clear the render texture and depth buffer.
			cmdList->ClearRenderTargetView(rtHandles[0], DirectX::Colors::Black, 0, nullptr);
			cmdList->ClearRenderTargetView(rtHandles[1], DirectX::Colors::Black, 0, nullptr);
			cmdList->ClearRenderTargetView(rtHandles[2], DirectX::Colors::Black, 0, nullptr);
			cmdList->ClearDepthStencilView(dsHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(rtHandles.size(), rtHandles.data(), false, &dsHandle);

			cmdList->SetGraphicsRootSignature(RsrcMngrDX12::Instance().GetRootSignature(Impl::ID_RootSignature_geometry));

			auto cbPerCamera = frameRsrcMngr.GetCurrentFrameResource()
				->GetResource<ShaderCBMngrDX12>("ShaderCBMngrDX12")
				.GetCommonBuffer()->GetResource();

			cmdList->SetGraphicsRootConstantBufferView(5, cbPerCamera->GetGPUVirtualAddress());

			DrawObjects(cmdList);
		}
	);

	fgExecutor.RegisterPassFunc(
		irradiancePass,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& /*rsrcs*/) {
			auto data = frameRsrcMngr.GetCurrentFrameResource()
				->GetResource<std::shared_ptr<IrradianceData>>("irradiance data");

			if (data->lastSkybox.ptr == renderContext.skybox.ptr)
				return;
			
			if (renderContext.skybox.ptr == defaultSkybox.ptr) {
				data->lastSkybox.ptr = defaultSkybox.ptr;
				return;
			}

			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);
			
			data->lastSkybox = renderContext.skybox;
			cmdList->SetGraphicsRootSignature(RsrcMngrDX12::Instance().GetRootSignature(Impl::ID_RootSignature_irradiance));
			cmdList->SetPipelineState(RsrcMngrDX12::Instance().GetPSO(ID_PSO_irradiance));

			D3D12_VIEWPORT viewport;
			viewport.MinDepth = 0.f;
			viewport.MaxDepth = 1.f;
			viewport.TopLeftX = 0.f;
			viewport.TopLeftY = 0.f;
			viewport.Width = Impl::IrradianceSize;
			viewport.Height = Impl::IrradianceSize;
			D3D12_RECT rect = { 0,0,Impl::IrradianceSize,Impl::IrradianceSize };
			cmdList->RSSetViewports(1, &viewport);
			cmdList->RSSetScissorRects(1, &rect);

			auto buffer = frameRsrcMngr.GetCurrentFrameResource()
				->GetResource<ShaderCBMngrDX12>("ShaderCBMngrDX12")
				.GetBuffer(irradianceShader);

			cmdList->SetGraphicsRootDescriptorTable(0, renderContext.skybox);
			for (size_t i = 0; i < 6; i++) {
				// Specify the buffers we are going to render to.
				cmdList->OMSetRenderTargets(1, &data->RTVsDH.GetCpuHandle(i), false, nullptr);
				auto address = buffer->GetResource()->GetGPUVirtualAddress()
					+ i * UDX12::Util::CalcConstantBufferByteSize(sizeof(IrradiancePositionLs));
				cmdList->SetGraphicsRootConstantBufferView(1, address);

				cmdList->IASetVertexBuffers(0, 0, nullptr);
				cmdList->IASetIndexBuffer(nullptr);
				cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				cmdList->DrawInstanced(6, 1, 0, 0);
			}
		}
	);

	fgExecutor.RegisterPassFunc(
		deferLightingPass,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);
			cmdList->RSSetViewports(1, &resizeData.screenViewport);
			cmdList->RSSetScissorRects(1, &resizeData.scissorRect);

			auto gb0 = rsrcs.at(gbuffer0);
			auto gb1 = rsrcs.at(gbuffer1);
			auto gb2 = rsrcs.at(gbuffer2);

			auto rt = rsrcs.at(lightedRT);

			//cmdList->CopyResource(bb.resource, rt.resource);

			// Clear the render texture and depth buffer.
			cmdList->ClearRenderTargetView(rt.info.null_info_rtv.cpuHandle, DirectX::Colors::Black, 0, nullptr);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &rt.info.null_info_rtv.cpuHandle, false, nullptr);

			cmdList->SetGraphicsRootSignature(RsrcMngrDX12::Instance().GetRootSignature(Impl::ID_RootSignature_defer_light));
			cmdList->SetPipelineState(RsrcMngrDX12::Instance().GetPSO(ID_PSO_defer_light));

			cmdList->SetGraphicsRootDescriptorTable(0, gb0.info.desc2info_srv.at(srvDesc).gpuHandle);

			if (renderContext.skybox.ptr == defaultSkybox.ptr)
				cmdList->SetGraphicsRootDescriptorTable(1, defaultSkybox);
			else
				cmdList->SetGraphicsRootDescriptorTable(1, irradianceData->SRVDH.GetGpuHandle());

			auto cbLights = frameRsrcMngr.GetCurrentFrameResource()
				->GetResource<ShaderCBMngrDX12>("ShaderCBMngrDX12")
				.GetBuffer(deferShader)->GetResource();
			cmdList->SetGraphicsRootConstantBufferView(2, cbLights->GetGPUVirtualAddress());

			auto cbPerCamera = frameRsrcMngr.GetCurrentFrameResource()
				->GetResource<ShaderCBMngrDX12>("ShaderCBMngrDX12")
				.GetCommonBuffer()->GetResource();
			cmdList->SetGraphicsRootConstantBufferView(3, cbPerCamera->GetGPUVirtualAddress());

			cmdList->IASetVertexBuffers(0, 0, nullptr);
			cmdList->IASetIndexBuffer(nullptr);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(6, 1, 0, 0);
		}
	);

	fgExecutor.RegisterPassFunc(
		skyboxPass,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			if (renderContext.skybox.ptr == defaultSkybox.ptr)
				return;

			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);

			cmdList->SetGraphicsRootSignature(RsrcMngrDX12::Instance().GetRootSignature(Impl::ID_RootSignature_skybox));
			cmdList->SetPipelineState(RsrcMngrDX12::Instance().GetPSO(ID_PSO_skybox));

			cmdList->RSSetViewports(1, &resizeData.screenViewport);
			cmdList->RSSetScissorRects(1, &resizeData.scissorRect);

			auto rt = rsrcs.find(fullRT)->second;
			auto ds = rsrcs.find(depthstencil)->second;

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &rt.info.null_info_rtv.cpuHandle, false, &ds.info.desc2info_dsv.at(dsvDesc).cpuHandle);

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
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			cmdList->SetGraphicsRootSignature(RsrcMngrDX12::Instance().GetRootSignature(Impl::ID_RootSignature_postprocess));
			cmdList->SetPipelineState(RsrcMngrDX12::Instance().GetPSO(ID_PSO_postprocess));

			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);
			cmdList->RSSetViewports(1, &resizeData.screenViewport);
			cmdList->RSSetScissorRects(1, &resizeData.scissorRect);

			auto rt = rsrcs.find(presentedRT)->second;
			auto img = rsrcs.find(fullRT)->second;

			// Clear the render texture and depth buffer.
			cmdList->ClearRenderTargetView(rt.info.null_info_rtv.cpuHandle, DirectX::Colors::Black, 0, nullptr);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &rt.info.null_info_rtv.cpuHandle, false, nullptr);

			cmdList->SetGraphicsRootDescriptorTable(0, img.info.desc2info_srv.at(srvDesc).gpuHandle);

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
	constexpr UINT matCBByteSize = UDX12::Util::CalcConstantBufferByteSize(sizeof(GeometryMaterialConstants));
	constexpr UINT objCBByteSize = UDX12::Util::CalcConstantBufferByteSize(sizeof(GeometryObjectConstants));

	auto& shaderCBMngr = frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<DustEngine::ShaderCBMngrDX12>("ShaderCBMngrDX12");

	auto buffer = shaderCBMngr.GetBuffer(geomrtryShader);

	const auto& mat2objects = renderContext.objectMap.find(geomrtryShader)->second;

	size_t offset = 0;
	for (const auto& [mat, objects] : mat2objects) {
		// For each render item...
		size_t objIdx = 0;
		for (size_t i = 0; i < objects.size(); i++) {
			auto object = objects[i];
			auto& meshGPUBuffer = DustEngine::RsrcMngrDX12::Instance().GetMeshGPUBuffer(object.mesh);
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
			auto albedoHandle = DustEngine::RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(albedo);
			auto roughnessHandle = DustEngine::RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(roughness);
			auto matalnessHandle = DustEngine::RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(metalness);
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
			[](std::shared_ptr<UDX12::FG::RsrcMngr> rsrcMngr) {
				rsrcMngr->Clear();
			}
		);
	}
}
