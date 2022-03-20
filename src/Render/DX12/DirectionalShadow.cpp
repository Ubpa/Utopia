#include "DirectionalShadow.h"

#include <Utopia/Render/ShaderMngr.h>
#include <Utopia/Render/DX12/GPURsrcMngrDX12.h>

Ubpa::Utopia::DirectionalShadow::DirectionalShadow()
	: shaderCBMngr(nullptr)
	, renderCtx(nullptr)
	, depthID(static_cast<size_t>(-1))
	, passID(static_cast<size_t>(-1))
{
	material = PipelineCommonResourceMngr::GetInstance().GetDirectionalShadowMaterial();
	dsvDesc = UDX12::Desc::DSV::Basic(DXGI_FORMAT_D32_FLOAT);
}

Ubpa::Utopia::DirectionalShadow::~DirectionalShadow() = default;

void Ubpa::Utopia::DirectionalShadow::NewFrame() {
	shaderCBMngr = nullptr;
	renderCtx = nullptr;

	depthID = static_cast<size_t>(-1);

	passID = static_cast<size_t>(-1);
}

bool Ubpa::Utopia::DirectionalShadow::RegisterInputOutputPassNodes(UFG::FrameGraph& framegraph, std::span<const size_t> inputNodeIDs) {
	if (inputNodeIDs.size() != 0)
	{
		return false;
	}

	depthID = framegraph.RegisterResourceNode("DirectionalShadow::Depth");

	const std::span<const size_t> outputIDs = GetOutputNodeIDs();
	passID = framegraph.RegisterGeneralPassNode(
		"DirectionalShadow",
		{ },
		{ outputIDs.begin(), outputIDs.end() }
	);

	return true;
}

std::span<const size_t> Ubpa::Utopia::DirectionalShadow::GetOutputNodeIDs() const {
	return { &depthID, 1 };
}

void Ubpa::Utopia::DirectionalShadow::RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) {
	rsrcMngr.RegisterPassRsrc(
		passID,
		depthID,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		dsvDesc
	);
}

void Ubpa::Utopia::DirectionalShadow::RegisterPassFuncData(
	const ShaderCBMngr* inShaderCBMngr,
	const RenderContext* inRenderCtx) {
	shaderCBMngr = inShaderCBMngr;
	renderCtx = inRenderCtx;
}

void Ubpa::Utopia::DirectionalShadow::RegisterPassFunc(UDX12::FG::Executor& executor) {
	executor.RegisterPassFunc(
		passID,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			auto depth = rsrcs.at(depthID);

			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);

			D3D12_VIEWPORT viewport;
			viewport.MinDepth = 0.f;
			viewport.MaxDepth = 1.f;
			viewport.TopLeftX = 0.f;
			viewport.TopLeftY = 0.f;
			viewport.Width = depth.resource->GetDesc().Width;
			viewport.Height = depth.resource->GetDesc().Height;

			D3D12_RECT scissorRect;
			scissorRect.left = 0;
			scissorRect.top = 0;
			scissorRect.right = viewport.Width;
			scissorRect.bottom = viewport.Height;

			cmdList->RSSetViewports(1, &viewport);
			cmdList->RSSetScissorRects(1, &scissorRect);

			auto depthHandle = depth.info->desc2info_dsv.at(dsvDesc).cpuHandle;
			cmdList->ClearDepthStencilView(depthHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(0, nullptr, false, &depthHandle);

			DrawObjects(
				*shaderCBMngr,
				*renderCtx,
				cmdList,
				"DirectionalShadow",
				0,
				DXGI_FORMAT_UNKNOWN,
				dsvDesc.Format,
				0,
				D3D12_GPU_DESCRIPTOR_HANDLE{ .ptr = 0 },
				material.get());
		}
	);
}
