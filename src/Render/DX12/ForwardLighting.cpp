#include "ForwardLighting.h"

#include <Utopia/Render/ShaderMngr.h>
#include <Utopia/Render/DX12/GPURsrcMngrDX12.h>

Ubpa::Utopia::ForwardLighting::ForwardLighting()
	: cameraCBAddress(0)
	, shaderCBMngr(nullptr)
	, renderCtx(nullptr)
	, iblData(nullptr)
	, irradianceMapID(static_cast<size_t>(-1))
	, prefillteredMapID(static_cast<size_t>(-1))
	, outputIDs{
		static_cast<size_t>(-1),
		static_cast<size_t>(-1),
	}
	, passID(static_cast<size_t>(-1))
{
	shader = ShaderMngr::Instance().Get("StdPipeline/Forward Lighting");
	dsvDesc = UDX12::Desc::DSV::Basic(DXGI_FORMAT_D24_UNORM_S8_UINT);
}

Ubpa::Utopia::ForwardLighting::~ForwardLighting() {}

void Ubpa::Utopia::ForwardLighting::NewFrame() {
	cameraCBAddress = 0;
	shaderCBMngr = nullptr;
	renderCtx = nullptr;
	iblData = nullptr;

	irradianceMapID = static_cast<size_t>(-1);
	prefillteredMapID = static_cast<size_t>(-1);

	for (size_t i = 0; i < 2; i++)
		outputIDs[i] = static_cast<size_t>(-1);

	passID = static_cast<size_t>(-1);
}

bool Ubpa::Utopia::ForwardLighting::RegisterInputOutputPassNodes(UFG::FrameGraph& framegraph, std::span<const size_t> inputNodeIDs) {
	if (inputNodeIDs.size() != 2)
	{
		return false;
	}

	irradianceMapID = inputNodeIDs[0];
	prefillteredMapID = inputNodeIDs[1];

	outputIDs[0] = framegraph.RegisterResourceNode("ForwardLighting::LightedResult");

	outputIDs[1] = framegraph.RegisterResourceNode("ForwardLighting::DepthStencil");

	const std::span<const size_t> outputIDs = GetOutputNodeIDs();
	passID = framegraph.RegisterGeneralPassNode(
		"ForwardLighting",
		{ irradianceMapID, prefillteredMapID },
		{ outputIDs.begin(), outputIDs.end() }
	);

	return true;
}

std::span<const size_t> Ubpa::Utopia::ForwardLighting::GetOutputNodeIDs() const {
	return { outputIDs, 2 };
}

void Ubpa::Utopia::ForwardLighting::RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) {
	rsrcMngr
		.RegisterPassRsrc(
			passID,
			outputIDs[0],
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			UDX12::FG::RsrcImplDesc_RTV_Null{}
		)
		.RegisterPassRsrc(
			passID,
			outputIDs[1],
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			dsvDesc
		);

	rsrcMngr
		.RegisterPassRsrcState(
			passID,
			irradianceMapID,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		)
		.RegisterPassRsrcState(
			passID,
			prefillteredMapID,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
}

void Ubpa::Utopia::ForwardLighting::RegisterPassFuncData(
	D3D12_GPU_VIRTUAL_ADDRESS inCameraCBAddress,
	const ShaderCBMngr* inShaderCBMngr,
	const RenderContext* inRenderCtx,
	const IBLData* inIblData
) {
	cameraCBAddress = inCameraCBAddress;
	shaderCBMngr = inShaderCBMngr;
	renderCtx = inRenderCtx;
	iblData = inIblData;
}

void Ubpa::Utopia::ForwardLighting::RegisterPassFunc(UDX12::FG::Executor& executor) {
	executor.RegisterPassFunc(
		passID,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			auto rt = rsrcs.at(outputIDs[0]);
			auto ds = rsrcs.at(outputIDs[1]);

			D3D12_VIEWPORT viewport;
			viewport.MinDepth = 0.f;
			viewport.MaxDepth = 1.f;
			viewport.TopLeftX = 0.f;
			viewport.TopLeftY = 0.f;
			viewport.Width = rt.resource->GetDesc().Width;
			viewport.Height = rt.resource->GetDesc().Height;

			D3D12_RECT scissorRect;
			scissorRect.left = 0;
			scissorRect.top = 0;
			scissorRect.right = viewport.Width;
			scissorRect.bottom = viewport.Height;

			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);

			cmdList->RSSetViewports(1, &viewport);
			cmdList->RSSetScissorRects(1, &scissorRect);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &rt.info->null_info_rtv.cpuHandle, false, &ds.info->desc2info_dsv.at(dsvDesc).cpuHandle);

			DrawObjects(
				*shaderCBMngr,
				*renderCtx,
				cmdList,
				"Forward",
				1,
				DXGI_FORMAT_R32G32B32A32_FLOAT,
				cameraCBAddress,
				*iblData);
		}
	);
}
