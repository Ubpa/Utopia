#include <Utopia/Render/DX12/PipelineCommonUtils.h>

#include <Utopia/Render/DX12/ShaderCBMngr.h>
#include <Utopia/Render/DX12/GPURsrcMngrDX12.h>
#include <Utopia/Render/DX12/MeshLayoutMngr.h>

#include <Utopia/Render/Material.h>
#include <Utopia/Render/Shader.h>
#include <Utopia/Render/ShaderMngr.h>
#include <Utopia/Render/Components/Camera.h>
#include <Utopia/Render/Components/MeshFilter.h>
#include <Utopia/Render/Components/MeshRenderer.h>
#include <Utopia/Render/Components/Skybox.h>
#include <Utopia/Render/Components/Light.h>

#include <Utopia/Core/Components/LocalToWorld.h>
#include <Utopia/Core/Components/Translation.h>
#include <Utopia/Core/Components/WorldToLocal.h>
#include <Utopia/Core/Components/PrevLocalToWorld.h>
#include <Utopia/Core/AssetMngr.h>
#include <Utopia/Core/GameTimer.h>

#include <UECS/UECS.hpp>

#include "../_deps/LTCTex.h"

using namespace Ubpa;
using namespace Ubpa::Utopia;
using namespace Ubpa::UECS;

MaterialCBDesc Utopia::RegisterMaterialCB(UDX12::DynamicUploadVector& buffer, const Material& material) {
	const Shader& shader = *material.shader;
	MaterialCBDesc rst;
	rst.beginOffset = buffer.Size();

	auto CalculateSize = [&](ID3D12ShaderReflection* refl) {
		D3D12_SHADER_DESC shaderDesc;
		ThrowIfFailed(refl->GetDesc(&shaderDesc));

		for (UINT i = 0; i < shaderDesc.ConstantBuffers; i++) {
			auto cb = refl->GetConstantBufferByIndex(i);
			D3D12_SHADER_BUFFER_DESC cbDesc;
			ThrowIfFailed(cb->GetDesc(&cbDesc));

			if (PipelineCommonResourceMngr::GetInstance().IsCommonCB(cbDesc.Name))
				continue;

			D3D12_SHADER_INPUT_BIND_DESC rsrcDesc;
			refl->GetResourceBindingDescByName(cbDesc.Name, &rsrcDesc);

			auto target = rst.registerIdx2LocalOffset.find(rsrcDesc.BindPoint);
			if (target != rst.registerIdx2LocalOffset.end())
				continue;

			rst.registerIdx2LocalOffset.emplace(std::pair{ rsrcDesc.BindPoint, rst.size });
			rst.size += UDX12::Util::CalcConstantBufferByteSize(cbDesc.Size);
		}
	};

	for (size_t i = 0; i < shader.passes.size(); i++) {
		CalculateSize(GPURsrcMngrDX12::Instance().GetShaderRefl_vs(shader, i));
		CalculateSize(GPURsrcMngrDX12::Instance().GetShaderRefl_ps(shader, i));
	}

	buffer.Resize(buffer.Size() + rst.size);

	auto UpdateShaderCBsForRefl = [&](std::set<size_t>& flags, const Material& material, ID3D12ShaderReflection* refl) {
		D3D12_SHADER_DESC shaderDesc;
		ThrowIfFailed(refl->GetDesc(&shaderDesc));

		for (UINT i = 0; i < shaderDesc.ConstantBuffers; i++) {
			auto cb = refl->GetConstantBufferByIndex(i);
			D3D12_SHADER_BUFFER_DESC cbDesc;
			ThrowIfFailed(cb->GetDesc(&cbDesc));

			D3D12_SHADER_INPUT_BIND_DESC rsrcDesc;
			refl->GetResourceBindingDescByName(cbDesc.Name, &rsrcDesc);

			if (rst.registerIdx2LocalOffset.find(rsrcDesc.BindPoint) == rst.registerIdx2LocalOffset.end())
				continue;

			if (flags.find(rsrcDesc.BindPoint) != flags.end())
				continue;

			flags.insert(rsrcDesc.BindPoint);

			size_t offset = rst.beginOffset + rst.registerIdx2LocalOffset.at(rsrcDesc.BindPoint);

			for (UINT j = 0; j < cbDesc.Variables; j++) {
				auto var = cb->GetVariableByIndex(j);
				D3D12_SHADER_VARIABLE_DESC varDesc;
				ThrowIfFailed(var->GetDesc(&varDesc));

				auto target = material.properties.find(varDesc.Name);
				if (target == material.properties.end())
					continue;

				std::visit([&](const auto& value) {
					using Value = std::decay_t<decltype(value)>;
					if constexpr (std::is_same_v<Value, bool>) {
						auto v = static_cast<unsigned int>(value);
						assert(varDesc.Size == sizeof(unsigned int));
						buffer.Set(offset + varDesc.StartOffset, &v, sizeof(unsigned int));
					}
					else if constexpr (std::is_same_v<Value, SharedVar<Texture2D>> || std::is_same_v<Value, SharedVar<TextureCube>>)
						assert(false);
					else {
						assert(varDesc.Size == sizeof(Value));
						buffer.Set(offset + varDesc.StartOffset, &value, varDesc.Size);
					}
					}, target->second.value);
			}
		}
	};

	std::set<size_t> flags;
	for (size_t i = 0; i < shader.passes.size(); i++) {
		UpdateShaderCBsForRefl(flags, material, GPURsrcMngrDX12::Instance().GetShaderRefl_vs(shader, i));
		UpdateShaderCBsForRefl(flags, material, GPURsrcMngrDX12::Instance().GetShaderRefl_ps(shader, i));
	}

	return rst;
}

PipelineCommonResourceMngr::PipelineCommonResourceMngr()
	: defaultSkyboxGpuHandle{ 0 }
{}

PipelineCommonResourceMngr& PipelineCommonResourceMngr::GetInstance() {
	static PipelineCommonResourceMngr instance;
	return instance;
}

void PipelineCommonResourceMngr::Init(ID3D12Device* device) {
	errorMat = AssetMngr::Instance().LoadAsset<Material>(LR"(_internal\materials\error.mat)");
	directionalShadowMat = AssetMngr::Instance().LoadAsset<Material>(LR"(_internal\materials\directionalShadow.mat)");

	blackTex2D = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\textures\black.png)");
	whiteTex2D = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\textures\white.png)");
	errorTex2D = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\textures\error.png)");
	normalTex2D = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\textures\normal.png)");
	brdfLutTex2D = AssetMngr::Instance().LoadAsset<Texture2D>(LR"(_internal\textures\BRDFLUT.png)");

	ltcTex2Ds[0] = std::make_shared<Texture2D>();
	ltcTex2Ds[1] = std::make_shared<Texture2D>();
	ltcTex2Ds[0]->image = Image(LTCTex::SIZE, LTCTex::SIZE, 4, LTCTex::data1);
	ltcTex2Ds[1]->image = Image(LTCTex::SIZE, LTCTex::SIZE, 4, LTCTex::data2);
	GPURsrcMngrDX12::Instance().RegisterTexture2D(*ltcTex2Ds[0]);
	GPURsrcMngrDX12::Instance().RegisterTexture2D(*ltcTex2Ds[1]);
	ltcSrvDHA = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(2);
	auto ltc0 = GPURsrcMngrDX12::Instance().GetTexture2DResource(*ltcTex2Ds[0]);
	auto ltc1 = GPURsrcMngrDX12::Instance().GetTexture2DResource(*ltcTex2Ds[1]);
	const auto ltc0SRVDesc = UDX12::Desc::SRV::Tex2D(ltc0->GetDesc().Format);
	const auto ltc1SRVDesc = UDX12::Desc::SRV::Tex2D(ltc1->GetDesc().Format);
	device->CreateShaderResourceView(
		ltc0,
		&ltc0SRVDesc,
		ltcSrvDHA.GetCpuHandle(static_cast<uint32_t>(0))
	);
	device->CreateShaderResourceView(
		ltc1,
		&ltc1SRVDesc,
		ltcSrvDHA.GetCpuHandle(static_cast<uint32_t>(1))
	);

	blackTexCube = AssetMngr::Instance().LoadAsset<TextureCube>(LR"(_internal\textures\blackCube.png)");
	whiteTexCube = AssetMngr::Instance().LoadAsset<TextureCube>(LR"(_internal\textures\whiteCube.png)");

	auto blackRsrc = GPURsrcMngrDX12::Instance().GetTexture2DResource(*blackTex2D);
	auto blackTexCubeRsrc = GPURsrcMngrDX12::Instance().GetTextureCubeResource(*blackTexCube);
	defaultSkyboxGpuHandle = GPURsrcMngrDX12::Instance().GetTextureCubeSrvGpuHandle(*blackTexCube);

	defaultIBLSrvDHA = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(3);
	auto cubeDesc = UDX12::Desc::SRV::TexCube(DXGI_FORMAT_R32G32B32A32_FLOAT);
	auto lutDesc = UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT);
	device->CreateShaderResourceView(blackTexCubeRsrc, &cubeDesc, defaultIBLSrvDHA.GetCpuHandle(0));
	device->CreateShaderResourceView(blackTexCubeRsrc, &cubeDesc, defaultIBLSrvDHA.GetCpuHandle(1));
	device->CreateShaderResourceView(blackRsrc, &lutDesc, defaultIBLSrvDHA.GetCpuHandle(2));

	commonCBs = {
		"StdPipeline_cbPerObject",
		"StdPipeline_cbPerCamera",
		"StdPipeline_cbLightArray",
		"StdPipeline_cbDirectionalShadow",
	};
	const vecf3 origin[6] = {
		{ 1,-1, 1}, // +x right
		{-1,-1,-1}, // -x left
		{-1, 1, 1}, // +y top
		{-1,-1,-1}, // -y buttom
		{-1,-1, 1}, // +z front
		{ 1,-1,-1}, // -z back
	};

	const vecf3 right[6] = {
		{ 0, 0,-2}, // +x
		{ 0, 0, 2}, // -x
		{ 2, 0, 0}, // +y
		{ 2, 0, 0}, // -y
		{ 2, 0, 0}, // +z
		{-2, 0, 0}, // -z
	};

	const vecf3 up[6] = {
		{ 0, 2, 0}, // +x
		{ 0, 2, 0}, // -x
		{ 0, 0,-2}, // +y
		{ 0, 0, 2}, // -y
		{ 0, 2, 0}, // +z
		{ 0, 2, 0}, // -z
	};
	constexpr auto quadPositionsSize = UDX12::Util::CalcConstantBufferByteSize(sizeof(QuadPositionLs));
	constexpr auto mipInfoSize = UDX12::Util::CalcConstantBufferByteSize(sizeof(MipInfo));
	constexpr auto atrousConfigSize = UDX12::Util::CalcConstantBufferByteSize(sizeof(ATrousConfig));
	constexpr auto persistentCBBufferSize = 6 * quadPositionsSize
		+ IBLData::PreFilterMapMipLevels * mipInfoSize
		+ UINT(ATrousN) * atrousConfigSize;
	persistentCBBuffer.emplace(device, persistentCBBufferSize);
	for (size_t i = 0; i < 6; i++) {
		QuadPositionLs positionLs;
		auto p0 = origin[i];
		auto p1 = origin[i] + right[i];
		auto p2 = origin[i] + right[i] + up[i];
		auto p3 = origin[i] + up[i];
		positionLs.positionL4x = { p0[0], p1[0], p2[0], p3[0] };
		positionLs.positionL4y = { p0[1], p1[1], p2[1], p3[1] };
		positionLs.positionL4z = { p0[2], p1[2], p2[2], p3[2] };
		persistentCBBuffer->Set(i * quadPositionsSize, &positionLs, sizeof(QuadPositionLs));
	}
	size_t size = IBLData::PreFilterMapSize;
	for (UINT i = 0; i < IBLData::PreFilterMapMipLevels; i++) {
		MipInfo info;
		info.roughness = i / float(IBLData::PreFilterMapMipLevels - 1);
		info.resolution = (float)size;

		persistentCBBuffer->Set(6 * quadPositionsSize + i * mipInfoSize, &info, sizeof(MipInfo));
		size /= 2;
	}
	for (size_t i = 0; i < ATrousN; i++) {
		ATrousConfig config;
		config.gKernelStep = (int)(i + 1);
		persistentCBBuffer->Set(
			6 * quadPositionsSize + IBLData::PreFilterMapMipLevels * mipInfoSize + i * atrousConfigSize,
			&config,
			sizeof(ATrousConfig));
	}
}

void PipelineCommonResourceMngr::Release() {
	blackTex2D = nullptr;
	whiteTex2D = nullptr;
	errorTex2D = nullptr;
	normalTex2D = nullptr;
	brdfLutTex2D = nullptr;
	ltcTex2Ds[0] = nullptr;
	ltcTex2Ds[1] = nullptr;
	UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(ltcSrvDHA));
	blackTexCube = nullptr;
	whiteTexCube = nullptr;
	errorMat = nullptr;
	directionalShadowMat = nullptr;
	defaultSkyboxGpuHandle.ptr = 0;
	UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(defaultIBLSrvDHA));
	commonCBs.clear();
	persistentCBBuffer.reset();
}

std::shared_ptr<Material> PipelineCommonResourceMngr::GetErrorMaterial() const { return errorMat; }

std::shared_ptr<Material> PipelineCommonResourceMngr::GetDirectionalShadowMaterial() const { return directionalShadowMat; }

D3D12_GPU_DESCRIPTOR_HANDLE PipelineCommonResourceMngr::GetDefaultSkyboxGpuHandle() const { return defaultSkyboxGpuHandle; }

bool PipelineCommonResourceMngr::IsCommonCB(std::string_view cbDescName) const {
	return commonCBs.contains(cbDescName);
}

const UDX12::DescriptorHeapAllocation& PipelineCommonResourceMngr::GetDefaultIBLSrvDHA() const {
	return defaultIBLSrvDHA;
}

std::shared_ptr<Texture2D> PipelineCommonResourceMngr::GetErrorTex2D() const {
	return errorTex2D;
}

std::shared_ptr<Texture2D> PipelineCommonResourceMngr::GetWhiteTex2D() const {
	return whiteTex2D;
}

std::shared_ptr<Texture2D> PipelineCommonResourceMngr::GetBlackTex2D() const {
	return blackTex2D;
}

std::shared_ptr<Texture2D> PipelineCommonResourceMngr::GetNormalTex2D() const {
	return normalTex2D;
}

std::shared_ptr<Texture2D> PipelineCommonResourceMngr::GetBrdfLutTex2D() const {
	return brdfLutTex2D;
}

std::shared_ptr<Texture2D> PipelineCommonResourceMngr::GetLtcTex2D(size_t idx) const {
	assert(idx == 0 || idx == 1);
	return ltcTex2Ds[idx];
}

const UDX12::DescriptorHeapAllocation& PipelineCommonResourceMngr::GetLtcSrvDHA() const {
	return ltcSrvDHA;
}

std::shared_ptr<TextureCube> PipelineCommonResourceMngr::GetBlackTexCube() const {
	return blackTexCube;
}

std::shared_ptr<TextureCube> PipelineCommonResourceMngr::GetWhiteTexCube() const {
	return whiteTexCube;
}

D3D12_GPU_VIRTUAL_ADDRESS PipelineCommonResourceMngr::GetQuadPositionLocalGpuAddress(size_t idx) const {
	assert(idx < 6);
	return persistentCBBuffer->GetResource()->GetGPUVirtualAddress()
		+ idx * UDX12::Util::CalcConstantBufferByteSize(sizeof(QuadPositionLs));
}

D3D12_GPU_VIRTUAL_ADDRESS PipelineCommonResourceMngr::GetMipInfoGpuAddress(size_t idx) const {
	assert(idx < IBLData::PreFilterMapMipLevels);
	return persistentCBBuffer->GetResource()->GetGPUVirtualAddress()
		+ 6 * UDX12::Util::CalcConstantBufferByteSize(sizeof(QuadPositionLs))
		+ idx * UDX12::Util::CalcConstantBufferByteSize(sizeof(MipInfo));
}

D3D12_GPU_VIRTUAL_ADDRESS PipelineCommonResourceMngr::GetATrousConfigGpuAddress(size_t idx) const {
	assert(idx < ATrousN);
	return persistentCBBuffer->GetResource()->GetGPUVirtualAddress()
		+ 6 * UDX12::Util::CalcConstantBufferByteSize(sizeof(QuadPositionLs))
		+ IBLData::PreFilterMapMipLevels * UDX12::Util::CalcConstantBufferByteSize(sizeof(MipInfo))
		+ idx * UDX12::Util::CalcConstantBufferByteSize(sizeof(ATrousConfig));
}

IBLData::~IBLData() {
	if (!RTVsDH.IsNull())
		UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Free(std::move(RTVsDH));
	if (!SRVDH.IsNull())
		UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(SRVDH));
}

void IBLData::Init(ID3D12Device* device) {
	D3D12_CLEAR_VALUE clearColor;
	clearColor.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	clearColor.Color[0] = 0.f;
	clearColor.Color[1] = 0.f;
	clearColor.Color[2] = 0.f;
	clearColor.Color[3] = 1.f;

	RTVsDH = UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Allocate(6 * (1 + IBLData::PreFilterMapMipLevels));
	SRVDH = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(3);
	{ // irradiance
		auto rsrcDesc = UDX12::Desc::RSRC::TextureCube(
			IBLData::IrradianceMapSize, IBLData::IrradianceMapSize,
			1, DXGI_FORMAT_R32G32B32A32_FLOAT
		);
		const auto defaultHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		device->CreateCommittedResource(
			&defaultHeapProp,
			D3D12_HEAP_FLAG_NONE,
			&rsrcDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearColor,
			IID_PPV_ARGS(&irradianceMapResource)
		);
		for (UINT i = 0; i < 6; i++) {
			auto rtvDesc = UDX12::Desc::RTV::Tex2DofTexCube(DXGI_FORMAT_R32G32B32A32_FLOAT, i);
			device->CreateRenderTargetView(irradianceMapResource.Get(), &rtvDesc, RTVsDH.GetCpuHandle(i));
		}
		auto srvDesc = UDX12::Desc::SRV::TexCube(DXGI_FORMAT_R32G32B32A32_FLOAT);
		device->CreateShaderResourceView(irradianceMapResource.Get(), &srvDesc, SRVDH.GetCpuHandle(0));
	}
	{ // prefilter
		auto rsrcDesc = UDX12::Desc::RSRC::TextureCube(
			IBLData::PreFilterMapSize, IBLData::PreFilterMapSize,
			IBLData::PreFilterMapMipLevels, DXGI_FORMAT_R32G32B32A32_FLOAT
		);
		const auto defaultHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		device->CreateCommittedResource(
			&defaultHeapProp,
			D3D12_HEAP_FLAG_NONE,
			&rsrcDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clearColor,
			IID_PPV_ARGS(&prefilterMapResource)
		);
		for (UINT mip = 0; mip < IBLData::PreFilterMapMipLevels; mip++) {
			for (UINT i = 0; i < 6; i++) {
				auto rtvDesc = UDX12::Desc::RTV::Tex2DofTexCube(DXGI_FORMAT_R32G32B32A32_FLOAT, i, mip);
				device->CreateRenderTargetView(
					prefilterMapResource.Get(),
					&rtvDesc,
					RTVsDH.GetCpuHandle(6 * (1 + mip) + i)
				);
			}
		}
		auto srvDesc = UDX12::Desc::SRV::TexCube(DXGI_FORMAT_R32G32B32A32_FLOAT, IBLData::PreFilterMapMipLevels);
		device->CreateShaderResourceView(prefilterMapResource.Get(), &srvDesc, SRVDH.GetCpuHandle(1));
	}
	{// BRDF LUT
		auto brdfLUTTex2D = PipelineCommonResourceMngr::GetInstance().GetBrdfLutTex2D();
		auto brdfLUTTex2DRsrc = GPURsrcMngrDX12::Instance().GetTexture2DResource(*brdfLUTTex2D);
		auto desc = UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32G32_FLOAT);
		device->CreateShaderResourceView(brdfLUTTex2DRsrc, &desc, SRVDH.GetCpuHandle(2));
	}
}

RenderContext Ubpa::Utopia::GenerateRenderContext(size_t ID, std::span<const UECS::World* const> worlds)
{
	auto errorMat = PipelineCommonResourceMngr::GetInstance().GetErrorMaterial();

	auto defaultSkyboxGpuHandle = PipelineCommonResourceMngr::GetInstance().GetDefaultSkyboxGpuHandle();

	RenderContext ctx;

	ctx.ID = ID;

	{ // object
		for (auto world : worlds) {
			world->RunChunkJob(
				[&](ChunkView chunk) {
					auto meshFilters = chunk->GetCmptArray<MeshFilter>();
					auto meshRenderers = chunk->GetCmptArray<MeshRenderer>();
					auto L2Ws = chunk->GetCmptArray<LocalToWorld>();
					auto W2Ls = chunk->GetCmptArray<WorldToLocal>();
					auto prevL2Ws = chunk->GetCmptArray<PrevLocalToWorld>();
					auto entities = chunk->GetEntityArray();

					size_t N = chunk->EntityNum();

					for (size_t i = 0; i < N; i++) {
						auto meshFilter = meshFilters[i];
						auto meshRenderer = meshRenderers[i];

						if (!meshFilter.mesh)
							return;

						RenderObject obj;
						obj.mesh = meshFilter.mesh;
						obj.entity = entities[i];

						size_t M = obj.mesh->GetSubMeshes().size();// std::min(meshRenderer.materials.size(), obj.mesh->GetSubMeshes().size());

						if (M == 0)
							continue;

						bool isDraw = false;

						for (size_t j = 0; j < M; j++) {
							auto material = j < meshRenderer.materials.size() ? meshRenderer.materials[j] : nullptr;
							if (!material || !material->shader)
								obj.material = errorMat;
							else {
								if (material->shader->passes.empty())
									continue;

								obj.material = material;
							}

							obj.translation = L2Ws[i].value.decompose_translation();
							obj.submeshIdx = j;
							for (size_t k = 0; k < obj.material->shader->passes.size(); k++) {
								obj.passIdx = k;
								ctx.renderQueue.Add(obj);
							}
							isDraw = true;
						}

						if (!isDraw)
							continue;

						auto target = ctx.entity2data.find(obj.entity.index);
						if (target != ctx.entity2data.end())
							continue;
						RenderContext::EntityData data;
						data.l2w = L2Ws[i].value;
						data.w2l = !W2Ls.empty() ? W2Ls[i].value : L2Ws[i].value.inverse();
						data.prevl2w = !prevL2Ws.empty() ? prevL2Ws[i].value : L2Ws[i].value;
						data.mesh = meshFilter.mesh;
						data.materials = meshRenderer.materials;
						ctx.entity2data.emplace_hint(target, std::pair{ obj.entity.index, data });

						bboxf3 boungdingBoxWS = L2Ws[i].value * data.mesh->GetBoundingBox();
						ctx.boundingBox.combine_to_self(boungdingBoxWS);
					}
				},
				{ // ArchetypeFilter
					.all = {
						AccessTypeID_of<Latest<MeshFilter>>,
						AccessTypeID_of<Latest<MeshRenderer>>,
						AccessTypeID_of<Latest<LocalToWorld>>,
					}
				},
				false
			);
		}
	}

	{ // light
		std::array<ShaderLight, LightArray::size> dirLights;
		std::array<ShaderLight, LightArray::size> pointLights;
		std::array<ShaderLight, LightArray::size> spotLights;
		std::array<ShaderLight, LightArray::size> rectLights;
		std::array<ShaderLight, LightArray::size> diskLights;
		ctx.lightArray.diectionalLightNum = 0;
		ctx.lightArray.pointLightNum = 0;
		ctx.lightArray.spotLightNum = 0;
		ctx.lightArray.rectLightNum = 0;
		ctx.lightArray.diskLightNum = 0;

		for (auto world : worlds) {
			world->RunEntityJob(
				[&](const Light* light) {
					switch (light->mode)
					{
					case Light::Mode::Directional:
						ctx.lightArray.diectionalLightNum++;
						break;
					case Light::Mode::Point:
						ctx.lightArray.pointLightNum++;
						break;
					case Light::Mode::Spot:
						ctx.lightArray.spotLightNum++;
						break;
					case Light::Mode::Rect:
						ctx.lightArray.rectLightNum++;
						break;
					case Light::Mode::Disk:
						ctx.lightArray.diskLightNum++;
						break;
					default:
						assert("not support" && false);
						break;
					}
				},
				false,
				{ // ArchetypeFilter
					.all = { UECS::AccessTypeID_of<UECS::Latest<LocalToWorld>> }
				}
			);
		}

		size_t offset_diectionalLight = 0;
		size_t offset_pointLight = offset_diectionalLight + ctx.lightArray.diectionalLightNum;
		size_t offset_spotLight = offset_pointLight + ctx.lightArray.pointLightNum;
		size_t offset_rectLight = offset_spotLight + ctx.lightArray.spotLightNum;
		size_t offset_diskLight = offset_rectLight + ctx.lightArray.rectLightNum;
		size_t cur_diectionalLight = 0;
		size_t cur_pointLight = 0;
		size_t cur_spotLight = 0;
		size_t cur_rectLight = 0;
		size_t cur_diskLight = 0;
		for (auto world : worlds) {
			world->RunEntityJob(
				[&](const Light* light, const LocalToWorld* l2w) {
					switch (light->mode)
					{
					case Light::Mode::Directional:
						ctx.lightArray.lights[cur_diectionalLight].color = light->color * light->intensity;
						ctx.lightArray.lights[cur_diectionalLight].dir = (l2w->value * vecf3{ 0,0,1 }).safe_normalize();
						cur_diectionalLight++;
						break;
					case Light::Mode::Point:
						ctx.lightArray.lights[cur_pointLight].color = light->color * light->intensity;
						ctx.lightArray.lights[cur_pointLight].position = l2w->value * pointf3{ 0.f };
						ctx.lightArray.lights[cur_pointLight].range = light->range;
						cur_pointLight++;
						break;
					case Light::Mode::Spot:
						ctx.lightArray.lights[cur_spotLight].color = light->color * light->intensity;
						ctx.lightArray.lights[cur_spotLight].position = l2w->value * pointf3{ 0.f };
						ctx.lightArray.lights[cur_spotLight].dir = (l2w->value * vecf3{ 0,1,0 }).safe_normalize();
						ctx.lightArray.lights[cur_spotLight].range = light->range;
						ctx.lightArray.lights[cur_spotLight].*
							ShaderLight::Spot::pCosHalfInnerSpotAngle = std::cos(to_radian(light->innerSpotAngle) / 2.f);
						ctx.lightArray.lights[cur_spotLight].*
							ShaderLight::Spot::pCosHalfOuterSpotAngle = std::cos(to_radian(light->outerSpotAngle) / 2.f);
						cur_spotLight++;
						break;
					case Light::Mode::Rect:
						ctx.lightArray.lights[cur_rectLight].color = light->color * light->intensity;
						ctx.lightArray.lights[cur_rectLight].position = l2w->value * pointf3{ 0.f };
						ctx.lightArray.lights[cur_rectLight].dir = (l2w->value * vecf3{ 0,1,0 }).safe_normalize();
						ctx.lightArray.lights[cur_rectLight].horizontal = (l2w->value * vecf3{ 1,0,0 }).safe_normalize();
						ctx.lightArray.lights[cur_rectLight].range = light->range;
						ctx.lightArray.lights[cur_rectLight].*
							ShaderLight::Rect::pWidth = light->width;
						ctx.lightArray.lights[cur_rectLight].*
							ShaderLight::Rect::pHeight = light->height;
						cur_rectLight++;
						break;
					case Light::Mode::Disk:
						ctx.lightArray.lights[cur_diskLight].color = light->color * light->intensity;
						ctx.lightArray.lights[cur_diskLight].position = l2w->value * pointf3{ 0.f };
						ctx.lightArray.lights[cur_diskLight].dir = (l2w->value * vecf3{ 0,1,0 }).safe_normalize();
						ctx.lightArray.lights[cur_diskLight].horizontal = (l2w->value * vecf3{ 1,0,0 }).safe_normalize();
						ctx.lightArray.lights[cur_diskLight].range = light->range;
						ctx.lightArray.lights[cur_diskLight].*
							ShaderLight::Disk::pWidth = light->width;
						ctx.lightArray.lights[cur_diskLight].*
							ShaderLight::Disk::pHeight = light->height;
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

	{ // directional shadow
		bool foundDirectionalLight = false;
		for (auto world : worlds) {
			if (foundDirectionalLight)
				break;
			world->RunEntityJob(
				[&](const Light* light, const LocalToWorld* l2w) {
					if (foundDirectionalLight)
						return;
					if (light->mode != Light::Mode::Directional)
						return;
					transformf w2l = l2w->value.inverse();
					bboxf3 boundingBox = w2l * ctx.boundingBox;
					vecf3 diagonal = boundingBox.diagonal();
					vecf3 dir{ 0,0,1 };
					transformf translate(vecf3(0, 0, 0) - (boundingBox.center().as<vecf3>() - dir * diagonal.z * 0.5));
					transformf rotate(quatf::rotate_with<Axis::Y>(to_radian(180.f)));

					ctx.directionalShadow.DirectionalShadowViewProj =
						transformf::orthographic(diagonal.x, diagonal.y, 0, diagonal.z) * rotate * translate * w2l;
				},
				false
			);
		}
	}

	// use first skybox in the world vector
	ctx.skyboxSrvGpuHandle = defaultSkyboxGpuHandle;
	for (auto world : worlds) {
		if (auto ptr = world->entityMngr.ReadSingleton<Skybox>(); ptr && ptr->material) {
			auto target = ptr->material->properties.find("gSkybox");
			if (target != ptr->material->properties.end() && std::holds_alternative<SharedVar<TextureCube>>(target->second.value)) {
				auto texcube = std::get<SharedVar<TextureCube>>(target->second.value);
				ctx.skyboxSrvGpuHandle = GPURsrcMngrDX12::Instance().GetTextureCubeSrvGpuHandle(*texcube);
				break;
			}
		}
	}

	return ctx;
}

void Ubpa::Utopia::DrawObjects(
	const ShaderCBMngr& shaderCBMngr,
	const RenderContext& ctx,
	ID3D12GraphicsCommandList* cmdList,
	std::string_view lightMode,
	size_t rtNum,
	DXGI_FORMAT rtFormat,
	DXGI_FORMAT dsvFormat,
	D3D12_GPU_VIRTUAL_ADDRESS cameraCBAddress,
	D3D12_GPU_DESCRIPTOR_HANDLE iblDataSrvGpuHandle,
	const Material* defaultMaterial)
{
	D3D12_GPU_DESCRIPTOR_HANDLE ibl;
	if (ctx.skyboxSrvGpuHandle.ptr == PipelineCommonResourceMngr::GetInstance().GetDefaultSkyboxGpuHandle().ptr)
		ibl = PipelineCommonResourceMngr::GetInstance().GetDefaultIBLSrvDHA().GetGpuHandle();
	else
		ibl = iblDataSrvGpuHandle;

	if (defaultMaterial)
		cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*defaultMaterial->shader));

	const Shader* currRootSignatureShader = defaultMaterial ? defaultMaterial->shader.get() : nullptr;

	auto Draw = [&](const RenderObject& obj) {
		const Material* material = defaultMaterial ? defaultMaterial : obj.material.get();
		const Shader* shader = material->shader.get();
		// TODO: default pass index
		size_t passIdx = defaultMaterial ? 0 : obj.passIdx;
		const auto& pass = shader->passes[passIdx];

		if (auto target = pass.tags.find("LightMode"); target == pass.tags.end() || target->second != lightMode)
			return;

		if (currRootSignatureShader != shader) {
			currRootSignatureShader = shader;
			cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*currRootSignatureShader));
		}

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = shaderCBMngr.GetObjectCBAddress(ctx.ID, obj.entity.index);

		auto lightCBAddress = shaderCBMngr.GetLightCBAddress(ctx.ID);

		auto directionalShadowCBAddress = shaderCBMngr.GetDirectionalShadowCBAddress(ctx.ID);

		shaderCBMngr.SetGraphicsRoot_CBV_SRV(
			cmdList,
			ctx.ID,
			*material,
			{
				{StdPipeline_cbPerObject, objCBAddress},
				{StdPipeline_cbPerCamera, cameraCBAddress},
				{StdPipeline_cbLightArray, lightCBAddress},
				{StdPipeline_cbDirectionalShadow, directionalShadowCBAddress}
			},
			{
				{StdPipeline_srvIBL, ibl},
				{StdPipeline_srvLTC, PipelineCommonResourceMngr::GetInstance().GetLtcSrvDHA().GetGpuHandle()}
			}
			);

		auto& meshGPUBuffer = GPURsrcMngrDX12::Instance().GetMeshGPUBuffer(*obj.mesh);
		const auto& submesh = obj.mesh->GetSubMeshes().at(obj.submeshIdx);
		const auto meshVBView = meshGPUBuffer.VertexBufferView();
		const auto meshIBView = meshGPUBuffer.IndexBufferView();
		cmdList->IASetVertexBuffers(0, 1, &meshVBView);
		cmdList->IASetIndexBuffer(&meshIBView);
		// submesh.topology
		D3D12_PRIMITIVE_TOPOLOGY d3d12Topology;
		switch (submesh.topology)
		{
		case Ubpa::Utopia::MeshTopology::Triangles:
			d3d12Topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		case Ubpa::Utopia::MeshTopology::Lines:
			d3d12Topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			break;
		case Ubpa::Utopia::MeshTopology::LineStrip:
			d3d12Topology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
			break;
		case Ubpa::Utopia::MeshTopology::Points:
			d3d12Topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
			break;
		default:
			assert(false);
			d3d12Topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
			break;
		}
		cmdList->IASetPrimitiveTopology(d3d12Topology);

		if (pass.renderState.stencilState.enable)
			cmdList->OMSetStencilRef(pass.renderState.stencilState.ref);
		cmdList->SetPipelineState(GPURsrcMngrDX12::Instance().GetOrCreateShaderPSO(
			*shader,
			passIdx,
			MeshLayoutMngr::Instance().GetMeshLayoutID(*obj.mesh),
			rtNum,
			rtFormat
		));
		cmdList->DrawIndexedInstanced((UINT)submesh.indexCount, 1, (UINT)submesh.indexStart, (INT)submesh.baseVertex, 0);
	};

	for (const auto& obj : ctx.renderQueue.GetOpaques())
		Draw(obj);

	for (const auto& obj : ctx.renderQueue.GetTransparents())
		Draw(obj);
}

void Ubpa::Utopia::UpdateCrossFrameCameraResources(
	CameraRsrcMngr& crossFrameCameraRsrcMngr,
	std::span<const IPipeline::CameraData> cameras,
	std::span<ID3D12Resource* const> defaultRTs)
{
	crossFrameCameraRsrcMngr.Update(cameras);

	for (size_t i = 0; i < cameras.size(); ++i) {
		const auto& camera = cameras[i];
		auto desc = defaultRTs[i]->GetDesc();
		size_t defalut_width = desc.Width;
		size_t defalut_height = desc.Height;

		auto& cameraRsrcMap = crossFrameCameraRsrcMngr.Get(camera);
		if (!cameraRsrcMap.Contains(key_CameraConstants))
			cameraRsrcMap.Register(key_CameraConstants, CameraConstants{});
		auto& cameraConstants = cameraRsrcMap.Get<CameraConstants>(key_CameraConstants);
		if (!cameraRsrcMap.Contains(key_CameraJitterIndex))
			cameraRsrcMap.Register(key_CameraJitterIndex, 0);
		auto& cameraJitterIndex = cameraRsrcMap.Get<int>(key_CameraJitterIndex);

		auto cmptCamera = camera.world->entityMngr.ReadComponent<Camera>(camera.entity);
		auto cmptW2L = camera.world->entityMngr.ReadComponent<WorldToLocal>(camera.entity);
		auto cmptTranslation = camera.world->entityMngr.ReadComponent<Translation>(camera.entity);
		size_t rt_width = defalut_width, rt_height = defalut_height;
		if (cmptCamera->renderTarget) {
			rt_width = cmptCamera->renderTarget->image.GetWidth();
			rt_height = cmptCamera->renderTarget->image.GetHeight();
		}

		float SampleX = HaltonSequence2[(cameraJitterIndex % 8)];
		float SampleY = HaltonSequence3[(cameraJitterIndex % 8)];
		cameraJitterIndex++;
		float JitterX = (SampleX * 2.0f - 1.0f) / rt_width;
		float JitterY = (SampleY * 2.0f - 1.0f) / rt_height;
		transformf view = cmptW2L ? cmptW2L->value : transformf::eye();
		transformf proj = cmptCamera->prjectionMatrix;
		transformf unjitteredProj = proj;
		proj[2][0] += JitterX;
		proj[2][1] += JitterY;
		cameraConstants.View = view;
		cameraConstants.InvView = view.inverse();
		cameraConstants.Proj = proj;
		cameraConstants.InvProj = proj.inverse();
		cameraConstants.PrevViewProj = cameraConstants.UnjitteredViewProj;
		cameraConstants.ViewProj = proj * view;
		cameraConstants.UnjitteredViewProj = unjitteredProj * view;
		cameraConstants.InvViewProj = view.inverse() * proj.inverse();
		cameraConstants.EyePosW = cmptTranslation ? cmptTranslation->value.as<pointf3>() : pointf3{ 0.f };
		cameraConstants.FrameCount = cameraConstants.FrameCount + 1;
		cameraConstants.RenderTargetSize = { rt_width, rt_height };
		cameraConstants.InvRenderTargetSize = { 1.0f / rt_width, 1.0f / rt_height };

		cameraConstants.NearZ = cmptCamera->clippingPlaneMin;
		cameraConstants.FarZ = cmptCamera->clippingPlaneMax;
		cameraConstants.TotalTime = GameTimer::Instance().TotalTime();
		cameraConstants.DeltaTime = GameTimer::Instance().DeltaTime();

		cameraConstants.Jitter.x = JitterX;
		cameraConstants.Jitter.y = JitterY;
		cameraConstants.padding0 = 0;
		cameraConstants.padding1 = 0;
	}
}