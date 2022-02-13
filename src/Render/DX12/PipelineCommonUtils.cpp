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

RenderContext Ubpa::Utopia::GenerateRenderContext(size_t ID, std::span<const UECS::World* const> worlds)
{
	auto errorMat = PipelineCommonResourceMngr::GetInstance().GetErrorMaterial();

	auto skyboxShader = PipelineCommonResourceMngr::GetInstance().GetSkyboxShader();

	auto defaultSkyboxGpuHandle = PipelineCommonResourceMngr::GetInstance().GetDefaultSkyboxGpuHandle();

	RenderContext ctx;

	ctx.ID = ID;

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

	return ctx;
}