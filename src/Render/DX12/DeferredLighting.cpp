#include "DeferredLighting.h"

#include <Utopia/Render/ShaderMngr.h>
#include <Utopia/Render/DX12/GPURsrcMngrDX12.h>

Ubpa::Utopia::DeferredLighting::DeferredLighting()
	: geometryBuffer0ID(static_cast<size_t>(-1))
	, geometryBuffer1ID(static_cast<size_t>(-1))
	, geometryBuffer2ID(static_cast<size_t>(-1))
	, depthStencilID(static_cast<size_t>(-1))
	, irradianceMapID(static_cast<size_t>(-1))
	, prefilterMapID(static_cast<size_t>(-1))
	, outputID(static_cast<size_t>(-1))
	, passID(static_cast<size_t>(-1))
{
	shader = ShaderMngr::Instance().Get("StdPipeline/Deferred Lighting");
	geometryBufferSharedSrvDesc = UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT);
	dsvDesc = UDX12::Desc::DSV::Basic(DXGI_FORMAT_D24_UNORM_S8_UINT);
	dsSrvDesc = UDX12::Desc::SRV::Tex2D(DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
}

Ubpa::Utopia::DeferredLighting::~DeferredLighting() {}

void Ubpa::Utopia::DeferredLighting::NewFrame() {
	iblDataSrvGpuHandle.ptr = 0;
	lightCBAddress = 0;
	geometryBuffer0ID = static_cast<size_t>(-1);
	geometryBuffer1ID = static_cast<size_t>(-1);
	geometryBuffer2ID = static_cast<size_t>(-1);
	depthStencilID = static_cast<size_t>(-1);
	irradianceMapID = static_cast<size_t>(-1);
	prefilterMapID = static_cast<size_t>(-1);
	outputID = static_cast<size_t>(-1);
	passID = static_cast<size_t>(-1);
}

bool Ubpa::Utopia::DeferredLighting::RegisterInputOutputPassNodes(UFG::FrameGraph& framegraph, std::span<const size_t> inputNodeIDs) {
	if (inputNodeIDs.size() != 6)
	{
		return false;
	}

	geometryBuffer0ID = inputNodeIDs[0];
	geometryBuffer1ID = inputNodeIDs[1];
	geometryBuffer2ID = inputNodeIDs[2];
	depthStencilID = inputNodeIDs[3];
	irradianceMapID = inputNodeIDs[4];
	prefilterMapID = inputNodeIDs[5];

	outputID = framegraph.RegisterResourceNode("DeferredLighting::Output");

	passID = framegraph.RegisterGeneralPassNode(
		"DeferredLighting",
		{ geometryBuffer0ID, geometryBuffer1ID, geometryBuffer2ID,depthStencilID,irradianceMapID,prefilterMapID },
		{ outputID }
	);

	return true;
}

std::span<const size_t> Ubpa::Utopia::DeferredLighting::GetOutputNodeIDs() const {
	return { &outputID, 1 };
}

void Ubpa::Utopia::DeferredLighting::RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) {
	rsrcMngr
		.RegisterPassRsrc(
			passID,
			geometryBuffer0ID,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			geometryBufferSharedSrvDesc
		)
		.RegisterPassRsrc(
			passID,
			geometryBuffer1ID,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			geometryBufferSharedSrvDesc
		)
		.RegisterPassRsrc(
			passID,
			geometryBuffer2ID,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			geometryBufferSharedSrvDesc
		);

	rsrcMngr
		.RegisterPassRsrcState(
			passID,
			depthStencilID,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_DEPTH_READ
		)
		.RegisterPassRsrcImplDesc(passID, depthStencilID, dsvDesc)
		.RegisterPassRsrcImplDesc(passID, depthStencilID, dsSrvDesc);

	rsrcMngr
		.RegisterPassRsrcState(passID, irradianceMapID, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
		.RegisterPassRsrcState(passID, prefilterMapID, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	rsrcMngr
		.RegisterPassRsrc(passID, outputID, D3D12_RESOURCE_STATE_RENDER_TARGET, UDX12::FG::RsrcImplDesc_RTV_Null{});

	rsrcMngr
		.RegisterRsrcTable(
			RsrcTableID,
			{
				{geometryBuffer0ID,geometryBufferSharedSrvDesc},
				{geometryBuffer1ID,geometryBufferSharedSrvDesc},
				{geometryBuffer2ID,geometryBufferSharedSrvDesc},
			}
	);
}

void Ubpa::Utopia::DeferredLighting::RegisterPassFuncData(
	D3D12_GPU_DESCRIPTOR_HANDLE inIblDataSrvGpuHandle,
	D3D12_GPU_VIRTUAL_ADDRESS inCameraCBAddress,
	D3D12_GPU_VIRTUAL_ADDRESS inLightCBAddress
) {
	iblDataSrvGpuHandle = inIblDataSrvGpuHandle;
	cameraCBAddress = inCameraCBAddress;
	lightCBAddress = inLightCBAddress;
}

void Ubpa::Utopia::DeferredLighting::RegisterPassFunc(UDX12::FG::Executor& executor) {
	executor.RegisterPassFunc(
		passID,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			auto gb0 = rsrcs.at(geometryBuffer0ID);
			auto ds = rsrcs.at(depthStencilID);

			auto rt = rsrcs.at(outputID);

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
			cmdList->ClearRenderTargetView(rt.info->null_info_rtv.cpuHandle, DirectX::Colors::Black, 0, nullptr);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &rt.info->null_info_rtv.cpuHandle, false, &ds.info->desc2info_dsv.at(dsvDesc).cpuHandle);

			cmdList->SetGraphicsRootSignature(GPURsrcMngrDX12::Instance().GetShaderRootSignature(*shader));
			cmdList->SetPipelineState(GPURsrcMngrDX12::Instance().GetOrCreateShaderPSO(
				*shader,
				0,
				static_cast<size_t>(-1),
				1,
				DXGI_FORMAT_R32G32B32A32_FLOAT)
			);

			cmdList->SetGraphicsRootDescriptorTable(0, gb0.info->desc2info_srv.at(geometryBufferSharedSrvDesc).at(RsrcTableID).gpuHandle);
			cmdList->SetGraphicsRootDescriptorTable(1, ds.info->GetAnySRV(dsSrvDesc).gpuHandle);

			// irradiance, prefilter, BRDF LUT
			cmdList->SetGraphicsRootDescriptorTable(2, iblDataSrvGpuHandle);

			cmdList->SetGraphicsRootDescriptorTable(3, PipelineCommonResourceMngr::GetInstance().GetLtcSrvDHA().GetGpuHandle());

			cmdList->SetGraphicsRootConstantBufferView(4, lightCBAddress);

			cmdList->SetGraphicsRootConstantBufferView(5, cameraCBAddress);

			cmdList->IASetVertexBuffers(0, 0, nullptr);
			cmdList->IASetIndexBuffer(nullptr);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(6, 1, 0, 0);
		}
	);
}
