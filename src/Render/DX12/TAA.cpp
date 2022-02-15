#include "TAA.h"

#include <Utopia/Render/ShaderMngr.h>
#include <Utopia/Render/DX12/GPURsrcMngrDX12.h>

Ubpa::Utopia::TAA::TAA()
	: prevSceneColorID(static_cast<size_t>(-1))
	, currSceneColorID(static_cast<size_t>(-1))
	, motionID(static_cast<size_t>(-1))
	, mixSceneColorID(static_cast<size_t>(-1))
	, taaPassID(static_cast<size_t>(-1))
	, copyPassID(static_cast<size_t>(-1))
{
	shader = ShaderMngr::Instance().Get("StdPipeline/TAA");
	prevSceneColorSrvDesc = UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT);
	currSceneColorSrvDesc = UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT);
	motionSrvDesc = UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT);
}

Ubpa::Utopia::TAA::~TAA() {
}

void Ubpa::Utopia::TAA::NewFrame() {
	prevSceneColorID = static_cast<size_t>(-1);
	currSceneColorID = static_cast<size_t>(-1);
	motionID = static_cast<size_t>(-1);
	mixSceneColorID = static_cast<size_t>(-1);

	cameraCB = 0;

	taaPassID = static_cast<size_t>(-1);
	copyPassID = static_cast<size_t>(-1);
}

bool Ubpa::Utopia::TAA::RegisterInputOutputPassNodes(UFG::FrameGraph& framegraph, std::span<const size_t> inputNodeIDs) {
	if (inputNodeIDs.size() != 3)
	{
		return false;
	}

	prevSceneColorID = inputNodeIDs[0];
	currSceneColorID = inputNodeIDs[1];
	motionID = inputNodeIDs[2];

	mixSceneColorID = framegraph.RegisterResourceNode("TAA::MixSceneColor");

	taaPassID = framegraph.RegisterGeneralPassNode(
		"TAA",
		{ prevSceneColorID, currSceneColorID, motionID },
		{ mixSceneColorID }
	);

	copyPassID = framegraph.RegisterCopyPassNode({ mixSceneColorID }, { prevSceneColorID });

	return true;
}

std::span<const size_t> Ubpa::Utopia::TAA::GetOutputNodeIDs() const {
	return { &mixSceneColorID, 1 };
}

void Ubpa::Utopia::TAA::RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) {
	rsrcMngr.RegisterPassRsrc(
		taaPassID,
		prevSceneColorID,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		prevSceneColorSrvDesc
	);

	rsrcMngr.RegisterPassRsrc(
		taaPassID,
		currSceneColorID,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		currSceneColorSrvDesc
	);

	rsrcMngr.RegisterPassRsrc(
		taaPassID,
		motionID,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		motionSrvDesc
	);

	rsrcMngr.RegisterPassRsrc(
		taaPassID,
		mixSceneColorID,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		UDX12::FG::RsrcImplDesc_RTV_Null{}
	);

	rsrcMngr.RegisterRsrcTable(
		RsrcTableID,
		{
			{ prevSceneColorID, prevSceneColorSrvDesc },
			{ currSceneColorID, currSceneColorSrvDesc },
		}
	);

	rsrcMngr.RegisterCopyPassRsrcState(
		copyPassID,
		{ &mixSceneColorID, 1 },
		{ &prevSceneColorID, 1 }
	);
}

void Ubpa::Utopia::TAA::RegisterPassFuncData(D3D12_GPU_VIRTUAL_ADDRESS inCameraCB) {
	cameraCB = inCameraCB;
}

void Ubpa::Utopia::TAA::RegisterPassFunc(UDX12::FG::Executor& executor) {
	executor.RegisterPassFunc(
		taaPassID,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			const auto& prevSceneColorRsrc = rsrcs.at(prevSceneColorID); // table {prev, curr}
			const auto& currSceneColorRsrc = rsrcs.at(currSceneColorID);
			const auto& motionRsrc = rsrcs.at(motionID);
			const auto& mixSceneColorRsrc = rsrcs.at(mixSceneColorID);

			cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*shader));
			cmdList->SetPipelineState(GPURsrcMngrDX12::Instance().GetOrCreateShaderPSO(
				*shader,
				0,
				static_cast<size_t>(-1),
				1,
				DXGI_FORMAT_R32G32B32A32_FLOAT,
				DXGI_FORMAT_UNKNOWN)
			);

			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);

			D3D12_VIEWPORT viewport;
			viewport.MinDepth = 0.f;
			viewport.MaxDepth = 1.f;
			viewport.TopLeftX = 0.f;
			viewport.TopLeftY = 0.f;
			viewport.Width = mixSceneColorRsrc.resource->GetDesc().Width;
			viewport.Height = mixSceneColorRsrc.resource->GetDesc().Height;

			D3D12_RECT scissorRect;
			scissorRect.left = 0;
			scissorRect.top = 0;
			scissorRect.right = viewport.Width;
			scissorRect.bottom = viewport.Height;

			cmdList->RSSetViewports(1, &viewport);
			cmdList->RSSetScissorRects(1, &scissorRect);

			// Clear the render texture and depth buffer.
			cmdList->ClearRenderTargetView(mixSceneColorRsrc.info->null_info_rtv.cpuHandle, DirectX::Colors::Black, 0, nullptr);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &mixSceneColorRsrc.info->null_info_rtv.cpuHandle, false, nullptr);

			cmdList->SetGraphicsRootDescriptorTable(0, prevSceneColorRsrc.info->desc2info_srv.at(prevSceneColorSrvDesc).at(RsrcTableID).gpuHandle);
			cmdList->SetGraphicsRootDescriptorTable(1, motionRsrc.info->GetAnySRV(motionSrvDesc).gpuHandle);

			assert(cameraCB != 0);
			cmdList->SetGraphicsRootConstantBufferView(2, cameraCB);

			cmdList->IASetVertexBuffers(0, 0, nullptr);
			cmdList->IASetIndexBuffer(nullptr);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(6, 1, 0, 0);
		}
	);

	executor.RegisterCopyPassFunc(copyPassID, { &mixSceneColorID, 1 }, { &prevSceneColorID, 1 });
}
