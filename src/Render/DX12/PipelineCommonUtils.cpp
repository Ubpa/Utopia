#include <Utopia/Render/DX12/PipelineCommonUtils.h>

#include <Utopia/Render/Material.h>
#include <Utopia/Render/Shader.h>

#include <Utopia/Render/DX12/GPURsrcMngrDX12.h>
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

#include <UECS/UECS.hpp>

#include "../_deps/LTCTex.h"

using namespace Ubpa;
using namespace Ubpa::Utopia;
using namespace Ubpa::UECS;

PipelineCommonResourceMngr::PipelineCommonResourceMngr()
	: defaultSkyboxGpuHandle{ 0 }
{}

PipelineCommonResourceMngr& PipelineCommonResourceMngr::GetInstance() {
	static PipelineCommonResourceMngr instance;
	return instance;
}

void PipelineCommonResourceMngr::Init(ID3D12Device* device) {
	errorMat = AssetMngr::Instance().LoadAsset<Material>(LR"(_internal\materials\error.mat)");

	skyboxShader = ShaderMngr::Instance().Get("StdPipeline/Skybox");

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
	};
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
	skyboxShader = nullptr;
	defaultSkyboxGpuHandle.ptr = 0;
	UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(defaultIBLSrvDHA));
	commonCBs.clear();
}

std::shared_ptr<Material> PipelineCommonResourceMngr::GetErrorMaterial() const { return errorMat; }

std::shared_ptr<Shader> PipelineCommonResourceMngr::GetSkyboxShader() const { return skyboxShader; }

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

ShaderCBDesc Ubpa::Utopia::UpdateShaderCBs(
	UDX12::DynamicUploadVector& uploadbuffer,
	const Shader& shader,
	const std::unordered_set<const Material*>& materials)
{
	ShaderCBDesc rst;
	rst.begin_offset = uploadbuffer.Size();

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

			auto target = rst.offsetMap.find(rsrcDesc.BindPoint);
			if (target != rst.offsetMap.end())
				continue;

			rst.offsetMap.emplace(std::pair{ rsrcDesc.BindPoint, rst.materialCBSize });
			rst.materialCBSize += UDX12::Util::CalcConstantBufferByteSize(cbDesc.Size);
		}
	};

	for (size_t i = 0; i < shader.passes.size(); i++) {
		CalculateSize(GPURsrcMngrDX12::Instance().GetShaderRefl_vs(shader, i));
		CalculateSize(GPURsrcMngrDX12::Instance().GetShaderRefl_ps(shader, i));
	}

	for (auto material : materials) {
		size_t idx = rst.indexMap.size();
		rst.indexMap[material->GetInstanceID()] = idx;
	}

	uploadbuffer.Resize(uploadbuffer.Size() + rst.materialCBSize * materials.size());

	auto UpdateShaderCBsForRefl = [&](std::set<size_t>& flags, const Material& material, ID3D12ShaderReflection* refl) {
		size_t index = rst.indexMap.at(material.GetInstanceID());

		D3D12_SHADER_DESC shaderDesc;
		ThrowIfFailed(refl->GetDesc(&shaderDesc));

		for (UINT i = 0; i < shaderDesc.ConstantBuffers; i++) {
			auto cb = refl->GetConstantBufferByIndex(i);
			D3D12_SHADER_BUFFER_DESC cbDesc;
			ThrowIfFailed(cb->GetDesc(&cbDesc));

			D3D12_SHADER_INPUT_BIND_DESC rsrcDesc;
			refl->GetResourceBindingDescByName(cbDesc.Name, &rsrcDesc);

			if (rst.offsetMap.find(rsrcDesc.BindPoint) == rst.offsetMap.end())
				continue;

			if (flags.find(rsrcDesc.BindPoint) != flags.end())
				continue;

			flags.insert(rsrcDesc.BindPoint);

			size_t offset = rst.begin_offset + rst.materialCBSize * index + rst.offsetMap.at(rsrcDesc.BindPoint);

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
						uploadbuffer.Set(offset + varDesc.StartOffset, &v, sizeof(unsigned int));
					}
					else if constexpr (std::is_same_v<Value, SharedVar<Texture2D>> || std::is_same_v<Value, SharedVar<TextureCube>>)
						assert(false);
					else {
						assert(varDesc.Size == sizeof(Value));
						uploadbuffer.Set(offset + varDesc.StartOffset, &value, varDesc.Size);
					}
					}, target->second.value);
			}
		}
	};

	for (auto material : materials) {
		std::set<size_t> flags;
		for (size_t i = 0; i < shader.passes.size(); i++) {
			UpdateShaderCBsForRefl(flags, *material, GPURsrcMngrDX12::Instance().GetShaderRefl_vs(shader, i));
			UpdateShaderCBsForRefl(flags, *material, GPURsrcMngrDX12::Instance().GetShaderRefl_ps(shader, i));
		}
	}

	return rst;
}

void Ubpa::Utopia::SetGraphicsRoot_CBV_SRV(
	ID3D12GraphicsCommandList* cmdList,
	UDX12::DynamicUploadVector& uploadbuffer,
	const ShaderCBDesc& shaderCBDesc,
	const Material& material,
	const std::map<std::string_view, D3D12_GPU_VIRTUAL_ADDRESS>& commonCBs,
	const std::map<std::string_view, D3D12_GPU_DESCRIPTOR_HANDLE>& commonSRVs
) {
	size_t cbPos = uploadbuffer.GetResource()->GetGPUVirtualAddress()
		+ shaderCBDesc.begin_offset + shaderCBDesc.indexMap.at(material.GetInstanceID()) * shaderCBDesc.materialCBSize;

	auto SetGraphicsRoot_Refl = [&](ID3D12ShaderReflection* refl) {
		D3D12_SHADER_DESC shaderDesc;
		ThrowIfFailed(refl->GetDesc(&shaderDesc));

		auto GetSRVRootParamIndex = [&](UINT registerIndex) {
			for (size_t i = 0; i < material.shader->rootParameters.size(); i++) {
				const auto& param = material.shader->rootParameters[i];

				bool flag = std::visit(
					[=](const auto& param) {
						using Type = std::decay_t<decltype(param)>;
						if constexpr (std::is_same_v<Type, RootDescriptorTable>) {
							const RootDescriptorTable& table = param;
							if (table.size() != 1)
								return false;

							const auto& range = table.front();
							assert(range.NumDescriptors > 0);

							return range.BaseShaderRegister == registerIndex;
						}
						else
							return false;
					},
					param
						);

				if (flag)
					return (UINT)i;
			}
			// assert(false);
			return static_cast<UINT>(-1); // inner SRV
		};

		auto GetCBVRootParamIndex = [&](UINT registerIndex) {
			for (size_t i = 0; i < material.shader->rootParameters.size(); i++) {
				const auto& param = material.shader->rootParameters[i];

				bool flag = std::visit(
					[=](const auto& param) {
						using Type = std::decay_t<decltype(param)>;
						if constexpr (std::is_same_v<Type, RootDescriptor>) {
							const RootDescriptor& descriptor = param;
							if (descriptor.DescriptorType != RootDescriptorType::CBV)
								return false;

							return descriptor.ShaderRegister == registerIndex;
						}
						else
							return false;
					},
					param
						);

				if (flag)
					return (UINT)i;
			}
			assert(false);
			return static_cast<UINT>(-1);
		};

		for (UINT i = 0; i < shaderDesc.BoundResources; i++) {
			D3D12_SHADER_INPUT_BIND_DESC rsrcDesc;
			ThrowIfFailed(refl->GetResourceBindingDesc(i, &rsrcDesc));

			switch (rsrcDesc.Type)
			{
			case D3D_SIT_CBUFFER:
			{
				UINT idx = GetCBVRootParamIndex(rsrcDesc.BindPoint);
				D3D12_GPU_VIRTUAL_ADDRESS adress;

				if (auto target = shaderCBDesc.offsetMap.find(rsrcDesc.BindPoint); target != shaderCBDesc.offsetMap.end())
					adress = cbPos + target->second;
				else if (auto target = commonCBs.find(rsrcDesc.Name); target != commonCBs.end())
					adress = target->second;
				else {
					assert(false);
					break;
				}
				cmdList->SetGraphicsRootConstantBufferView(idx, adress);
				break;
			}
			case D3D_SIT_TEXTURE:
			{
				UINT rootParamIndex = GetSRVRootParamIndex(rsrcDesc.BindPoint);
				if (rootParamIndex == static_cast<size_t>(-1))
					break;

				D3D12_GPU_DESCRIPTOR_HANDLE handle;
				handle.ptr = 0;

				if (auto target = material.properties.find(rsrcDesc.Name); target != material.properties.end()) {
					auto dim = rsrcDesc.Dimension;
					switch (dim)
					{
					case D3D_SRV_DIMENSION_TEXTURE2D: {
						assert(std::holds_alternative<SharedVar<Texture2D>>(target->second.value));
						auto tex2d = std::get<SharedVar<Texture2D>>(target->second.value);
						handle = GPURsrcMngrDX12::Instance().GetTexture2DSrvGpuHandle(*tex2d);
						break;
					}
					case D3D_SRV_DIMENSION_TEXTURECUBE: {
						assert(std::holds_alternative<SharedVar<TextureCube>>(target->second.value));
						auto texcube = std::get<SharedVar<TextureCube>>(target->second.value);
						handle = GPURsrcMngrDX12::Instance().GetTextureCubeSrvGpuHandle(*texcube);
						break;
					}
					default:
						assert("not support" && false);
						break;
					}
				}
				else if (auto target = commonSRVs.find(rsrcDesc.Name); target != commonSRVs.end())
					handle = target->second;
				else
					break;

				if (handle.ptr != 0)
					cmdList->SetGraphicsRootDescriptorTable(rootParamIndex, handle);

				break;
			}
			default:
				break;
			}
		}
	};

	for (size_t i = 0; i < material.shader->passes.size(); i++) {
		SetGraphicsRoot_Refl(GPURsrcMngrDX12::Instance().GetShaderRefl_vs(*material.shader, i));
		SetGraphicsRoot_Refl(GPURsrcMngrDX12::Instance().GetShaderRefl_ps(*material.shader, i));
	}
}

RenderContext Ubpa::Utopia::GenerateRenderContext(
	std::span<const UECS::World* const> worlds,
	std::span<const CameraConstants* const> cameraConstantsSpan,
	UDX12::DynamicUploadVector& shaderCB,
	UDX12::DynamicUploadVector& commonShaderCB)
{
	auto errorMat = PipelineCommonResourceMngr::GetInstance().GetErrorMaterial();

	auto skyboxShader = PipelineCommonResourceMngr::GetInstance().GetSkyboxShader();

	auto defaultSkyboxGpuHandle = PipelineCommonResourceMngr::GetInstance().GetDefaultSkyboxGpuHandle();

	RenderContext ctx;

	{ // object
		ArchetypeFilter filter;
		filter.all = {
			AccessTypeID_of<Latest<MeshFilter>>,
			AccessTypeID_of<Latest<MeshRenderer>>,
			AccessTypeID_of<Latest<LocalToWorld>>,
		};
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
					}
				},
				filter,
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

		UECS::ArchetypeFilter filter;
		filter.all = { UECS::AccessTypeID_of<UECS::Latest<LocalToWorld>> };
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
					filter
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

	// use first skybox in the world vector
	ctx.skyboxGpuHandle = defaultSkyboxGpuHandle;
	for (auto world : worlds) {
		if (auto ptr = world->entityMngr.ReadSingleton<Skybox>(); ptr && ptr->material && ptr->material->shader.get() == skyboxShader.get()) {
			auto target = ptr->material->properties.find("gSkybox");
			if (target != ptr->material->properties.end()
				&& std::holds_alternative<SharedVar<TextureCube>>(target->second.value)
				) {
				auto texcube = std::get<SharedVar<TextureCube>>(target->second.value);
				ctx.skyboxGpuHandle = GPURsrcMngrDX12::Instance().GetTextureCubeSrvGpuHandle(*texcube);
				break;
			}
		}
	}

	{ // camera, lights, objects
		ctx.cameraOffset = commonShaderCB.Size();
		ctx.lightOffset = ctx.cameraOffset
			+ cameraConstantsSpan.size() * UDX12::Util::CalcConstantBufferByteSize(sizeof(CameraConstants));
		ctx.objectOffset = ctx.lightOffset
			+ UDX12::Util::CalcConstantBufferByteSize(sizeof(LightArray));

		commonShaderCB.Resize(
			commonShaderCB.Size()
			+ cameraConstantsSpan.size() * UDX12::Util::CalcConstantBufferByteSize(sizeof(CameraConstants))
			+ UDX12::Util::CalcConstantBufferByteSize(sizeof(LightArray))
			+ ctx.entity2data.size() * UDX12::Util::CalcConstantBufferByteSize(sizeof(ObjectConstants))
		);

		for (size_t i = 0; i < cameraConstantsSpan.size(); i++) {
			commonShaderCB.Set(
				ctx.cameraOffset + i * UDX12::Util::CalcConstantBufferByteSize(sizeof(CameraConstants)),
				cameraConstantsSpan[i],
				sizeof(CameraConstants)
			);
		}

		// light array
		commonShaderCB.Set(ctx.lightOffset, &ctx.lightArray, sizeof(LightArray));

		// objects
		size_t offset = ctx.objectOffset;
		for (auto& [idx, data] : ctx.entity2data) {
			ObjectConstants objectConstants;
			objectConstants.World = data.l2w;
			objectConstants.InvWorld = data.w2l;
			objectConstants.PrevWorld = data.prevl2w;
			ctx.entity2offset[idx] = offset;
			commonShaderCB.Set(offset, &objectConstants, sizeof(ObjectConstants));
			offset += UDX12::Util::CalcConstantBufferByteSize(sizeof(ObjectConstants));
		}
	}

	std::unordered_map<const Shader*, std::unordered_set<const Material*>> opaqueMaterialMap;
	for (const auto& opaque : ctx.renderQueue.GetOpaques())
		opaqueMaterialMap[opaque.material->shader.get()].insert(opaque.material.get());

	std::unordered_map<const Shader*, std::unordered_set<const Material*>> transparentMaterialMap;
	for (const auto& transparent : ctx.renderQueue.GetTransparents())
		transparentMaterialMap[transparent.material->shader.get()].insert(transparent.material.get());

	for (const auto& [shader, materials] : opaqueMaterialMap) {
		ctx.shaderCBDescMap[shader] =
			UpdateShaderCBs(shaderCB, *shader, materials);
	}

	for (const auto& [shader, materials] : transparentMaterialMap) {
		ctx.shaderCBDescMap[shader] =
			UpdateShaderCBs(shaderCB, *shader, materials);
	}

	return ctx;
}