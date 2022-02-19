#include <Utopia/Render/DX12/StdPipeline.h>

#include <Utopia/Render/DX12/PipelineCommonUtils.h>
#include <Utopia/Render/DX12/ShaderCBMngr.h>

#include <Utopia/Render/DX12/CameraRsrcMngr.h>
#include <Utopia/Render/DX12/WorldRsrcMngr.h>

#include <Utopia/Render/DX12/GPURsrcMngrDX12.h>
#include <Utopia/Render/DX12/MeshLayoutMngr.h>
#include <Utopia/Render/ShaderMngr.h>
#include <Utopia/Render/Texture2D.h>
#include <Utopia/Render/TextureCube.h>
#include <Utopia/Render/HLSLFile.h>
#include <Utopia/Render/Shader.h>
#include <Utopia/Render/Mesh.h>
#include <Utopia/Render/RenderQueue.h>

#include <Utopia/Core/AssetMngr.h>

#include <Utopia/Core/Image.h>
#include <Utopia/Render/Components/Camera.h>
#include <Utopia/Render/Components/MeshFilter.h>
#include <Utopia/Render/Components/MeshRenderer.h>
#include <Utopia/Render/Components/Skybox.h>
#include <Utopia/Render/Components/Light.h>
#include <Utopia/Core/GameTimer.h>

#include <Utopia/Core/Components/LocalToWorld.h>
#include <Utopia/Core/Components/Translation.h>
#include <Utopia/Core/Components/WorldToLocal.h>
#include <Utopia/Core/Components/PrevLocalToWorld.h>

#include <UECS/UECS.hpp>

#include <_deps/imgui/imgui.h>
#include <_deps/imgui/imgui_impl_win32.h>
#include <_deps/imgui/imgui_impl_dx12.h>

#include <UDX12/FrameResourceMngr.h>

#include "GeometryBuffer.h"
#include "DeferredLighting.h"
#include "ForwardLighting.h"
#include "Tonemapping.h"
#include "TAA.h"

using namespace Ubpa::Utopia;
using namespace Ubpa::UECS;
using namespace Ubpa;

struct StdPipeline::Impl {
	Impl(InitDesc initDesc) :
		initDesc{ initDesc },
		frameRsrcMngr{ initDesc.numFrame, initDesc.device },
		fg { "Standard Pipeline" },
		crossFrameShaderCB{ initDesc.device }
	{
		BuildTextures();
		BuildFrameResources();
		BuildShaders();
	}

	~Impl();

	struct QuadPositionLs {
		valf4 positionL4x;
		valf4 positionL4y;
		valf4 positionL4z;
	};

	struct MipInfo {
		float roughness;
		float resolution;
	};

	CameraRsrcMngr crossFrameCameraRsrcs;
	static constexpr const char key_CameraConstants[] = "CameraConstants";

	struct Stages {
		GeometryBuffer geometryBuffer;
		DeferredLighting deferredLighting;
		ForwardLighting forwardLighting;
		Tonemapping tonemapping;
		TAA taa;
	};

	const InitDesc initDesc;
	
	UDX12::FrameResourceMngr frameRsrcMngr;

	UFG::Compiler fgCompiler;
	UFG::FrameGraph fg;

	std::shared_ptr<Shader> irradianceShader;
	std::shared_ptr<Shader> prefilterShader;

	UDX12::DynamicUploadVector crossFrameShaderCB;
	WorldRsrcMngr crossFrameWorldRsrcMngr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	const DXGI_FORMAT dsFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	static constexpr const char key_CameraResizeData[] = "CameraResizeData";
	static constexpr const char key_CameraResizeData_Backup[] = "CameraResizeData_Backup";
	struct CameraResizeData {
		size_t width{ 0 };
		size_t height{ 0 };

		Microsoft::WRL::ComPtr<ID3D12Resource> taaPrevRsrc;
	};

	void BuildTextures();
	void BuildFrameResources();
	void BuildShaders();

	void UpdateCrossFrameCameraResources(std::span<const CameraData> cameras, std::span<ID3D12Resource* const> defaultRTs);
	void Render(
		std::span<const UECS::World* const> worlds,
		std::span<const CameraData> cameras,
		std::span<const WorldCameraLink> links,
		std::span<ID3D12Resource* const> defaultRTs);

	void CameraRender(const RenderContext& ctx, const CameraData& cameraData, size_t cameraIdx, ID3D12Resource* rtb, const ResourceMap& worldRsrc);
	void UpdateCameraResource(const CameraData& cameraData, size_t width, size_t height);
};

StdPipeline::Impl::~Impl() {
}

void StdPipeline::Impl::BuildTextures() {
}

void StdPipeline::Impl::BuildFrameResources() {
	for (const auto& fr : frameRsrcMngr.GetFrameResources()) {
		fr->RegisterResource("ShaderCBMngr", std::make_shared<ShaderCBMngr>(initDesc.device));

		fr->RegisterResource("CameraRsrcMngr", std::make_shared<CameraRsrcMngr>());
		fr->RegisterResource("WorldRsrcMngr", std::make_shared<WorldRsrcMngr>());

		fr->RegisterResource("Stages", std::make_shared<Stages>());
	}
}

void StdPipeline::Impl::BuildShaders() {
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

	constexpr auto quadPositionsSize = UDX12::Util::CalcConstantBufferByteSize(sizeof(QuadPositionLs));
	constexpr auto mipInfoSize = UDX12::Util::CalcConstantBufferByteSize(sizeof(MipInfo));
	crossFrameShaderCB.Resize(6 * quadPositionsSize + IBLData::PreFilterMapMipLevels * mipInfoSize);
	for (size_t i = 0; i < 6; i++) {
		QuadPositionLs positionLs;
		auto p0 = origin[i];
		auto p1 = origin[i] + right[i];
		auto p2 = origin[i] + right[i] + up[i];
		auto p3 = origin[i] + up[i];
		positionLs.positionL4x = { p0[0], p1[0], p2[0], p3[0] };
		positionLs.positionL4y = { p0[1], p1[1], p2[1], p3[1] };
		positionLs.positionL4z = { p0[2], p1[2], p2[2], p3[2] };
		crossFrameShaderCB.Set(i * quadPositionsSize, &positionLs, sizeof(QuadPositionLs));
	}
	size_t size = IBLData::PreFilterMapSize;
	for (UINT i = 0; i < IBLData::PreFilterMapMipLevels; i++) {
		MipInfo info;
		info.roughness = i / float(IBLData::PreFilterMapMipLevels - 1);
		info.resolution = (float)size;
		
		crossFrameShaderCB.Set(6 * quadPositionsSize + i * mipInfoSize, &info, sizeof(MipInfo));
		size /= 2;
	}
}

void StdPipeline::Impl::UpdateCrossFrameCameraResources(std::span<const CameraData> cameras, std::span<ID3D12Resource* const> defaultRTs) {
	crossFrameCameraRsrcs.Update(cameras);

	for (size_t i = 0; i < cameras.size(); ++i) {
		const auto& camera = cameras[i];
		auto desc = defaultRTs[i]->GetDesc();
		size_t defalut_width = desc.Width;
		size_t defalut_height = desc.Height;

		auto& cameraRsrcMap = crossFrameCameraRsrcs.Get(camera);
		if (!cameraRsrcMap.Contains(key_CameraConstants))
			cameraRsrcMap.Register(key_CameraConstants, CameraConstants{});
		auto& cameraConstants = cameraRsrcMap.Get<CameraConstants>(key_CameraConstants);

		auto cmptCamera = camera.world->entityMngr.ReadComponent<Camera>(camera.entity);
		auto cmptW2L = camera.world->entityMngr.ReadComponent<WorldToLocal>(camera.entity);
		auto cmptTranslation = camera.world->entityMngr.ReadComponent<Translation>(camera.entity);
		size_t rt_width = defalut_width, rt_height = defalut_height;
		if (cmptCamera->renderTarget) {
			rt_width = cmptCamera->renderTarget->image.GetWidth();
			rt_height = cmptCamera->renderTarget->image.GetHeight();
		}

		float SampleX = rand01<float>();
		float SampleY = rand01<float>();
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

void StdPipeline::Impl::CameraRender(const RenderContext& ctx, const CameraData& cameraData, size_t cameraIdx, ID3D12Resource* default_rtb, const ResourceMap& worldRsrc) {
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
	
	fg.Clear();

	auto cameraRsrcMngr = frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<std::shared_ptr<CameraRsrcMngr>>("CameraRsrcMngr");

	auto stages = frameRsrcMngr.GetCurrentFrameResource()->GetResource<std::shared_ptr<Stages>>("Stages");

	if (!cameraRsrcMngr->Get(cameraData).Contains("FrameGraphRsrcMngr"))
		cameraRsrcMngr->Get(cameraData).Register("FrameGraphRsrcMngr", std::make_shared<UDX12::FG::RsrcMngr>(initDesc.device));
	auto fgRsrcMngr = cameraRsrcMngr->Get(cameraData).Get<std::shared_ptr<UDX12::FG::RsrcMngr>>("FrameGraphRsrcMngr");

	if (!cameraRsrcMngr->Get(cameraData).Contains("FrameGraphExecutor"))
		cameraRsrcMngr->Get(cameraData).Register("FrameGraphExecutor", std::make_shared<UDX12::FG::Executor>(initDesc.device));
	auto fgExecutor = cameraRsrcMngr->Get(cameraData).Get<std::shared_ptr<UDX12::FG::Executor>>("FrameGraphExecutor");

	fgRsrcMngr->NewFrame();
	fgExecutor->NewFrame();
	stages->geometryBuffer.NewFrame();
	stages->deferredLighting.NewFrame();
	stages->forwardLighting.NewFrame();
	stages->taa.NewFrame();
	stages->tonemapping.NewFrame();

	const auto& cameraResizeData = cameraRsrcMngr->Get(cameraData).Get<CameraResizeData>(key_CameraResizeData);
	auto taaPrevRsrc = cameraResizeData.taaPrevRsrc;

	stages->geometryBuffer.RegisterInputOutputPassNodes(fg, {});

	auto gbuffer0 = stages->geometryBuffer.GetOutputNodeIDs()[0];
	auto gbuffer1 = stages->geometryBuffer.GetOutputNodeIDs()[1];
	auto gbuffer2 = stages->geometryBuffer.GetOutputNodeIDs()[2];
	auto motion = stages->geometryBuffer.GetOutputNodeIDs()[3];
	auto deferDS = stages->geometryBuffer.GetOutputNodeIDs()[4];
	auto irradianceMap = fg.RegisterResourceNode("Irradiance Map");
	auto prefilterMap = fg.RegisterResourceNode("PreFilter Map");
	const size_t deferredLightingInputs[6] = { gbuffer0, gbuffer1, gbuffer2, deferDS, irradianceMap, prefilterMap };
	stages->deferredLighting.RegisterInputOutputPassNodes(fg, deferredLightingInputs);
	auto deferLightedRT = stages->deferredLighting.GetOutputNodeIDs()[0];
	auto deferLightedSkyRT = fg.RegisterResourceNode("Defer Lighted with Sky");
	const size_t forwardLightingInputs[2] = { irradianceMap, prefilterMap };
	stages->forwardLighting.RegisterInputOutputPassNodes(fg, forwardLightingInputs);
	auto sceneRT = stages->forwardLighting.GetOutputNodeIDs()[0];
	auto forwardDS = stages->forwardLighting.GetOutputNodeIDs()[1];
	fg.RegisterMoveNode(forwardDS, deferDS);
	auto taaPrevResult = fg.RegisterResourceNode("TAA Prev Result");
	const size_t taaInputs[3] = { taaPrevResult, sceneRT, motion };
	stages->taa.RegisterInputOutputPassNodes(fg, taaInputs);
	auto taaResult = stages->taa.GetOutputNodeIDs().front();
	stages->tonemapping.RegisterInputOutputPassNodes(fg, { &taaResult, 1 });
	auto tonemappedRT = stages->tonemapping.GetOutputNodeIDs()[0];
	auto presentedRT = fg.RegisterResourceNode("Present");
	fg.RegisterMoveNode(deferLightedSkyRT, deferLightedRT);
	fg.RegisterMoveNode(sceneRT, deferLightedSkyRT);
	fg.RegisterMoveNode(presentedRT, tonemappedRT);
	auto skyboxPass = fg.RegisterGeneralPassNode(
		"Skybox",
		{ deferDS },
		{ deferLightedSkyRT }
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

	D3D12_CLEAR_VALUE gbuffer_clearvalue = {};
	gbuffer_clearvalue.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	gbuffer_clearvalue.Color[0] = 0;
	gbuffer_clearvalue.Color[1] = 0;
	gbuffer_clearvalue.Color[2] = 0;
	gbuffer_clearvalue.Color[3] = 0;

	auto dsvDesc = UDX12::Desc::DSV::Basic(dsFormat);
	auto rt2dDesc = UDX12::Desc::RSRC::RT2D(width, (UINT)height, DXGI_FORMAT_R32G32B32A32_FLOAT);
	const UDX12::FG::RsrcImplDesc_RTV_Null rtvNull;

	auto iblData = worldRsrc.Get<std::shared_ptr<IBLData>>("IBL data");

	(*fgRsrcMngr)
		.RegisterTemporalRsrc(gbuffer0, rt2dDesc, gbuffer_clearvalue)
		.RegisterTemporalRsrc(gbuffer1, rt2dDesc, gbuffer_clearvalue)
		.RegisterTemporalRsrc(gbuffer2, rt2dDesc, gbuffer_clearvalue)
		.RegisterTemporalRsrc(motion, rt2dDesc, gbuffer_clearvalue)
		.RegisterTemporalRsrc(deferDS, dsDesc, dsClear)
		.RegisterTemporalRsrcAutoClear(deferLightedRT, rt2dDesc)
		//.RegisterTemporalRsrcAutoClear(deferLightedSkyRT, rt2dDesc)
		//.RegisterTemporalRsrcAutoClear(sceneRT, rt2dDesc)
		.RegisterTemporalRsrcAutoClear(taaResult, rt2dDesc)

		.RegisterImportedRsrc(tonemappedRT, {rtb, D3D12_RESOURCE_STATE_PRESENT})
		.RegisterImportedRsrc(irradianceMap, { iblData->irradianceMapResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })
		.RegisterImportedRsrc(prefilterMap, { iblData->prefilterMapResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })
		.RegisterImportedRsrc(taaPrevResult, { taaPrevRsrc.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })

		.RegisterPassRsrc(skyboxPass, deferDS, D3D12_RESOURCE_STATE_DEPTH_READ, dsvDesc)
		.RegisterPassRsrc(skyboxPass, deferLightedSkyRT, D3D12_RESOURCE_STATE_RENDER_TARGET, rtvNull)
		;

	stages->geometryBuffer.RegisterPassResources(*fgRsrcMngr);
	stages->deferredLighting.RegisterPassResources(*fgRsrcMngr);
	stages->forwardLighting.RegisterPassResources(*fgRsrcMngr);
	stages->taa.RegisterPassResources(*fgRsrcMngr);
	stages->tonemapping.RegisterPassResources(*fgRsrcMngr);

	auto shaderCBMngr = frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<std::shared_ptr<ShaderCBMngr>>("ShaderCBMngr");

	const D3D12_GPU_VIRTUAL_ADDRESS cameraCBAddress = shaderCBMngr->GetCameraCBAddress(ctx.ID, cameraIdx);
	const D3D12_GPU_VIRTUAL_ADDRESS lightCBAddress = shaderCBMngr->GetLightCBAddress(ctx.ID);
	// irradiance, prefilter, BRDF LUT
	const D3D12_GPU_DESCRIPTOR_HANDLE iblDataSrvGpuHandle = ctx.skyboxGpuHandle.ptr == PipelineCommonResourceMngr::GetInstance().GetDefaultSkyboxGpuHandle().ptr ?
		PipelineCommonResourceMngr::GetInstance().GetDefaultIBLSrvDHA().GetGpuHandle()
		: iblData->SRVDH.GetGpuHandle();

	stages->geometryBuffer.RegisterPassFuncData(cameraCBAddress, shaderCBMngr.get(), &ctx, "Deferred");
	stages->geometryBuffer.RegisterPassFunc(*fgExecutor);

	stages->deferredLighting.RegisterPassFuncData(iblDataSrvGpuHandle, cameraCBAddress, lightCBAddress);
	stages->deferredLighting.RegisterPassFunc(*fgExecutor);

	fgExecutor->RegisterPassFunc(
		skyboxPass,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			if (ctx.skyboxGpuHandle.ptr == PipelineCommonResourceMngr::GetInstance().GetDefaultSkyboxGpuHandle().ptr)
				return;

			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);
			auto skyboxShader = PipelineCommonResourceMngr::GetInstance().GetSkyboxShader();
			cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*skyboxShader));
			cmdList->SetPipelineState(GPURsrcMngrDX12::Instance().GetOrCreateShaderPSO(
				*skyboxShader,
				0,
				static_cast<size_t>(-1),
				1,
				DXGI_FORMAT_R32G32B32A32_FLOAT)
			);

			cmdList->RSSetViewports(1, &viewport);
			cmdList->RSSetScissorRects(1, &scissorRect);

			auto rt = rsrcs.at(deferLightedSkyRT);
			auto ds = rsrcs.at(deferDS);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &rt.info->null_info_rtv.cpuHandle, false, &ds.info->desc2info_dsv.at(dsvDesc).cpuHandle);

			cmdList->SetGraphicsRootDescriptorTable(0, ctx.skyboxGpuHandle);

			cmdList->SetGraphicsRootConstantBufferView(1, cameraCBAddress);

			cmdList->IASetVertexBuffers(0, 0, nullptr);
			cmdList->IASetIndexBuffer(nullptr);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(36, 1, 0, 0);
		}
	);

	stages->forwardLighting.RegisterPassFuncData(iblData->SRVDH.GetGpuHandle(), cameraCBAddress, shaderCBMngr.get(), &ctx);
	stages->forwardLighting.RegisterPassFunc(*fgExecutor);

	stages->taa.RegisterPassFuncData(cameraCBAddress);
	stages->taa.RegisterPassFunc(*fgExecutor);

	stages->tonemapping.RegisterPassFunc(*fgExecutor);

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

	UpdateCrossFrameCameraResources(cameras, defaultRTs);

	frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<std::shared_ptr<CameraRsrcMngr>>("CameraRsrcMngr")
		->Update(cameras);

	std::vector<std::vector<const UECS::World*>> worldArrs;
	for (const auto& link : links) {
		std::vector<const UECS::World*> worldArr;
		for (auto idx : link.worldIndices)
			worldArr.push_back(worlds[idx]);
		worldArrs.push_back(std::move(worldArr));
	}

	auto shaderCBMngr = frameRsrcMngr.GetCurrentFrameResource()
		->GetResource<std::shared_ptr<ShaderCBMngr>>("ShaderCBMngr");
	shaderCBMngr->NewFrame();

	std::vector<RenderContext> ctxs;
	for (size_t i = 0; i < worldArrs.size(); ++i) {
		auto ctx = GenerateRenderContext(i, worldArrs[i]);

		std::vector<CameraData> linkedCameras;
		for (size_t camIdx : links[i].cameraIndices)
			linkedCameras.push_back(cameras[camIdx]);
		std::vector<const CameraConstants*> cameraConstantsVector;
		for (const auto& linkedCamera : linkedCameras) {
			auto* camConts = &crossFrameCameraRsrcs.Get(linkedCamera).Get<CameraConstants>(key_CameraConstants);
			cameraConstantsVector.push_back(camConts);
		}
		shaderCBMngr->RegisterRenderContext(ctx, cameraConstantsVector);

		ctxs.push_back(std::move(ctx));
	}

	crossFrameWorldRsrcMngr.Update(worldArrs, initDesc.numFrame);
	auto curFrameWorldRsrcMngr = frameRsrcMngr.GetCurrentFrameResource()->GetResource<std::shared_ptr<WorldRsrcMngr>>("WorldRsrcMngr");
	curFrameWorldRsrcMngr->Update(worldArrs);

	for (size_t linkIdx = 0; linkIdx < links.size(); ++linkIdx) {
		auto& ctx = ctxs[linkIdx];
		auto& crossFrameWorldRsrc = crossFrameWorldRsrcMngr.Get(worldArrs[linkIdx]);
		auto& curFrameWorldRsrc = curFrameWorldRsrcMngr->Get(worldArrs[linkIdx]);

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
		if (!curFrameWorldRsrc.Contains("IBL data"))
			curFrameWorldRsrc.Register("IBL data", crossFrameWorldRsrc.Get<std::shared_ptr<IBLData>>("IBL data"));
		auto iblData = curFrameWorldRsrc.Get<std::shared_ptr<IBLData>>("IBL data");

		fg.Clear();

		auto irradianceMap = fg.RegisterResourceNode("Irradiance Map");
		auto prefilterMap = fg.RegisterResourceNode("PreFilter Map");
		auto iblPass = fg.RegisterGeneralPassNode(
			"IBL",
			{ },
			{ irradianceMap, prefilterMap }
		);

		(*fgRsrcMngr)
			.RegisterImportedRsrc(irradianceMap, { iblData->irradianceMapResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })
			.RegisterImportedRsrc(prefilterMap, { iblData->prefilterMapResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE })
			.RegisterPassRsrcState(iblPass, irradianceMap, D3D12_RESOURCE_STATE_RENDER_TARGET)
			.RegisterPassRsrcState(iblPass, prefilterMap, D3D12_RESOURCE_STATE_RENDER_TARGET)
			;

		fgExecutor->RegisterPassFunc(
			iblPass,
			[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& /*rsrcs*/) {
				if (iblData->lastSkybox.ptr == ctx.skyboxGpuHandle.ptr) {
					if (iblData->nextIdx >= 6 * (1 + IBLData::PreFilterMapMipLevels))
						return;
				}
				else {
					auto defaultSkyboxGpuHandle = PipelineCommonResourceMngr::GetInstance().GetDefaultSkyboxGpuHandle();
					if (ctx.skyboxGpuHandle.ptr == defaultSkyboxGpuHandle.ptr) {
						iblData->lastSkybox.ptr = defaultSkyboxGpuHandle.ptr;
						return;
					}
					iblData->lastSkybox = ctx.skyboxGpuHandle;
					iblData->nextIdx = 0;
				}

				auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
				cmdList->SetDescriptorHeaps(1, &heap);

				if (iblData->nextIdx < 6) { // irradiance
					cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*irradianceShader));
					cmdList->SetPipelineState(GPURsrcMngrDX12::Instance().GetOrCreateShaderPSO(
						*irradianceShader,
						0,
						static_cast<size_t>(-1),
						1,
						DXGI_FORMAT_R32G32B32A32_FLOAT,
						DXGI_FORMAT_UNKNOWN)
					);

					D3D12_VIEWPORT viewport;
					viewport.MinDepth = 0.f;
					viewport.MaxDepth = 1.f;
					viewport.TopLeftX = 0.f;
					viewport.TopLeftY = 0.f;
					viewport.Width = IBLData::IrradianceMapSize;
					viewport.Height = IBLData::IrradianceMapSize;
					D3D12_RECT rect = { 0,0,IBLData::IrradianceMapSize,IBLData::IrradianceMapSize };
					cmdList->RSSetViewports(1, &viewport);
					cmdList->RSSetScissorRects(1, &rect);

					cmdList->SetGraphicsRootDescriptorTable(0, ctx.skyboxGpuHandle);
					//for (UINT i = 0; i < 6; i++) {
					UINT i = static_cast<UINT>(iblData->nextIdx);
					// Specify the buffers we are going to render to.
					const auto iblRTVHandle = iblData->RTVsDH.GetCpuHandle(i);
					cmdList->OMSetRenderTargets(1, &iblRTVHandle, false, nullptr);
					auto address = crossFrameShaderCB.GetResource()->GetGPUVirtualAddress()
						+ i * UDX12::Util::CalcConstantBufferByteSize(sizeof(QuadPositionLs));
					cmdList->SetGraphicsRootConstantBufferView(1, address);

					cmdList->IASetVertexBuffers(0, 0, nullptr);
					cmdList->IASetIndexBuffer(nullptr);
					cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					cmdList->DrawInstanced(6, 1, 0, 0);
					//}
				}
				else { // prefilter
					cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*prefilterShader));
					cmdList->SetPipelineState(GPURsrcMngrDX12::Instance().GetOrCreateShaderPSO(
						*prefilterShader,
						0,
						static_cast<size_t>(-1),
						1,
						DXGI_FORMAT_R32G32B32A32_FLOAT,
						DXGI_FORMAT_UNKNOWN)
					);

					cmdList->SetGraphicsRootDescriptorTable(0, ctx.skyboxGpuHandle);
					//size_t size = Impl::IBLData::PreFilterMapSize;
					//for (UINT mip = 0; mip < Impl::IBLData::PreFilterMapMipLevels; mip++) {
					UINT mip = static_cast<UINT>((iblData->nextIdx - 6) / 6);
					size_t size = IBLData::PreFilterMapSize >> mip;
					auto mipinfo = crossFrameShaderCB.GetResource()->GetGPUVirtualAddress()
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

					//for (UINT i = 0; i < 6; i++) {
					UINT i = iblData->nextIdx % 6;
					auto positionLs = crossFrameShaderCB.GetResource()->GetGPUVirtualAddress()
						+ i * UDX12::Util::CalcConstantBufferByteSize(sizeof(QuadPositionLs));
					cmdList->SetGraphicsRootConstantBufferView(1, positionLs);

					// Specify the buffers we are going to render to.
					const auto iblRTVHandle = iblData->RTVsDH.GetCpuHandle(6 * (1 + mip) + i);
					cmdList->OMSetRenderTargets(1, &iblRTVHandle, false, nullptr);

					cmdList->IASetVertexBuffers(0, 0, nullptr);
					cmdList->IASetIndexBuffer(nullptr);
					cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					cmdList->DrawInstanced(6, 1, 0, 0);
					//}

					size /= 2;
					//}
				}

				iblData->nextIdx++;
			}
		);

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
			CameraRender(ctx, cameras[cameraIdx], i, defaultRTs[cameraIdx], curFrameWorldRsrc);
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

void StdPipeline::Impl::UpdateCameraResource(const CameraData& cameraData, size_t width, size_t height) {
	auto cameraRsrcMngr = frameRsrcMngr.GetCurrentFrameResource()->GetResource<std::shared_ptr<CameraRsrcMngr>>("CameraRsrcMngr");
	if (cameraRsrcMngr->Get(cameraData).Contains(key_CameraResizeData_Backup))
		cameraRsrcMngr->Get(cameraData).Unregister(key_CameraResizeData_Backup);
	if (!cameraRsrcMngr->Get(cameraData).Contains(key_CameraResizeData))
		cameraRsrcMngr->Get(cameraData).Register(key_CameraResizeData, CameraResizeData{});
	const auto& currCameraResizeData = cameraRsrcMngr->Get(cameraData).Get<CameraResizeData>(key_CameraResizeData);
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

	cameraRsrcMngr->Get(cameraData).Unregister(key_CameraResizeData);
	cameraRsrcMngr->Get(cameraData).Register(key_CameraResizeData, cameraResizeData);

	for (auto& frsrc : frameRsrcMngr.GetFrameResources()) {
		if (frsrc.get() == frameRsrcMngr.GetCurrentFrameResource())
			continue;

		auto cameraRsrcMngr = frsrc->GetResource<std::shared_ptr<CameraRsrcMngr>>("CameraRsrcMngr");
		if (!cameraRsrcMngr->Contains(cameraData))
			continue;

		if (!cameraRsrcMngr->Get(cameraData).Contains(key_CameraResizeData))
			cameraRsrcMngr->Get(cameraData).Register(key_CameraResizeData, CameraResizeData{});

		if (!cameraRsrcMngr->Get(cameraData).Contains(key_CameraResizeData_Backup)) {
			cameraRsrcMngr->Get(cameraData).Register(
				key_CameraResizeData_Backup,
				std::move(cameraRsrcMngr->Get(cameraData).Get<CameraResizeData>(key_CameraResizeData))
			);
		}

		cameraRsrcMngr->Get(cameraData).Get<CameraResizeData>(key_CameraResizeData) = cameraResizeData;
	}
}
