#include "Tonemapping.h"

#include <Utopia/Render/ShaderMngr.h>
#include <Utopia/Render/DX12/GPURsrcMngrDX12.h>

Ubpa::Utopia::Tonemapping::Tonemapping()
	: inputID(static_cast<size_t>(-1))
	, outputID(static_cast<size_t>(-1))
	, passID(static_cast<size_t>(-1))
{
	shader = ShaderMngr::Instance().Get("StdPipeline/Tonemapping");
	inputSrvDesc = UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT);
}

Ubpa::Utopia::Tonemapping::~Tonemapping() {
	//shader.reset();
	//inputSrvDesc = {};
	//inputID = static_cast<size_t>(-1);
	//outputID = static_cast<size_t>(-1);
	//passID = static_cast<size_t>(-1);
}

void Ubpa::Utopia::Tonemapping::NewFrame() {
	inputID = static_cast<size_t>(-1);
	outputID = static_cast<size_t>(-1);
	passID = static_cast<size_t>(-1);
}

bool Ubpa::Utopia::Tonemapping::RegisterInputNodes(std::span<const size_t> inputNodeIDs) {
	if (inputNodeIDs.size() != 1)
	{
		return false;
	}

	inputID = inputNodeIDs[0];

	return true;
}

void Ubpa::Utopia::Tonemapping::RegisterOutputNodes(UFG::FrameGraph& framegraph) {
	outputID = framegraph.RegisterResourceNode("Tonemapping::output");
}

void Ubpa::Utopia::Tonemapping::RegisterPass(UFG::FrameGraph& framegraph) {
	passID = framegraph.RegisterGeneralPassNode("Tonemapping", { inputID }, { outputID });
}

std::span<const size_t> Ubpa::Utopia::Tonemapping::GetOutputNodeIDs() const {
	return { &outputID, 1 };
}

void Ubpa::Utopia::Tonemapping::RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) {
	rsrcMngr.RegisterPassRsrc(
		passID,
		inputID,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		inputSrvDesc
	);

	rsrcMngr.RegisterPassRsrc(
		passID,
		outputID,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		UDX12::FG::RsrcImplDesc_RTV_Null{}
	);
}

void Ubpa::Utopia::Tonemapping::RegisterPassFunc(UDX12::FG::Executor& executor) {
	executor.RegisterPassFunc(
		passID,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			auto img = rsrcs.at(inputID);
			auto rt = rsrcs.at(outputID);

			cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*shader));
			cmdList->SetPipelineState(GPURsrcMngrDX12::Instance().GetOrCreateShaderPSO(
				*shader,
				0,
				static_cast<size_t>(-1),
				1,
				rt.resource->GetDesc().Format,
				DXGI_FORMAT_UNKNOWN)
			);

			auto heap = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->GetDescriptorHeap();
			cmdList->SetDescriptorHeaps(1, &heap);

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

			cmdList->RSSetViewports(1, &viewport);
			cmdList->RSSetScissorRects(1, &scissorRect);

			// Clear the render texture and depth buffer.
			//cmdList->ClearRenderTargetView(rt.info->null_info_rtv.cpuHandle, DirectX::Colors::Black, 0, nullptr);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &rt.info->null_info_rtv.cpuHandle, false, nullptr);

			cmdList->SetGraphicsRootDescriptorTable(0, img.info->desc2info_srv.at(inputSrvDesc).gpuHandle);

			cmdList->IASetVertexBuffers(0, 0, nullptr);
			cmdList->IASetIndexBuffer(nullptr);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(6, 1, 0, 0);
		}
	);
}
