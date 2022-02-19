#include "GeometryBuffer.h"

#include <Utopia/Render/ShaderMngr.h>
#include <Utopia/Render/DX12/GPURsrcMngrDX12.h>

Ubpa::Utopia::GeometryBuffer::GeometryBuffer(bool inNeedLinearZ)
	: needLinearZ(inNeedLinearZ)
	, cameraCBAddress(0)
	, shaderCBMngr(nullptr)
	, renderCtx(nullptr)
	, iblData(nullptr)
	, outputIDs {
		static_cast<size_t>(-1),
		static_cast<size_t>(-1),
		static_cast<size_t>(-1),
		static_cast<size_t>(-1),
		static_cast<size_t>(-1),
		static_cast<size_t>(-1),
	}
	, passID(static_cast<size_t>(-1))
{
	shader = ShaderMngr::Instance().Get("StdPipeline/Geometry Buffer");
	dsvDesc = UDX12::Desc::DSV::Basic(DXGI_FORMAT_D24_UNORM_S8_UINT);
}

Ubpa::Utopia::GeometryBuffer::~GeometryBuffer() {
}

void Ubpa::Utopia::GeometryBuffer::NewFrame() {
	for (size_t i = 0; i < 6; i++)
		outputIDs[i] = static_cast<size_t>(-1);

	cameraCBAddress = 0;
	shaderCBMngr = nullptr;
	renderCtx = nullptr;
	iblData = nullptr;
	lightMode.clear();

	passID = static_cast<size_t>(-1);
}

bool Ubpa::Utopia::GeometryBuffer::RegisterInputOutputPassNodes(UFG::FrameGraph& framegraph, std::span<const size_t> inputNodeIDs) {
	if (inputNodeIDs.size() != 0)
	{
		return false;
	}

	outputIDs[0] = framegraph.RegisterResourceNode("GeometryBuffer::GeometryBuffer0");
	outputIDs[1] = framegraph.RegisterResourceNode("GeometryBuffer::GeometryBuffer1");
	outputIDs[2] = framegraph.RegisterResourceNode("GeometryBuffer::GeometryBuffer2");
	outputIDs[3] = framegraph.RegisterResourceNode("GeometryBuffer::Motion");
	outputIDs[4] = framegraph.RegisterResourceNode("GeometryBuffer::DepthStencil");
	if (needLinearZ)
		outputIDs[5] = framegraph.RegisterResourceNode("GeometryBuffer::LinearZ");

	const std::span<const size_t> outputIDs = GetOutputNodeIDs();
	passID = framegraph.RegisterGeneralPassNode(
		"GeometryBuffer",
		{ },
		{ outputIDs.begin(), outputIDs.end() }
	);

	return true;
}

std::span<const size_t> Ubpa::Utopia::GeometryBuffer::GetOutputNodeIDs() const {
	size_t num = 5;
	if (needLinearZ)
		++num;
	return { outputIDs, num };
}

void Ubpa::Utopia::GeometryBuffer::RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) {
	rsrcMngr.RegisterPassRsrc(
		passID,
		outputIDs[0],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		UDX12::FG::RsrcImplDesc_RTV_Null{}
	);
	rsrcMngr.RegisterPassRsrc(
		passID,
		outputIDs[1],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		UDX12::FG::RsrcImplDesc_RTV_Null{}
	);
	rsrcMngr.RegisterPassRsrc(
		passID,
		outputIDs[2],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		UDX12::FG::RsrcImplDesc_RTV_Null{}
	);
	rsrcMngr.RegisterPassRsrc(
		passID,
		outputIDs[3],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		UDX12::FG::RsrcImplDesc_RTV_Null{}
	);
	rsrcMngr.RegisterPassRsrc(
		passID,
		outputIDs[4],
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		dsvDesc
	);
	if (needLinearZ) {
		rsrcMngr.RegisterPassRsrc(
			passID,
			outputIDs[5],
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			UDX12::FG::RsrcImplDesc_RTV_Null{}
		);
	}
}

void Ubpa::Utopia::GeometryBuffer::RegisterPassFuncData(
	D3D12_GPU_VIRTUAL_ADDRESS inCameraCBAddress,
	const ShaderCBMngr* inShaderCBMngr,
	const RenderContext* inRenderCtx,
	const IBLData* inIblData,
	std::string inLightMode) {
	cameraCBAddress = inCameraCBAddress;
	shaderCBMngr = inShaderCBMngr;
	renderCtx = inRenderCtx;
	iblData = inIblData;
	lightMode = std::move(inLightMode);
}

void Ubpa::Utopia::GeometryBuffer::RegisterPassFunc(UDX12::FG::Executor& executor) {
	executor.RegisterPassFunc(
		passID,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			auto gb0 = rsrcs.at(outputIDs[0]);
			auto gb1 = rsrcs.at(outputIDs[1]);
			auto gb2 = rsrcs.at(outputIDs[2]);
			auto mt = rsrcs.at(outputIDs[3]);
			auto ds = rsrcs.at(outputIDs[4]);

			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);

			D3D12_VIEWPORT viewport;
			viewport.MinDepth = 0.f;
			viewport.MaxDepth = 1.f;
			viewport.TopLeftX = 0.f;
			viewport.TopLeftY = 0.f;
			viewport.Width = gb0.resource->GetDesc().Width;
			viewport.Height = gb0.resource->GetDesc().Height;

			D3D12_RECT scissorRect;
			scissorRect.left = 0;
			scissorRect.top = 0;
			scissorRect.right = viewport.Width;
			scissorRect.bottom = viewport.Height;

			cmdList->RSSetViewports(1, &viewport);
			cmdList->RSSetScissorRects(1, &scissorRect);

			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtHandles{
				gb0.info->null_info_rtv.cpuHandle,
				gb1.info->null_info_rtv.cpuHandle,
				gb2.info->null_info_rtv.cpuHandle,
				mt.info->null_info_rtv.cpuHandle,
			};
			if (needLinearZ)
				rtHandles.push_back(rsrcs.at(outputIDs[5]).info->null_info_rtv.cpuHandle);
			auto dsHandle = ds.info->desc2info_dsv.at(dsvDesc).cpuHandle;
			// Clear the render texture and depth buffer.
			constexpr FLOAT clearColor[4] = { 0.f,0.f,0.f,0.f };
			for (size_t i = 0; i < rtHandles.size(); i++)
				cmdList->ClearRenderTargetView(rtHandles[i], clearColor, 0, nullptr);
			cmdList->ClearDepthStencilView(dsHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets((UINT)rtHandles.size(), rtHandles.data(), false, &dsHandle);

			DrawObjects(
				*shaderCBMngr,
				*renderCtx,
				cmdList,
				lightMode,
				rtHandles.size(),
				DXGI_FORMAT_R32G32B32A32_FLOAT,
				cameraCBAddress,
				*iblData);
		}
	);
}
