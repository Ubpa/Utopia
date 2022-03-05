#include <Utopia/Render/DX12/StdPipeline.h>

#include <Utopia/Render/DX12/PipelineCommonUtils.h>
#include <Utopia/Render/DX12/ShaderCBMngr.h>

#include <Utopia/Render/DX12/CameraRsrcMngr.h>
#include <Utopia/Render/DX12/WorldRsrcMngr.h>

#include <Utopia/Render/DX12/GPURsrcMngrDX12.h>
#include <Utopia/Render/Texture2D.h>

#include <Utopia/Render/Components/Camera.h>

#include <UDX12/FrameResourceMngr.h>

#include <UECS/UECS.hpp>

#include "IBLPreprocess.h"
#include "GeometryBuffer.h"
#include "DeferredLighting.h"
#include "ForwardLighting.h"
#include "Sky.h"
#include "Tonemapping.h"
#include "TAA.h"

using namespace Ubpa::Utopia;
using namespace Ubpa;

struct StdPipeline::Impl {
	struct Stages {
		GeometryBuffer geometryBuffer;
		DeferredLighting deferredLighting;
		ForwardLighting forwardLighting;
		Sky sky;
		Tonemapping tonemapping;
		TAA taa;
	};

	struct FrameRsrcs {
		FrameRsrcs(ID3D12Device* device) : shaderCBMngr(device) {}

		ShaderCBMngr shaderCBMngr;
		CameraRsrcMngr cameraRsrcMngr;
		WorldRsrcMngr worldRsrcMngr;
		Stages stages;
	};

	Impl(InitDesc initDesc) :
		initDesc{ initDesc },
		frameRsrcMngr{ initDesc.numFrame, initDesc.device }
	{
		for (const auto& fr : frameRsrcMngr.GetFrameResources())
			fr->RegisterResource("FrameRsrcs", std::make_shared<FrameRsrcs>(initDesc.device));
	}

	~Impl();

	CameraRsrcMngr crossFrameCameraRsrcMngr;

	const InitDesc initDesc;
	
	UDX12::FrameResourceMngr frameRsrcMngr;

	UFG::Compiler fgCompiler;

	WorldRsrcMngr crossFrameWorldRsrcMngr;

	const DXGI_FORMAT dsFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	static constexpr const char key_CameraResizeData[] = "CameraResizeData";
	static constexpr const char key_CameraResizeData_Backup[] = "CameraResizeData_Backup";
	struct CameraResizeData {
		size_t width{ 0 };
		size_t height{ 0 };

		Microsoft::WRL::ComPtr<ID3D12Resource> taaPrevRsrc;
	};

	std::map<std::string, UFG::FrameGraph> frameGraphMap;

	void Render(
		std::span<const UECS::World* const> worlds,
		std::span<const CameraData> cameras,
		std::span<const WorldCameraLink> links,
		std::span<ID3D12Resource* const> defaultRTs);

	void CameraRender(
		const RenderContext& ctx,
		const CameraData& cameraData,
		D3D12_GPU_VIRTUAL_ADDRESS cameraCBAddress,
		ID3D12Resource* default_rtb,
		const ResourceMap& worldRsrc,
		std::string_view fgName);

	void UpdateCameraResource(const CameraData& cameraData, size_t width, size_t height);
};

StdPipeline::Impl::~Impl() = default;

void StdPipeline::Impl::CameraRender(
	const RenderContext& ctx,
	const CameraData& cameraData,
	D3D12_GPU_VIRTUAL_ADDRESS cameraCBAddress,
	ID3D12Resource* default_rtb,
	const ResourceMap& worldRsrc,
	std::string_view fgName)
{
	ID3D12Resource* rtb = default_rtb;
	{ // set rtb
		if (cameraData.entity.Valid()) {
			auto camera_rtb = cameraData.world->entityMngr.ReadComponent<Camera>(cameraData.entity)->renderTarget;
			if (camera_rtb) {
				GPURsrcMngrDX12::Instance().RegisterRenderTargetTexture2D(*camera_rtb);
				rtb = GPURsrcMngrDX12::Instance().GetRenderTargetTexture2DResource(*camera_rtb);
			}
		}
		assert(rtb);
	}
	size_t width = rtb->GetDesc().Width;
	size_t height = rtb->GetDesc().Height;
	CD3DX12_VIEWPORT viewport(0.f, 0.f, width, height);
	D3D12_RECT scissorRect(0.f, 0.f, static_cast<LONG>(width), static_cast<LONG>(height));

	UpdateCameraResource(cameraData, width, height);

	auto [frameGraphMapIter, frameGraphEmplaceSuccess] = frameGraphMap.emplace(std::string(fgName), UFG::FrameGraph{ std::string(fgName) });
	assert(frameGraphEmplaceSuccess);
	UFG::FrameGraph& fg = frameGraphMapIter->second;

	auto frameRsrcs = frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<std::shared_ptr<FrameRsrcs>>("FrameRsrcs");
	auto& cameraRsrcMngr = frameRsrcs->cameraRsrcMngr;
	auto& shaderCBMngr = frameRsrcs->shaderCBMngr;

	auto& stages = frameRsrcs->stages;

	if (!cameraRsrcMngr.Get(cameraData).Contains("FrameGraphRsrcMngr"))
		cameraRsrcMngr.Get(cameraData).Register("FrameGraphRsrcMngr", std::make_shared<UDX12::FG::RsrcMngr>(initDesc.device));
	auto fgRsrcMngr = cameraRsrcMngr.Get(cameraData).Get<std::shared_ptr<UDX12::FG::RsrcMngr>>("FrameGraphRsrcMngr");

	if (!cameraRsrcMngr.Get(cameraData).Contains("FrameGraphExecutor"))
		cameraRsrcMngr.Get(cameraData).Register("FrameGraphExecutor", std::make_shared<UDX12::FG::Executor>(initDesc.device));
	auto fgExecutor = cameraRsrcMngr.Get(cameraData).Get<std::shared_ptr<UDX12::FG::Executor>>("FrameGraphExecutor");

	fgRsrcMngr->NewFrame();
	fgExecutor->NewFrame();
	stages.geometryBuffer.NewFrame();
	stages.deferredLighting.NewFrame();
	stages.forwardLighting.NewFrame();
	stages.sky.NewFrame();
	stages.taa.NewFrame();
	stages.tonemapping.NewFrame();

	const auto& cameraResizeData = cameraRsrcMngr.Get(cameraData).Get<CameraResizeData>(key_CameraResizeData);
	auto taaPrevRsrc = cameraResizeData.taaPrevRsrc;

	stages.geometryBuffer.RegisterInputOutputPassNodes(fg, {});

	auto gbuffer0 = stages.geometryBuffer.GetOutputNodeIDs()[0];
	auto gbuffer1 = stages.geometryBuffer.GetOutputNodeIDs()[1];
	auto gbuffer2 = stages.geometryBuffer.GetOutputNodeIDs()[2];
	auto motion = stages.geometryBuffer.GetOutputNodeIDs()[3];
	auto deferDS = stages.geometryBuffer.GetOutputNodeIDs()[4];
	auto irradianceMap = fg.RegisterResourceNode("Irradiance Map");
	auto prefilterMap = fg.RegisterResourceNode("PreFilter Map");
	const size_t deferredLightingInputs[6] = { gbuffer0, gbuffer1, gbuffer2, deferDS, irradianceMap, prefilterMap };
	stages.deferredLighting.RegisterInputOutputPassNodes(fg, deferredLightingInputs);
	auto deferLightedRT = stages.deferredLighting.GetOutputNodeIDs()[0];
	const size_t forwardLightingInputs[2] = { irradianceMap, prefilterMap };
	stages.forwardLighting.RegisterInputOutputPassNodes(fg, forwardLightingInputs);
	auto sceneRT = stages.forwardLighting.GetOutputNodeIDs()[0];
	auto forwardDS = stages.forwardLighting.GetOutputNodeIDs()[1];
	fg.RegisterMoveNode(forwardDS, deferDS);
	stages.sky.RegisterInputOutputPassNodes(fg, { &deferDS, 1 });
	auto deferLightedSkyRT = stages.sky.GetOutputNodeIDs()[0];
	auto taaPrevResult = fg.RegisterResourceNode("TAA Prev Result");
	const size_t taaInputs[3] = { taaPrevResult, sceneRT, motion };
	stages.taa.RegisterInputOutputPassNodes(fg, taaInputs);
	auto taaResult = stages.taa.GetOutputNodeIDs().front();
	stages.tonemapping.RegisterInputOutputPassNodes(fg, { &taaResult, 1 });
	auto tonemappedRT = stages.tonemapping.GetOutputNodeIDs()[0];
	auto presentedRT = fg.RegisterResourceNode("Present");
	fg.RegisterMoveNode(deferLightedSkyRT, deferLightedRT);
	fg.RegisterMoveNode(sceneRT, deferLightedSkyRT);
	fg.RegisterMoveNode(presentedRT, tonemappedRT);

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

	D3D12_CLEAR_VALUE gbuffer_clearvalue = {};
	gbuffer_clearvalue.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	gbuffer_clearvalue.Color[0] = 0;
	gbuffer_clearvalue.Color[1] = 0;
	gbuffer_clearvalue.Color[2] = 0;
	gbuffer_clearvalue.Color[3] = 0;

	auto rt2dDesc = UDX12::Desc::RSRC::RT2D(width, (UINT)height, DXGI_FORMAT_R32G32B32A32_FLOAT);

	auto iblData = worldRsrc.Get<std::shared_ptr<IBLData>>("IBL data");

	(*fgRsrcMngr)
		.RegisterTemporalRsrc(gbuffer0, rt2dDesc, gbuffer_clearvalue)
		.RegisterTemporalRsrc(gbuffer1, rt2dDesc, gbuffer_clearvalue)
		.RegisterTemporalRsrc(gbuffer2, rt2dDesc, gbuffer_clearvalue)
		.RegisterTemporalRsrc(motion, rt2dDesc, gbuffer_clearvalue)
		.RegisterTemporalRsrc(deferDS, dsDesc, dsClear)
		.RegisterTemporalRsrcAutoClear(deferLightedRT, rt2dDesc)
		.RegisterTemporalRsrcAutoClear(taaResult, rt2dDesc)

		.RegisterImportedRsrc(tonemappedRT, {rtb, D3D12_RESOURCE_STATE_PRESENT})
		.RegisterImportedRsrc(irradianceMap, { iblData->irradianceMapResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })
		.RegisterImportedRsrc(prefilterMap, { iblData->prefilterMapResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })
		.RegisterImportedRsrc(taaPrevResult, { taaPrevRsrc.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })
		;

	stages.geometryBuffer.RegisterPassResources(*fgRsrcMngr);
	stages.deferredLighting.RegisterPassResources(*fgRsrcMngr);
	stages.forwardLighting.RegisterPassResources(*fgRsrcMngr);
	stages.sky.RegisterPassResources(*fgRsrcMngr);
	stages.taa.RegisterPassResources(*fgRsrcMngr);
	stages.tonemapping.RegisterPassResources(*fgRsrcMngr);

	const D3D12_GPU_VIRTUAL_ADDRESS lightCBAddress = shaderCBMngr.GetLightCBAddress(ctx.ID);
	// irradiance, prefilter, BRDF LUT
	const D3D12_GPU_DESCRIPTOR_HANDLE iblDataSrvGpuHandle = ctx.skyboxSrvGpuHandle.ptr == PipelineCommonResourceMngr::GetInstance().GetDefaultSkyboxGpuHandle().ptr ?
		PipelineCommonResourceMngr::GetInstance().GetDefaultIBLSrvDHA().GetGpuHandle()
		: iblData->SRVDH.GetGpuHandle();

	stages.geometryBuffer.RegisterPassFuncData(cameraCBAddress, &shaderCBMngr, &ctx, "Deferred");
	stages.geometryBuffer.RegisterPassFunc(*fgExecutor);

	stages.deferredLighting.RegisterPassFuncData(iblDataSrvGpuHandle, cameraCBAddress, lightCBAddress);
	stages.deferredLighting.RegisterPassFunc(*fgExecutor);

	stages.forwardLighting.RegisterPassFuncData(iblData->SRVDH.GetGpuHandle(), cameraCBAddress, &shaderCBMngr, &ctx);
	stages.forwardLighting.RegisterPassFunc(*fgExecutor);

	stages.sky.RegisterPassFuncData(ctx.skyboxSrvGpuHandle, cameraCBAddress);
	stages.sky.RegisterPassFunc(*fgExecutor);

	stages.taa.RegisterPassFuncData(cameraCBAddress);
	stages.taa.RegisterPassFunc(*fgExecutor);

	stages.tonemapping.RegisterPassFunc(*fgExecutor);

	static bool flag{ false };
	if (!flag) {
		OutputDebugStringA(fg.ToGraphvizGraph2().Dump().c_str());
		flag = true;
	}

	auto crst = fgCompiler.Compile(fg);
	fgExecutor->Execute(
		initDesc.cmdQueue,
		crst,
		*fgRsrcMngr
	);
}

void StdPipeline::Impl::Render(
	std::span<const UECS::World* const> worlds,
	std::span<const CameraData> cameras,
	std::span<const WorldCameraLink> links,
	std::span<ID3D12Resource* const> defaultRTs
) {
	// Cycle through the circular frame resource array.
	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	frameRsrcMngr.BeginFrame();

	frameGraphMap.clear();

	UpdateCrossFrameCameraResources(crossFrameCameraRsrcMngr, cameras, defaultRTs);

	auto frameRsrcs = frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<std::shared_ptr<FrameRsrcs>>("FrameRsrcs");
	frameRsrcs->cameraRsrcMngr.Update(cameras);

	std::vector<std::vector<const UECS::World*>> worldArrs;
	for (const auto& link : links) {
		std::vector<const UECS::World*> worldArr;
		for (auto idx : link.worldIndices)
			worldArr.push_back(worlds[idx]);
		worldArrs.push_back(std::move(worldArr));
	}

	auto& shaderCBMngr = frameRsrcs->shaderCBMngr;
	shaderCBMngr.NewFrame();

	std::vector<RenderContext> ctxs;
	for (size_t i = 0; i < worldArrs.size(); ++i) {
		auto ctx = GenerateRenderContext(i, worldArrs[i]);

		std::vector<CameraData> linkedCameras;
		for (size_t camIdx : links[i].cameraIndices)
			linkedCameras.push_back(cameras[camIdx]);
		std::vector<const CameraConstants*> cameraConstantsVector;
		for (const auto& linkedCamera : linkedCameras) {
			auto* camConts = &crossFrameCameraRsrcMngr.Get(linkedCamera).Get<CameraConstants>(key_CameraConstants);
			cameraConstantsVector.push_back(camConts);
		}
		shaderCBMngr.RegisterRenderContext(ctx, cameraConstantsVector);

		ctxs.push_back(std::move(ctx));
	}

	crossFrameWorldRsrcMngr.Update(worldArrs, initDesc.numFrame);
	auto& curFrameWorldRsrcMngr = frameRsrcs->worldRsrcMngr;
	curFrameWorldRsrcMngr.Update(worldArrs);

	for (size_t linkIdx = 0; linkIdx < links.size(); ++linkIdx) {
		auto& ctx = ctxs[linkIdx];
		auto& crossFrameWorldRsrc = crossFrameWorldRsrcMngr.Get(worldArrs[linkIdx]);
		auto& curFrameWorldRsrc = curFrameWorldRsrcMngr.Get(worldArrs[linkIdx]);

		if (!curFrameWorldRsrc.Contains("FrameGraphRsrcMngr"))
			curFrameWorldRsrc.Register("FrameGraphRsrcMngr", std::make_shared<UDX12::FG::RsrcMngr>(initDesc.device));
		auto fgRsrcMngr = curFrameWorldRsrc.Get<std::shared_ptr<UDX12::FG::RsrcMngr>>("FrameGraphRsrcMngr");
		fgRsrcMngr->NewFrame();

		if (!curFrameWorldRsrc.Contains("FrameGraphExecutor"))
			curFrameWorldRsrc.Register("FrameGraphExecutor", std::make_shared<UDX12::FG::Executor>(initDesc.device));
		auto fgExecutor = curFrameWorldRsrc.Get<std::shared_ptr<UDX12::FG::Executor>>("FrameGraphExecutor");
		fgExecutor->NewFrame();

		if (!crossFrameWorldRsrc.Contains("IBL data")) {
			auto iblData = std::make_shared<IBLData>();
			iblData->Init(initDesc.device);
			crossFrameWorldRsrc.Register("IBL data", iblData);
		}

		if (!crossFrameWorldRsrc.Contains("IBLPreprocess"))
			crossFrameWorldRsrc.Register("IBLPreprocess", std::make_shared<IBLPreprocess>());
		auto iblPreprocess = crossFrameWorldRsrc.Get<std::shared_ptr<IBLPreprocess>>("IBLPreprocess");
		iblPreprocess->NewFrame();

		if (!curFrameWorldRsrc.Contains("IBL data"))
			curFrameWorldRsrc.Register("IBL data", crossFrameWorldRsrc.Get<std::shared_ptr<IBLData>>("IBL data"));
		auto iblData = curFrameWorldRsrc.Get<std::shared_ptr<IBLData>>("IBL data");

		const std::string fgName = "L" + std::to_string(linkIdx);
		auto [frameGraphMapIter, frameGraphEmplaceSuccess] = frameGraphMap.emplace(fgName, UFG::FrameGraph{ fgName });
		assert(frameGraphEmplaceSuccess);
		UFG::FrameGraph& fg = frameGraphMapIter->second;

		iblPreprocess->RegisterInputOutputPassNodes(fg, {});
		auto irradianceMap = iblPreprocess->GetOutputNodeIDs()[0];
		auto prefilterMap = iblPreprocess->GetOutputNodeIDs()[1];

		(*fgRsrcMngr)
			.RegisterImportedRsrc(irradianceMap, { iblData->irradianceMapResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })
			.RegisterImportedRsrc(prefilterMap, { iblData->prefilterMapResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })
			;

		iblPreprocess->RegisterPassResources(*fgRsrcMngr);

		iblPreprocess->RegisterPassFuncData(iblData.get(), ctx.skyboxSrvGpuHandle);

		iblPreprocess->RegisterPassFunc(*fgExecutor);

		static bool flag{ false };
		if (!flag) {
			OutputDebugStringA(fg.ToGraphvizGraph2().Dump().c_str());
			flag = true;
		}

		auto crst = fgCompiler.Compile(fg);
		fgExecutor->Execute(
			initDesc.cmdQueue,
			crst,
			*fgRsrcMngr
		);

		for (size_t i = 0; i < links[linkIdx].cameraIndices.size(); ++i) {
			size_t cameraIdx = links[linkIdx].cameraIndices[i];
			CameraRender(
				ctx,
				cameras[cameraIdx],
				shaderCBMngr.GetCameraCBAddress(ctx.ID, i),
				defaultRTs[cameraIdx],
				curFrameWorldRsrc,
				fgName + "C" + std::to_string(i));
		}
	}

	frameRsrcMngr.EndFrame(initDesc.cmdQueue);
}

void StdPipeline::Init(InitDesc desc) {
	if(!pImpl)
		pImpl = new Impl{ desc };
}

StdPipeline::~StdPipeline() { delete pImpl; }

void StdPipeline::Render(
	std::span<const UECS::World* const> worlds,
	std::span<const CameraData> cameras,
	std::span<const WorldCameraLink> links,
	std::span<ID3D12Resource*const> defaultRTs)
{
	assert(pImpl);
	pImpl->Render(worlds, cameras, links, defaultRTs);
}

const std::map<std::string, UFG::FrameGraph>& StdPipeline::GetFrameGraphMap() const {
	assert(pImpl);
	return pImpl->frameGraphMap;
}

void StdPipeline::Impl::UpdateCameraResource(const CameraData& cameraData, size_t width, size_t height) {
	auto& cameraRsrcMngr = frameRsrcMngr.GetCurrentFrameResource()->GetResource<std::shared_ptr<FrameRsrcs>>("FrameRsrcs")->cameraRsrcMngr;
	if (cameraRsrcMngr.Get(cameraData).Contains(key_CameraResizeData_Backup))
		cameraRsrcMngr.Get(cameraData).Unregister(key_CameraResizeData_Backup);
	if (!cameraRsrcMngr.Get(cameraData).Contains(key_CameraResizeData))
		cameraRsrcMngr.Get(cameraData).Register(key_CameraResizeData, CameraResizeData{});
	const auto& currCameraResizeData = cameraRsrcMngr.Get(cameraData).Get<CameraResizeData>(key_CameraResizeData);
	if (currCameraResizeData.width == width && currCameraResizeData.height == height)
		return;
	CameraResizeData cameraResizeData;
	cameraResizeData.width = width;
	cameraResizeData.height = height;

	auto rt2dDesc = UDX12::Desc::RSRC::RT2D(width, (UINT)height, DXGI_FORMAT_R32G32B32A32_FLOAT);

	D3D12_CLEAR_VALUE clearvalue = {};
	clearvalue.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	clearvalue.Color[0] = 0;
	clearvalue.Color[1] = 0;
	clearvalue.Color[2] = 0;
	clearvalue.Color[3] = 1;
	const auto defaultHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	Microsoft::WRL::ComPtr<ID3D12Resource> taaPrevRsrc;
	initDesc.device->CreateCommittedResource(
		&defaultHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&rt2dDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearvalue,
		IID_PPV_ARGS(&taaPrevRsrc)
	);

	cameraResizeData.taaPrevRsrc = std::move(taaPrevRsrc);

	cameraRsrcMngr.Get(cameraData).Unregister(key_CameraResizeData);
	cameraRsrcMngr.Get(cameraData).Register(key_CameraResizeData, cameraResizeData);

	for (auto& frsrc : frameRsrcMngr.GetFrameResources()) {
		if (frsrc.get() == frameRsrcMngr.GetCurrentFrameResource())
			continue;

		auto& cameraRsrcMngr = frsrc->GetResource<std::shared_ptr<FrameRsrcs>>("FrameRsrcs")->cameraRsrcMngr;;
		if (!cameraRsrcMngr.Contains(cameraData))
			continue;

		if (!cameraRsrcMngr.Get(cameraData).Contains(key_CameraResizeData))
			cameraRsrcMngr.Get(cameraData).Register(key_CameraResizeData, CameraResizeData{});

		if (!cameraRsrcMngr.Get(cameraData).Contains(key_CameraResizeData_Backup)) {
			cameraRsrcMngr.Get(cameraData).Register(
				key_CameraResizeData_Backup,
				std::move(cameraRsrcMngr.Get(cameraData).Get<CameraResizeData>(key_CameraResizeData))
			);
		}

		cameraRsrcMngr.Get(cameraData).Get<CameraResizeData>(key_CameraResizeData) = cameraResizeData;
	}
}
