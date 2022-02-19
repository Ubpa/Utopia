#include "Sky.h"

#include <Utopia/Render/ShaderMngr.h>
#include <Utopia/Render/DX12/GPURsrcMngrDX12.h>

Ubpa::Utopia::Sky::Sky()
	: skyboxSrvGpuHandle{ .ptr = 0 }
	, cameraCBAddress(0)
	, depthStencilID(static_cast<size_t>(-1))
	, outputID(static_cast<size_t>(-1))
	, passID(static_cast<size_t>(-1))
{
	shader = ShaderMngr::Instance().Get("StdPipeline/Skybox");
	dsvDesc = UDX12::Desc::DSV::Basic(DXGI_FORMAT_D24_UNORM_S8_UINT);
}

Ubpa::Utopia::Sky::~Sky() = default;

void Ubpa::Utopia::Sky::NewFrame() {
	skyboxSrvGpuHandle.ptr = 0;
	cameraCBAddress = 0;

	depthStencilID = static_cast<size_t>(-1);

	outputID = 0;

	passID = static_cast<size_t>(-1);
}

bool Ubpa::Utopia::Sky::RegisterInputOutputPassNodes(UFG::FrameGraph& framegraph, std::span<const size_t> inputNodeIDs) {
	if (inputNodeIDs.size() != 1)
	{
		return false;
	}

	depthStencilID = inputNodeIDs[0];

	outputID = framegraph.RegisterResourceNode("Sky::Output");

	passID = framegraph.RegisterGeneralPassNode(
		"Sky",
		{ depthStencilID },
		{ outputID }
	);

	return true;
}

std::span<const size_t> Ubpa::Utopia::Sky::GetOutputNodeIDs() const {
	return { &outputID, 1 };
}

void Ubpa::Utopia::Sky::RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) {
	rsrcMngr.RegisterPassRsrc(
		passID,
		depthStencilID,
		D3D12_RESOURCE_STATE_DEPTH_READ,
		dsvDesc
	);

	rsrcMngr.RegisterPassRsrc(
		passID,
		outputID,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		UDX12::FG::RsrcImplDesc_RTV_Null{}
	);
}

void Ubpa::Utopia::Sky::RegisterPassFuncData(
	D3D12_GPU_DESCRIPTOR_HANDLE inSkyboxSrvGpuHandle,
	D3D12_GPU_VIRTUAL_ADDRESS inCameraCBAddress) {
	skyboxSrvGpuHandle = inSkyboxSrvGpuHandle;
	cameraCBAddress = inCameraCBAddress;
}

void Ubpa::Utopia::Sky::RegisterPassFunc(UDX12::FG::Executor& executor) {
	executor.RegisterPassFunc(
		passID,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			if (skyboxSrvGpuHandle.ptr == PipelineCommonResourceMngr::GetInstance().GetDefaultSkyboxGpuHandle().ptr)
				return;

			const auto& ds = rsrcs.at(depthStencilID);
			const auto& rt = rsrcs.at(outputID);

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

			cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*shader));
			cmdList->SetPipelineState(GPURsrcMngrDX12::Instance().GetOrCreateShaderPSO(
				*shader,
				0,
				static_cast<size_t>(-1),
				1,
				DXGI_FORMAT_R32G32B32A32_FLOAT)
			);

			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);

			cmdList->RSSetViewports(1, &viewport);
			cmdList->RSSetScissorRects(1, &scissorRect);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &rt.info->null_info_rtv.cpuHandle, false, &ds.info->desc2info_dsv.at(dsvDesc).cpuHandle);

			cmdList->SetGraphicsRootDescriptorTable(0, skyboxSrvGpuHandle);

			cmdList->SetGraphicsRootConstantBufferView(1, cameraCBAddress);

			cmdList->IASetVertexBuffers(0, 0, nullptr);
			cmdList->IASetIndexBuffer(nullptr);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(36, 1, 0, 0);
		}
	);
}
