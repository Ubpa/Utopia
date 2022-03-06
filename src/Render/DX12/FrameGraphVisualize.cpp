#include <Utopia/Render/DX12/FrameGraphVisualize.h>

#include <Utopia/Render/ShaderMngr.h>
#include <Utopia/Render/DX12/GPURsrcMngrDX12.h>

Ubpa::Utopia::FrameGraphVisualize::FrameGraphVisualize()
	: width(0)
	, height(0)
	, passID(static_cast<size_t>(-1))
{
	shader = ShaderMngr::Instance().Get("StdPipeline/ImageScaler");
	outputSrvDesc = UDX12::Desc::SRV::Tex2D(outputFormat);
}

Ubpa::Utopia::FrameGraphVisualize::~FrameGraphVisualize() = default;

void Ubpa::Utopia::FrameGraphVisualize::NewFrame() {
	width = 0;
	height = 0;
	inputIDs.clear();
	outputIDs.clear();
	passID = static_cast<size_t>(-1);
	outputSrvGpuHandles.clear();
	outputSrvCpuHandles.clear();
}

bool Ubpa::Utopia::FrameGraphVisualize::RegisterInputOutputPassNodes(UFG::FrameGraph& framegraph, std::span<const size_t> inputNodeIDs) {
	inputIDs = std::vector(inputNodeIDs.begin(), inputNodeIDs.end());
	for (size_t inputID : inputIDs) {
		size_t outputID = framegraph.RegisterResourceNode("FrameGraphVisualize::" + std::string(framegraph.GetResourceNodes()[inputID].Name()));
		outputIDs.push_back(outputID);
	}

	passID = framegraph.RegisterGeneralPassNode("FrameGraphVisualize", inputIDs, outputIDs);

	return true;
}

std::span<const size_t> Ubpa::Utopia::FrameGraphVisualize::GetOutputNodeIDs() const {
	return outputIDs;
}

std::span<const D3D12_GPU_DESCRIPTOR_HANDLE> Ubpa::Utopia::FrameGraphVisualize::GetOutputSrvGpuHandles() const {
	return outputSrvGpuHandles;
}

std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> Ubpa::Utopia::FrameGraphVisualize::GetOutputSrvCpuHandles() const {
	return outputSrvCpuHandles;
}

void Ubpa::Utopia::FrameGraphVisualize::RegisterPassResourcesData(size_t inWidth, size_t inHeight) {
	width = inWidth;
	height = inHeight;
}

void Ubpa::Utopia::FrameGraphVisualize::RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) {
	for (size_t inputID : inputIDs) {
		const DXGI_FORMAT inputFormat = rsrcMngr.GetResourceFormat(inputID);
		assert(inputFormat != DXGI_FORMAT_UNKNOWN);
		rsrcMngr.RegisterPassRsrc(
			passID,
			inputID,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			UDX12::Desc::SRV::Tex2D(inputFormat)
		);
	}

	assert(width != 0 && height != 0);
	for (size_t outputID : outputIDs) {
		rsrcMngr.RegisterTemporalRsrc(outputID, UDX12::Desc::RSRC::RT2D(width, (UINT)height, outputFormat), false);
		rsrcMngr.RegisterPassRsrc(
			passID,
			outputID,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			UDX12::FG::RsrcImplDesc_RTV_Null{}
		);
		rsrcMngr.RegisterPassRsrc(
			passID,
			outputID,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			outputSrvDesc
		);
	}

}

void Ubpa::Utopia::FrameGraphVisualize::RegisterPassFunc(UDX12::FG::Executor& executor) {
	executor.RegisterPassFunc(
		passID,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*shader));

			for (size_t i = 0; i < inputIDs.size(); i++) {
				auto img = rsrcs.at(inputIDs[i]);
				auto rt = rsrcs.at(outputIDs[i]);

				const auto& rtHandle = rt.info->GetAnySRV(outputSrvDesc);
				outputSrvGpuHandles.push_back(rtHandle.gpuHandle);
				outputSrvCpuHandles.push_back(rtHandle.cpuHandle);

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

				cmdList->SetGraphicsRootDescriptorTable(0, img.info->GetAnySRV(UDX12::Desc::SRV::Tex2D(img.resource->GetDesc().Format)).gpuHandle);

				cmdList->IASetVertexBuffers(0, 0, nullptr);
				cmdList->IASetIndexBuffer(nullptr);
				cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				cmdList->DrawInstanced(6, 1, 0, 0);
			}
		}
	);
}
