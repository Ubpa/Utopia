#include <DustEngine/Render/DX12/StdPipeline.h>

#include <DustEngine/Render/DX12/RsrcMngrDX12.h>
#include <DustEngine/Render/DX12/MeshLayoutMngr.h>
#include <DustEngine/Render/DX12/ShaderCBMngrDX12.h>
#include <DustEngine/Render/ShaderMngr.h>
#include <DustEngine/Render/Texture2D.h>
#include <DustEngine/Render/TextureCube.h>
#include <DustEngine/Render/HLSLFile.h>
#include <DustEngine/Render/Shader.h>
#include <DustEngine/Render/Mesh.h>

#include <DustEngine/Asset/AssetMngr.h>

#include <DustEngine/Core/Image.h>
#include <DustEngine/Render/Components/Camera.h>
#include <DustEngine/Render/Components/MeshFilter.h>
#include <DustEngine/Render/Components/MeshRenderer.h>
#include <DustEngine/Render/Components/Skybox.h>
#include <DustEngine/Render/Components/Light.h>
#include <DustEngine/Core/GameTimer.h>

#include <DustEngine/Core/Components/LocalToWorld.h>
#include <DustEngine/Core/Components/Translation.h>
#include <DustEngine/Core/Components/WorldToLocal.h>

#include <UECS/World.h>

#include <_deps/imgui/imgui.h>
#include <_deps/imgui/imgui_impl_win32.h>
#include <_deps/imgui/imgui_impl_dx12.h>

#include <UDX12/FrameResourceMngr.h>

using namespace Ubpa::DustEngine;
using namespace Ubpa::UECS;
using namespace Ubpa;

struct StdPipeline::Impl {
	Impl(InitDesc initDesc)
		:
		initDesc{ initDesc },
		frameRsrcMngr{ initDesc.numFrame, initDesc.device },
		fg { "Standard Pipeline" },
		shaderCBMngr{ initDesc.device }
	{
		BuildTextures();
		BuildFrameResources();
		BuildShaders();
		BuildPSOs();
	}

	~Impl();

	size_t ID_PSO_defer_light;
	size_t ID_PSO_screen;
	size_t ID_PSO_skybox;
	size_t ID_PSO_postprocess;
	size_t ID_PSO_irradiance;
	size_t ID_PSO_prefilter;

	struct ObjectConstants {
		transformf World;
		transformf InvWorld;
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

	struct ShaderLight {
		rgbf color;
		float range;
		vecf3 dir;
		float f0;
		pointf3 position;
		float f1;
		vecf3 horizontal;
		float f2;

		struct Spot {
			static constexpr auto pCosHalfInnerSpotAngle  = &ShaderLight::f0;
			static constexpr auto pCosHalfOuterSpotAngle = &ShaderLight::f1;
		};
		struct Rect {
			static constexpr auto pWidth  = &ShaderLight::f0;
			static constexpr auto pHeight = &ShaderLight::f1;
		};
		struct Disk {
			static constexpr auto pRadius = &ShaderLight::f0;
		};
	};
	struct ShaderLightArray {
		static constexpr size_t size = 16;

		UINT diectionalLightNum;
		UINT pointLightNum;
		UINT spotLightNum;
		UINT rectLightNum;
		UINT diskLightNum;
		UINT _g_cbLightArray_pad0;
		UINT _g_cbLightArray_pad1;
		UINT _g_cbLightArray_pad2;
		ShaderLight lights[size];
	};

	struct QuadPositionLs {
		valf4 positionL4x;
		valf4 positionL4y;
		valf4 positionL4z;
	};

	struct MipInfo {
		float roughness;
		float resolution;
	};

	struct IBLData {
		D3D12_GPU_DESCRIPTOR_HANDLE lastSkybox{ 0 };

		static constexpr size_t IrradianceMapSize = 256;
		static constexpr size_t PreFilterMapSize = 512;
		static constexpr UINT PreFilterMapMipLevels = 5;

		Microsoft::WRL::ComPtr<ID3D12Resource> irradianceMapResource;
		Microsoft::WRL::ComPtr<ID3D12Resource> prefilterMapResource;
		// irradiance map rtv : 0 ~ 5
		// prefilter map rtv  : 6 ~ 6 + 6 * PreFilterMapMipLevels
		UDX12::DescriptorHeapAllocation RTVsDH;
		// irradiance map srv : 0
		// prefilter map rtv  : 1
		// BRDF LUT           : 2
		UDX12::DescriptorHeapAllocation SRVDH;
	};
	UDX12::DescriptorHeapAllocation defaultIBLSRVDH; // 3

	struct RenderContext {
		struct Object {
			const Mesh* mesh{ nullptr };
			size_t submeshIdx{ static_cast<size_t>(-1) };
			size_t entity;
		};
		std::unordered_map<const Shader*,
			std::unordered_map<const Material*,
			std::vector<Object>>> objectMap;

		D3D12_GPU_DESCRIPTOR_HANDLE skybox;
		ShaderLightArray lights;

		// common
		size_t cameraOffset = 0;
		size_t objectOffset = UDX12::Util::CalcConstantBufferByteSize(sizeof(CameraConstants));
		struct EntityData {
			valf<16> l2w;
			valf<16> w2l;
		};
		std::unordered_map<size_t, EntityData> entity2data;
		std::unordered_map<size_t, size_t> entity2offset;
	};

	const InitDesc initDesc;

	RenderContext renderContext;
	D3D12_GPU_DESCRIPTOR_HANDLE defaultSkybox;

	UDX12::FrameResourceMngr frameRsrcMngr;

	UDX12::FG::Executor fgExecutor;
	UFG::Compiler fgCompiler;
	UFG::FrameGraph fg;

	DustEngine::Shader* geomrtryShader;
	DustEngine::Shader* deferShader;
	DustEngine::Shader* skyboxShader;
	DustEngine::Shader* postprocessShader;
	DustEngine::Shader* irradianceShader;
	DustEngine::Shader* prefilterShader;

	DustEngine::ShaderCBMngrDX12 shaderCBMngr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	const DXGI_FORMAT dsFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	void BuildTextures();
	void BuildFrameResources();
	void BuildShaders();
	void BuildPSOs();

	size_t GetGeometryPSO_ID(const Mesh* mesh);
	std::unordered_map<size_t, size_t> PSOIDMap;

	void UpdateRenderContext(const std::vector<const UECS::World*>& worlds);
	void UpdateShaderCBs(const ResizeData& resizeData, const CameraData& cameraData);
	void Render(const ResizeData& resizeData, ID3D12Resource* rtb);
	void DrawObjects(ID3D12GraphicsCommandList*);
};

StdPipeline::Impl::~Impl() {
	for (auto& fr : frameRsrcMngr.GetFrameResources()) {
		auto data = fr->GetResource<std::shared_ptr<IBLData>>("IBL data");
		UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Free(std::move(data->RTVsDH));
		UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(data->SRVDH));
	}
	UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(defaultIBLSRVDH));
}

void StdPipeline::Impl::BuildTextures() {
	auto skyboxBlack = AssetMngr::Instance().LoadAsset<Material>(LR"(..\assets\_internal\materials\skyBlack.mat)");
	auto blackTexCube = skyboxBlack->textureCubes.at("gSkybox");
	auto blackTexCubeRsrc = RsrcMngrDX12::Instance().GetTextureCubeResource(blackTexCube);
	defaultSkybox = RsrcMngrDX12::Instance().GetTextureCubeSrvGpuHandle(blackTexCube);

	auto blackTex2D = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(..\assets\_internal\textures\black.tex2d)");
	auto blackRsrc = RsrcMngrDX12::Instance().GetTexture2DResource(blackTex2D);

	defaultIBLSRVDH = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(3);
	auto cubeDesc = UDX12::Desc::SRV::TexCube(DXGI_FORMAT_R32G32B32A32_FLOAT);
	auto lutDesc = UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT);
	initDesc.device->CreateShaderResourceView(blackTexCubeRsrc, &cubeDesc, defaultIBLSRVDH.GetCpuHandle(0));
	initDesc.device->CreateShaderResourceView(blackTexCubeRsrc, &cubeDesc, defaultIBLSRVDH.GetCpuHandle(1));
	initDesc.device->CreateShaderResourceView(blackRsrc, &lutDesc, defaultIBLSRVDH.GetCpuHandle(2));
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
		auto iblData = std::make_shared<IBLData>();
		iblData->RTVsDH = UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Allocate(6 * (1 + IBLData::PreFilterMapMipLevels));
		iblData->SRVDH = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(3);
		{ // irradiance
			auto rsrcDesc = UDX12::Desc::RSRC::TextureCube(
				IBLData::IrradianceMapSize, IBLData::IrradianceMapSize,
				1, DXGI_FORMAT_R32G32B32A32_FLOAT
			);
			initDesc.device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&rsrcDesc,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				&clearColor,
				IID_PPV_ARGS(&iblData->irradianceMapResource)
			);
			for (UINT i = 0; i < 6; i++) {
				auto rtvDesc = UDX12::Desc::RTV::Tex2DofTexCube(DXGI_FORMAT_R32G32B32A32_FLOAT, i);
				initDesc.device->CreateRenderTargetView(iblData->irradianceMapResource.Get(), &rtvDesc, iblData->RTVsDH.GetCpuHandle(i));
			}
			auto srvDesc = UDX12::Desc::SRV::TexCube(DXGI_FORMAT_R32G32B32A32_FLOAT);
			initDesc.device->CreateShaderResourceView(iblData->irradianceMapResource.Get(), &srvDesc, iblData->SRVDH.GetCpuHandle(0));
		}
		{ // prefilter
			auto rsrcDesc = UDX12::Desc::RSRC::TextureCube(
				IBLData::PreFilterMapSize, IBLData::PreFilterMapSize,
				IBLData::PreFilterMapMipLevels, DXGI_FORMAT_R32G32B32A32_FLOAT
			);
			initDesc.device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&rsrcDesc,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				&clearColor,
				IID_PPV_ARGS(&iblData->prefilterMapResource)
			);
			for (UINT mip = 0; mip < IBLData::PreFilterMapMipLevels; mip++) {
				for (UINT i = 0; i < 6; i++) {
					auto rtvDesc = UDX12::Desc::RTV::Tex2DofTexCube(DXGI_FORMAT_R32G32B32A32_FLOAT, i, mip);
					initDesc.device->CreateRenderTargetView(
						iblData->prefilterMapResource.Get(),
						&rtvDesc,
						iblData->RTVsDH.GetCpuHandle(6 * (1 + mip) + i)
					);
				}
			}
			auto srvDesc = UDX12::Desc::SRV::TexCube(DXGI_FORMAT_R32G32B32A32_FLOAT, IBLData::PreFilterMapMipLevels);
			initDesc.device->CreateShaderResourceView(iblData->prefilterMapResource.Get(), &srvDesc, iblData->SRVDH.GetCpuHandle(1));
		}
		{// BRDF LUT
			auto brdfLUTTex2D = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(..\assets\_internal\textures\BRDFLUT.tex2d)");
			auto brdfLUTTex2DRsrc = RsrcMngrDX12::Instance().GetTexture2DResource(brdfLUTTex2D);
			auto desc = UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32G32_FLOAT);
			initDesc.device->CreateShaderResourceView(brdfLUTTex2DRsrc, &desc, iblData->SRVDH.GetCpuHandle(2));
		}
		fr->RegisterResource("IBL data", iblData);
	}
}

void StdPipeline::Impl::BuildShaders() {
	geomrtryShader = ShaderMngr::Instance().Get("StdPipeline/Geometry");
	deferShader = ShaderMngr::Instance().Get("StdPipeline/Defer Lighting");
	skyboxShader = ShaderMngr::Instance().Get("StdPipeline/Skybox");
	postprocessShader = ShaderMngr::Instance().Get("StdPipeline/Post Process");
	irradianceShader = ShaderMngr::Instance().Get("StdPipeline/Irradiance");
	prefilterShader = ShaderMngr::Instance().Get("StdPipeline/PreFilter");

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

	auto buffer = shaderCBMngr.GetCommonBuffer();
	constexpr auto quadPositionsSize = UDX12::Util::CalcConstantBufferByteSize(sizeof(QuadPositionLs));
	constexpr auto mipInfoSize = UDX12::Util::CalcConstantBufferByteSize(sizeof(MipInfo));
	buffer->FastReserve(6 * quadPositionsSize + IBLData::PreFilterMapMipLevels * mipInfoSize);
	for (size_t i = 0; i < 6; i++) {
		QuadPositionLs positionLs;
		auto p0 = origin[i];
		auto p1 = origin[i] + right[i];
		auto p2 = origin[i] + right[i] + up[i];
		auto p3 = origin[i] + up[i];
		positionLs.positionL4x = { p0[0], p1[0], p2[0], p3[0] };
		positionLs.positionL4y = { p0[1], p1[1], p2[1], p3[1] };
		positionLs.positionL4z = { p0[2], p1[2], p2[2], p3[2] };
		buffer->Set(i * quadPositionsSize, &positionLs, sizeof(QuadPositionLs));
	}
	size_t size = IBLData::PreFilterMapSize;
	for (UINT i = 0; i < IBLData::PreFilterMapMipLevels; i++) {
		MipInfo info;
		info.roughness = i / float(IBLData::PreFilterMapMipLevels - 1);
		info.resolution = (float)size;
		
		buffer->Set(6 * quadPositionsSize + i * mipInfoSize, &info, sizeof(MipInfo));
		size /= 2;
	}
}

void StdPipeline::Impl::BuildPSOs() {
	auto skyboxPsoDesc = UDX12::Desc::PSO::Basic(
		RsrcMngrDX12::Instance().GetShaderRootSignature(skyboxShader),
		nullptr, 0,
		RsrcMngrDX12::Instance().GetShaderByteCode_vs(skyboxShader, 0),
		RsrcMngrDX12::Instance().GetShaderByteCode_ps(skyboxShader, 0),
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
		RsrcMngrDX12::Instance().GetShaderRootSignature(deferShader),
		nullptr, 0,
		RsrcMngrDX12::Instance().GetShaderByteCode_vs(deferShader, 0),
		RsrcMngrDX12::Instance().GetShaderByteCode_ps(deferShader, 0),
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_D24_UNORM_S8_UINT
	);
	deferLightingPsoDesc.RasterizerState.FrontCounterClockwise = TRUE;
	deferLightingPsoDesc.DepthStencilState.DepthEnable = true;
	deferLightingPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	deferLightingPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
	ID_PSO_defer_light = RsrcMngrDX12::Instance().RegisterPSO(&deferLightingPsoDesc);

	auto postprocessPsoDesc = UDX12::Desc::PSO::Basic(
		RsrcMngrDX12::Instance().GetShaderRootSignature(postprocessShader),
		nullptr, 0,
		RsrcMngrDX12::Instance().GetShaderByteCode_vs(postprocessShader, 0),
		RsrcMngrDX12::Instance().GetShaderByteCode_ps(postprocessShader, 0),
		initDesc.rtFormat,
		DXGI_FORMAT_UNKNOWN
	);
	postprocessPsoDesc.RasterizerState.FrontCounterClockwise = TRUE;
	postprocessPsoDesc.DepthStencilState.DepthEnable = false;
	postprocessPsoDesc.DepthStencilState.StencilEnable = false;
	ID_PSO_postprocess = RsrcMngrDX12::Instance().RegisterPSO(&postprocessPsoDesc);

	{
		auto desc = UDX12::Desc::PSO::Basic(
			RsrcMngrDX12::Instance().GetShaderRootSignature(irradianceShader),
			nullptr, 0,
			RsrcMngrDX12::Instance().GetShaderByteCode_vs(irradianceShader, 0),
			RsrcMngrDX12::Instance().GetShaderByteCode_ps(irradianceShader, 0),
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			DXGI_FORMAT_UNKNOWN
		);
		desc.RasterizerState.FrontCounterClockwise = TRUE;
		desc.DepthStencilState.DepthEnable = false;
		desc.DepthStencilState.StencilEnable = false;
		ID_PSO_irradiance = RsrcMngrDX12::Instance().RegisterPSO(&desc);
	}

	{
		auto desc = UDX12::Desc::PSO::Basic(
			RsrcMngrDX12::Instance().GetShaderRootSignature(prefilterShader),
			nullptr, 0,
			RsrcMngrDX12::Instance().GetShaderByteCode_vs(prefilterShader, 0),
			RsrcMngrDX12::Instance().GetShaderByteCode_ps(prefilterShader, 0),
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			DXGI_FORMAT_UNKNOWN
		);
		desc.RasterizerState.FrontCounterClockwise = TRUE;
		desc.DepthStencilState.DepthEnable = false;
		desc.DepthStencilState.StencilEnable = false;
		ID_PSO_prefilter = RsrcMngrDX12::Instance().RegisterPSO(&desc);
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
			RsrcMngrDX12::Instance().GetShaderRootSignature(geomrtryShader),
			layout.data(), (UINT)layout.size(),
			RsrcMngrDX12::Instance().GetShaderByteCode_vs(geomrtryShader, 0),
			RsrcMngrDX12::Instance().GetShaderByteCode_ps(geomrtryShader, 0),
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

void StdPipeline::Impl::UpdateRenderContext(const std::vector<const UECS::World*>& worlds) {
	renderContext.objectMap.clear();
	renderContext.entity2data.clear();
	renderContext.entity2offset.clear();

	{ // object
		ArchetypeFilter filter;
		filter.all = {
			CmptAccessType::Of<Latest<MeshFilter>>,
			CmptAccessType::Of<Latest<MeshRenderer>>,
			CmptAccessType::Of<Latest<LocalToWorld>>,
		};
		for (auto world : worlds) {
			const_cast<UECS::World*>(world)->RunChunkJob(
				[&](ChunkView chunk) {
					auto meshFilters = chunk.GetCmptArray<MeshFilter>();
					auto meshRenderers = chunk.GetCmptArray<MeshRenderer>();
					auto L2Ws = chunk.GetCmptArray<LocalToWorld>();
					auto W2Ls = chunk.GetCmptArray<WorldToLocal>();
					auto entities = chunk.GetEntityArray();

					size_t N = chunk.EntityNum();

					for (size_t i = 0; i < N; i++) {
						auto meshFilter = meshFilters[i];
						auto meshRenderer = meshRenderers[i];

						if (!meshFilter.mesh)
							return;

						RenderContext::Object obj;
						obj.mesh = meshFilter.mesh;
						obj.entity = entities[i].Idx();

						size_t M = std::min(meshRenderer.materials.size(), obj.mesh->GetSubMeshes().size());

						if(M == 0)
							continue;

						bool isDraw = false;

						for (size_t j = 0; j < M; j++) {
							auto material = meshRenderer.materials[j];
							if (!material || !material->shader)
								continue;
							obj.submeshIdx = j;
							renderContext.objectMap[material->shader][material].push_back(obj);
							isDraw = true;
						}

						if(!isDraw)
							continue;

						auto target = renderContext.entity2data.find(obj.entity);
						if (target != renderContext.entity2data.end())
							continue;
						RenderContext::EntityData data;
						data.l2w = L2Ws[i].value;
						data.w2l = W2Ls ? W2Ls[i].value : L2Ws[i].value.inverse();
						renderContext.entity2data.emplace_hint(target, std::pair{ obj.entity, data });
					}
				},
				filter,
				false
			);
		}
	}

	{ // light
		std::array<ShaderLight, ShaderLightArray::size> dirLights;
		std::array<ShaderLight, ShaderLightArray::size> pointLights;
		std::array<ShaderLight, ShaderLightArray::size> spotLights;
		std::array<ShaderLight, ShaderLightArray::size> rectLights;
		std::array<ShaderLight, ShaderLightArray::size> diskLights;
		renderContext.lights.diectionalLightNum = 0;
		renderContext.lights.pointLightNum = 0;
		renderContext.lights.spotLightNum = 0;
		renderContext.lights.rectLightNum = 0;
		renderContext.lights.diskLightNum = 0;

		for (auto world : worlds) {
			const_cast<UECS::World*>(world)->RunEntityJob(
				[&](const Light* light) {
					switch (light->type)
					{
					case Ubpa::DustEngine::LightType::Directional:
						renderContext.lights.diectionalLightNum++;
						break;
					case Ubpa::DustEngine::LightType::Point:
						renderContext.lights.pointLightNum++;
						break;
					case Ubpa::DustEngine::LightType::Spot:
						renderContext.lights.spotLightNum++;
						break;
					case Ubpa::DustEngine::LightType::Rect:
						renderContext.lights.rectLightNum++;
						break;
					case Ubpa::DustEngine::LightType::Disk:
						renderContext.lights.diskLightNum++;
						break;
					default:
						assert("not support" && false);
						break;
					}
				},
				false
			);
		}
		
		size_t offset_diectionalLight = 0;
		size_t offset_pointLight = offset_diectionalLight + renderContext.lights.diectionalLightNum;
		size_t offset_spotLight = offset_pointLight + renderContext.lights.pointLightNum;
		size_t offset_rectLight = offset_spotLight + renderContext.lights.spotLightNum;
		size_t offset_diskLight = offset_rectLight + renderContext.lights.rectLightNum;
		size_t cur_diectionalLight = 0;
		size_t cur_pointLight = 0;
		size_t cur_spotLight = 0;
		size_t cur_rectLight = 0;
		size_t cur_diskLight = 0;
		for (auto world : worlds) {
			const_cast<UECS::World*>(world)->RunEntityJob(
				[&](const Light* light, const LocalToWorld* l2w) {
					switch (light->type)
					{
					case Ubpa::DustEngine::LightType::Directional:
						renderContext.lights.lights[cur_diectionalLight].color = light->color * light->intensity;
						renderContext.lights.lights[cur_diectionalLight].dir = (l2w->value * vecf3{ 0,0,1 }).normalize();
						cur_diectionalLight++;
						break;
					case Ubpa::DustEngine::LightType::Point:
						renderContext.lights.lights[cur_pointLight].color = light->color * light->intensity;
						renderContext.lights.lights[cur_pointLight].position = l2w->value * pointf3{ 0.f };
						renderContext.lights.lights[cur_pointLight].range = light->range;
						cur_pointLight++;
						break;
					case Ubpa::DustEngine::LightType::Spot:
						renderContext.lights.lights[cur_spotLight].color = light->color * light->intensity;
						renderContext.lights.lights[cur_spotLight].position = l2w->value * pointf3{ 0.f };
						renderContext.lights.lights[cur_spotLight].dir = (l2w->value * vecf3{ 0,0,1 }).normalize();
						renderContext.lights.lights[cur_spotLight].range = light->range;
						renderContext.lights.lights[cur_spotLight].*
							ShaderLight::Spot::pCosHalfInnerSpotAngle = std::cos(to_radian(light->innerSpotAngle) / 2.f);
						renderContext.lights.lights[cur_spotLight].*
							ShaderLight::Spot::pCosHalfOuterSpotAngle = std::cos(to_radian(light->outerSpotAngle) / 2.f);
						cur_spotLight++;
						break;
					case Ubpa::DustEngine::LightType::Rect:
						renderContext.lights.lights[cur_rectLight].color = light->color * light->intensity;
						renderContext.lights.lights[cur_rectLight].position = l2w->value * pointf3{ 0.f };
						renderContext.lights.lights[cur_rectLight].dir = (l2w->value * vecf3{ 0,0,1 }).normalize();
						renderContext.lights.lights[cur_rectLight].horizontal = (l2w->value * vecf3{ 1,0,0 }).normalize();
						renderContext.lights.lights[cur_rectLight].range = light->range;
						renderContext.lights.lights[cur_rectLight].*
							ShaderLight::Rect::pWidth = light->width;
						renderContext.lights.lights[cur_rectLight].*
							ShaderLight::Rect::pHeight = light->height;
						cur_rectLight++;
						break;
					case Ubpa::DustEngine::LightType::Disk:
						renderContext.lights.lights[cur_diskLight].color = light->color * light->intensity;
						renderContext.lights.lights[cur_diskLight].position = l2w->value * pointf3{ 0.f };
						renderContext.lights.lights[cur_diskLight].dir = (l2w->value * vecf3{ 0,0,1 }).normalize();
						renderContext.lights.lights[cur_diskLight].range = light->range;
						renderContext.lights.lights[cur_diskLight].*
							ShaderLight::Disk::pRadius = light->radius;
						cur_diskLight++;
						break;
					default:
						break;
					}
				},
				false
			);
		}
		
	}

	// use first skybox in the world vector
	renderContext.skybox = defaultSkybox;
	for (auto world : worlds) {
		if (auto ptr = world->entityMngr.GetSingleton<Skybox>(); ptr && ptr->material && ptr->material->shader == skyboxShader) {
			auto target = ptr->material->textureCubes.find("gSkybox");
			if (target != ptr->material->textureCubes.end()) {
				renderContext.skybox = RsrcMngrDX12::Instance().GetTextureCubeSrvGpuHandle(target->second);
				break;
			}
		}
	}
}

void StdPipeline::Impl::UpdateShaderCBs(
	const ResizeData& resizeData,
	const CameraData& cameraData
) {
	auto& shaderCBMngr = frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<DustEngine::ShaderCBMngrDX12>("ShaderCBMngrDX12");

	{ // defer lighting
		auto buffer = shaderCBMngr.GetBuffer(deferShader);
		buffer->FastReserve(UDX12::Util::CalcConstantBufferByteSize(sizeof(ShaderLightArray)));
		buffer->Set(0, &renderContext.lights, sizeof(ShaderLightArray));
	}

	{ // camera, objects
		auto buffer = shaderCBMngr.GetCommonBuffer();
		buffer->FastReserve(
			UDX12::Util::CalcConstantBufferByteSize(sizeof(CameraConstants))
			+ renderContext.entity2data.size() * UDX12::Util::CalcConstantBufferByteSize(sizeof(ObjectConstants))
		);

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

		buffer->Set(renderContext.cameraOffset, &cbPerCamera, sizeof(CameraConstants));

		size_t offset = renderContext.objectOffset;
		for (auto& [idx, data] : renderContext.entity2data) {
			ObjectConstants objectConstants;
			objectConstants.World = data.l2w;
			objectConstants.InvWorld = data.w2l;
			renderContext.entity2offset[idx] = offset;
			buffer->Set(offset, &objectConstants, sizeof(ObjectConstants));
			offset += UDX12::Util::CalcConstantBufferByteSize(sizeof(ObjectConstants));
		}
	}

	// geometry
	for (const auto& [shader, mat2objects] : renderContext.objectMap) {
		if (shader->shaderName == "StdPipeline/Geometry") {
			auto buffer = shaderCBMngr.GetBuffer(shader);
			buffer->FastReserve(
				mat2objects.size() * UDX12::Util::CalcConstantBufferByteSize(sizeof(GeometryMaterialConstants))
			);
			size_t offset = 0;
			for (const auto& [mat, objects] : mat2objects) {
				GeometryMaterialConstants matC;
				matC.albedoFactor = { 1.f };
				matC.roughnessFactor = 1.f;
				matC.metalnessFactor = 1.f;
				buffer->Set(offset, &matC, sizeof(GeometryMaterialConstants));
				offset += UDX12::Util::CalcConstantBufferByteSize(sizeof(GeometryMaterialConstants));
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
	auto lightedRT = fg.RegisterResourceNode("Lighted");
	auto resultRT = fg.RegisterResourceNode("Result");
	auto presentedRT = fg.RegisterResourceNode("Present");
	fg.RegisterMoveNode(resultRT, lightedRT);
	auto depthstencil = fg.RegisterResourceNode("Depth Stencil");
	auto irradianceMap = fg.RegisterResourceNode("Irradiance Map");
	auto prefilterMap = fg.RegisterResourceNode("PreFilter Map");
	auto gbPass = fg.RegisterPassNode(
		"GBuffer Pass",
		{},
		{ gbuffer0,gbuffer1,gbuffer2,depthstencil }
	);
	auto iblPass = fg.RegisterPassNode(
		"IBL",
		{ },
		{ irradianceMap, prefilterMap }
	);
	auto deferLightingPass = fg.RegisterPassNode(
		"Defer Lighting",
		{ gbuffer0,gbuffer1,gbuffer2,depthstencil,irradianceMap,prefilterMap },
		{ lightedRT }
	);
	auto skyboxPass = fg.RegisterPassNode(
		"Skybox",
		{ depthstencil },
		{ resultRT }
	);
	auto postprocessPass = fg.RegisterPassNode(
		"Post Process",
		{ resultRT },
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
	auto dsSrvDesc = UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
	auto dsvDesc = UDX12::Desc::DSV::Basic(dsFormat);
	auto rsrcType = UDX12::FG::RsrcType::RT2D(DXGI_FORMAT_R32G32B32A32_FLOAT, width, (UINT)height, DirectX::Colors::Black);
	const UDX12::FG::RsrcImplDesc_RTV_Null rtvNull;
	
	auto iblData = frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<std::shared_ptr<IBLData>>("IBL data");

	(*fgRsrcMngr)
		.RegisterTemporalRsrc(gbuffer0, rsrcType)
		.RegisterTemporalRsrc(gbuffer1, rsrcType)
		.RegisterTemporalRsrc(gbuffer2, rsrcType)
		.RegisterTemporalRsrc(depthstencil, { dsClear, dsDesc })
		.RegisterTemporalRsrc(lightedRT, rsrcType)
		.RegisterTemporalRsrc(resultRT, rsrcType)

		.RegisterRsrcTable({
			{gbuffer0,srvDesc},
			{gbuffer1,srvDesc},
			{gbuffer2,srvDesc}
		})

		.RegisterImportedRsrc(presentedRT, { rtb, D3D12_RESOURCE_STATE_PRESENT })
		.RegisterImportedRsrc(irradianceMap, { iblData->irradianceMapResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })
		.RegisterImportedRsrc(prefilterMap, { iblData->prefilterMapResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })

		.RegisterPassRsrc(gbPass, gbuffer0, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)
		.RegisterPassRsrc(gbPass, gbuffer1, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)
		.RegisterPassRsrc(gbPass, gbuffer2, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)
		.RegisterPassRsrc(gbPass, depthstencil, D3D12_RESOURCE_STATE_DEPTH_WRITE, dsvDesc)

		.RegisterPassRsrcState(iblPass, irradianceMap, D3D12_RESOURCE_STATE_RENDER_TARGET)
		.RegisterPassRsrcState(iblPass, prefilterMap, D3D12_RESOURCE_STATE_RENDER_TARGET)

		.RegisterPassRsrc(deferLightingPass, gbuffer0, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(deferLightingPass, gbuffer1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrc(deferLightingPass, gbuffer2, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
		.RegisterPassRsrcState(deferLightingPass, depthstencil, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_DEPTH_READ)
		.RegisterPassRsrcImplDesc(deferLightingPass, depthstencil, dsvDesc)
		.RegisterPassRsrcImplDesc(deferLightingPass, depthstencil, dsSrvDesc)
		.RegisterPassRsrcState(deferLightingPass, irradianceMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
		.RegisterPassRsrcState(deferLightingPass, prefilterMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
		.RegisterPassRsrc(deferLightingPass, lightedRT, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)

		.RegisterPassRsrc(skyboxPass, depthstencil, D3D12_RESOURCE_STATE_DEPTH_READ, dsvDesc)
		.RegisterPassRsrc(skyboxPass, resultRT, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)

		.RegisterPassRsrc(postprocessPass, resultRT, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, srvDesc)
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
			cmdList->OMSetRenderTargets((UINT)rtHandles.size(), rtHandles.data(), false, &dsHandle);

			cmdList->SetGraphicsRootSignature(RsrcMngrDX12::Instance().GetShaderRootSignature(geomrtryShader));

			auto cbPerCamera = frameRsrcMngr.GetCurrentFrameResource()
				->GetResource<ShaderCBMngrDX12>("ShaderCBMngrDX12")
				.GetCommonBuffer()->GetResource();

			cmdList->SetGraphicsRootConstantBufferView(6, cbPerCamera->GetGPUVirtualAddress());

			DrawObjects(cmdList);
		}
	);

	fgExecutor.RegisterPassFunc(
		iblPass,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& /*rsrcs*/) {
			if (iblData->lastSkybox.ptr == renderContext.skybox.ptr)
				return;
			
			if (renderContext.skybox.ptr == defaultSkybox.ptr) {
				iblData->lastSkybox.ptr = defaultSkybox.ptr;
				return;
			}
			iblData->lastSkybox = renderContext.skybox;

			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);
			
			{ // irradiance
				cmdList->SetGraphicsRootSignature(RsrcMngrDX12::Instance().GetShaderRootSignature(irradianceShader));
				cmdList->SetPipelineState(RsrcMngrDX12::Instance().GetPSO(ID_PSO_irradiance));

				D3D12_VIEWPORT viewport;
				viewport.MinDepth = 0.f;
				viewport.MaxDepth = 1.f;
				viewport.TopLeftX = 0.f;
				viewport.TopLeftY = 0.f;
				viewport.Width = Impl::IBLData::IrradianceMapSize;
				viewport.Height = Impl::IBLData::IrradianceMapSize;
				D3D12_RECT rect = { 0,0,Impl::IBLData::IrradianceMapSize,Impl::IBLData::IrradianceMapSize };
				cmdList->RSSetViewports(1, &viewport);
				cmdList->RSSetScissorRects(1, &rect);

				auto buffer = shaderCBMngr.GetCommonBuffer();

				cmdList->SetGraphicsRootDescriptorTable(0, renderContext.skybox);
				for (UINT i = 0; i < 6; i++) {
					// Specify the buffers we are going to render to.
					cmdList->OMSetRenderTargets(1, &iblData->RTVsDH.GetCpuHandle(i), false, nullptr);
					auto address = buffer->GetResource()->GetGPUVirtualAddress()
						+ i * UDX12::Util::CalcConstantBufferByteSize(sizeof(QuadPositionLs));
					cmdList->SetGraphicsRootConstantBufferView(1, address);

					cmdList->IASetVertexBuffers(0, 0, nullptr);
					cmdList->IASetIndexBuffer(nullptr);
					cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					cmdList->DrawInstanced(6, 1, 0, 0);
				}
			}

			{ // prefilter
				cmdList->SetGraphicsRootSignature(RsrcMngrDX12::Instance().GetShaderRootSignature(prefilterShader));
				cmdList->SetPipelineState(RsrcMngrDX12::Instance().GetPSO(ID_PSO_prefilter));

				auto buffer = shaderCBMngr.GetCommonBuffer();

				cmdList->SetGraphicsRootDescriptorTable(0, renderContext.skybox);
				size_t size = Impl::IBLData::PreFilterMapSize;
				for (UINT mip = 0; mip < Impl::IBLData::PreFilterMapMipLevels; mip++) {
					auto mipinfo = buffer->GetResource()->GetGPUVirtualAddress()
						+ 6 * UDX12::Util::CalcConstantBufferByteSize(sizeof(QuadPositionLs))
						+ mip * UDX12::Util::CalcConstantBufferByteSize(sizeof(MipInfo));
					cmdList->SetGraphicsRootConstantBufferView(2, mipinfo);

					D3D12_VIEWPORT viewport;
					viewport.MinDepth = 0.f;
					viewport.MaxDepth = 1.f;
					viewport.TopLeftX = 0.f;
					viewport.TopLeftY = 0.f;
					viewport.Width = (float)size;
					viewport.Height = (float)size;
					D3D12_RECT rect = { 0,0,(LONG)size,(LONG)size };
					cmdList->RSSetViewports(1, &viewport);
					cmdList->RSSetScissorRects(1, &rect);

					for (UINT i = 0; i < 6; i++) {
						auto positionLs = buffer->GetResource()->GetGPUVirtualAddress()
							+ i * UDX12::Util::CalcConstantBufferByteSize(sizeof(QuadPositionLs));
						cmdList->SetGraphicsRootConstantBufferView(1, positionLs);

						// Specify the buffers we are going to render to.
						cmdList->OMSetRenderTargets(1, &iblData->RTVsDH.GetCpuHandle(6 * (1 + mip) + i), false, nullptr);

						cmdList->IASetVertexBuffers(0, 0, nullptr);
						cmdList->IASetIndexBuffer(nullptr);
						cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
						cmdList->DrawInstanced(6, 1, 0, 0);
					}

					size /= 2;
				}
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
			auto ds = rsrcs.find(depthstencil)->second;

			auto rt = rsrcs.at(lightedRT);

			//cmdList->CopyResource(bb.resource, rt.resource);

			// Clear the render texture and depth buffer.
			cmdList->ClearRenderTargetView(rt.info.null_info_rtv.cpuHandle, DirectX::Colors::Black, 0, nullptr);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &rt.info.null_info_rtv.cpuHandle, false, &ds.info.desc2info_dsv.at(dsvDesc).cpuHandle);

			cmdList->SetGraphicsRootSignature(RsrcMngrDX12::Instance().GetShaderRootSignature(deferShader));
			cmdList->SetPipelineState(RsrcMngrDX12::Instance().GetPSO(ID_PSO_defer_light));

			cmdList->SetGraphicsRootDescriptorTable(0, gb0.info.desc2info_srv.at(srvDesc).gpuHandle);
			cmdList->SetGraphicsRootDescriptorTable(1, ds.info.desc2info_srv.at(dsSrvDesc).gpuHandle);

			// irradiance, prefilter, BRDF LUT
			if (renderContext.skybox.ptr == defaultSkybox.ptr)
				cmdList->SetGraphicsRootDescriptorTable(2, defaultIBLSRVDH.GetGpuHandle());
			else
				cmdList->SetGraphicsRootDescriptorTable(2, iblData->SRVDH.GetGpuHandle());

			auto cbLights = frameRsrcMngr.GetCurrentFrameResource()
				->GetResource<ShaderCBMngrDX12>("ShaderCBMngrDX12")
				.GetBuffer(deferShader)->GetResource();
			cmdList->SetGraphicsRootConstantBufferView(3, cbLights->GetGPUVirtualAddress());

			auto cbPerCamera = frameRsrcMngr.GetCurrentFrameResource()
				->GetResource<ShaderCBMngrDX12>("ShaderCBMngrDX12")
				.GetCommonBuffer()->GetResource();
			cmdList->SetGraphicsRootConstantBufferView(4, cbPerCamera->GetGPUVirtualAddress());

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

			cmdList->SetGraphicsRootSignature(RsrcMngrDX12::Instance().GetShaderRootSignature(skyboxShader));
			cmdList->SetPipelineState(RsrcMngrDX12::Instance().GetPSO(ID_PSO_skybox));

			cmdList->RSSetViewports(1, &resizeData.screenViewport);
			cmdList->RSSetScissorRects(1, &resizeData.scissorRect);

			auto rt = rsrcs.find(resultRT)->second;
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
			cmdList->SetGraphicsRootSignature(RsrcMngrDX12::Instance().GetShaderRootSignature(postprocessShader));
			cmdList->SetPipelineState(RsrcMngrDX12::Instance().GetPSO(ID_PSO_postprocess));

			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);
			cmdList->RSSetViewports(1, &resizeData.screenViewport);
			cmdList->RSSetScissorRects(1, &resizeData.scissorRect);

			auto rt = rsrcs.find(presentedRT)->second;
			auto img = rsrcs.find(resultRT)->second;

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

	auto& shaderCBMngr = frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<DustEngine::ShaderCBMngrDX12>("ShaderCBMngrDX12");

	auto matBuffer = shaderCBMngr.GetBuffer(geomrtryShader);
	auto commonBuffer = shaderCBMngr.GetCommonBuffer();

	const auto& mat2objects = renderContext.objectMap.find(geomrtryShader)->second;

	size_t matOffset = 0;
	for (const auto& [mat, objects] : mat2objects) {
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress =
			matBuffer->GetResource()->GetGPUVirtualAddress()
			+ matOffset;
		// For each render item...
		for (size_t i = 0; i < objects.size(); i++) {
			const auto& object = objects[i];
			auto& meshGPUBuffer = DustEngine::RsrcMngrDX12::Instance().GetMeshGPUBuffer(object.mesh);
			const auto& submesh = object.mesh->GetSubMeshes().at(object.submeshIdx);
			cmdList->IASetVertexBuffers(0, 1, &meshGPUBuffer.VertexBufferView());
			cmdList->IASetIndexBuffer(&meshGPUBuffer.IndexBufferView());
			// submesh.topology
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			D3D12_GPU_VIRTUAL_ADDRESS objCBAddress =
				commonBuffer->GetResource()->GetGPUVirtualAddress()
				+ renderContext.entity2offset.at(object.entity);

			auto albedo = mat->texture2Ds.find("gAlbedoMap")->second;
			auto roughness = mat->texture2Ds.find("gRoughnessMap")->second;
			auto metalness = mat->texture2Ds.find("gMetalnessMap")->second;
			auto normalmap = mat->texture2Ds.find("gNormalMap")->second;
			auto albedoHandle = DustEngine::RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(albedo);
			auto roughnessHandle = DustEngine::RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(roughness);
			auto matalnessHandle = DustEngine::RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(metalness);
			auto normalHandle = DustEngine::RsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(normalmap);
			cmdList->SetGraphicsRootDescriptorTable(0, albedoHandle);
			cmdList->SetGraphicsRootDescriptorTable(1, roughnessHandle);
			cmdList->SetGraphicsRootDescriptorTable(2, matalnessHandle);
			cmdList->SetGraphicsRootDescriptorTable(3, normalHandle);
			cmdList->SetGraphicsRootConstantBufferView(4, objCBAddress);
			cmdList->SetGraphicsRootConstantBufferView(5, matCBAddress);

			cmdList->SetPipelineState(RsrcMngrDX12::Instance().GetPSO(GetGeometryPSO_ID(object.mesh)));
			cmdList->DrawIndexedInstanced((UINT)submesh.indexCount, 1, (UINT)submesh.indexStart, (INT)submesh.baseVertex, 0);
		}
		matOffset += matCBByteSize;
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

void StdPipeline::BeginFrame(const std::vector<const UECS::World*>& worlds, const CameraData& cameraData) {
	// collect some cpu data
	pImpl->UpdateRenderContext(worlds);

	// Cycle through the circular frame resource array.
	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	pImpl->frameRsrcMngr.BeginFrame();

	// cpu -> gpu
	pImpl->UpdateShaderCBs(GetResizeData(), cameraData);
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
