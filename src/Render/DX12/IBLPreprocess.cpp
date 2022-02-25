#include "IBLPreprocess.h"

#include <Utopia/Render/ShaderMngr.h>
#include <Utopia/Render/DX12/GPURsrcMngrDX12.h>

Ubpa::Utopia::IBLPreprocess::IBLPreprocess()
	: iblData(nullptr)
	, skyboxSrvGpuHandle{.ptr = 0}
	, outputIDs{
		static_cast<size_t>(-1),
		static_cast<size_t>(-1),
	}
	, passID(static_cast<size_t>(-1))
{
	irradianceShader = ShaderMngr::Instance().Get("StdPipeline/Irradiance");
	prefilterShader = ShaderMngr::Instance().Get("StdPipeline/Prefilter");
}

Ubpa::Utopia::IBLPreprocess::~IBLPreprocess() = default;

void Ubpa::Utopia::IBLPreprocess::NewFrame() {
	iblData = nullptr;
	skyboxSrvGpuHandle.ptr = 0;
	for (size_t i = 0; i < 2; i++)
		outputIDs[i] = static_cast<size_t>(-1);
	passID = static_cast<size_t>(-1);
}

bool Ubpa::Utopia::IBLPreprocess::RegisterInputOutputPassNodes(UFG::FrameGraph& framegraph, std::span<const size_t> inputNodeIDs) {
	if (inputNodeIDs.size() != 0)
	{
		return false;
	}

	outputIDs[0] = framegraph.RegisterResourceNode("IBLPreprocess::IrradianceMap");
	outputIDs[1] = framegraph.RegisterResourceNode("IBLPreprocess::PrefilterMap");

	const auto outputIDs = GetOutputNodeIDs();

	passID = framegraph.RegisterGeneralPassNode(
		"IBLPreprocess",
		{ },
		{ std::begin(outputIDs), std::end(outputIDs) }
	);

	return true;
}

std::span<const size_t> Ubpa::Utopia::IBLPreprocess::GetOutputNodeIDs() const {
	return { outputIDs, 2 };
}

void Ubpa::Utopia::IBLPreprocess::RegisterPassResources(UDX12::FG::RsrcMngr& rsrcMngr) {
	rsrcMngr
		.RegisterPassRsrcState(
			passID,
			outputIDs[0],
			D3D12_RESOURCE_STATE_RENDER_TARGET
		)
		.RegisterPassRsrcState(
			passID,
			outputIDs[1],
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
}

void Ubpa::Utopia::IBLPreprocess::RegisterPassFuncData(
	IBLData* inIblData,
	D3D12_GPU_DESCRIPTOR_HANDLE inSkyboxSrvGpuHandle)
{
	iblData = inIblData;
	skyboxSrvGpuHandle = inSkyboxSrvGpuHandle;
}

void Ubpa::Utopia::IBLPreprocess::RegisterPassFunc(UDX12::FG::Executor& executor) {
	executor.RegisterPassFunc(
		passID,
		[&](ID3D12GraphicsCommandList* cmdList, const UDX12::FG::PassRsrcs& rsrcs) {
			if (iblData->lastSkybox.ptr == skyboxSrvGpuHandle.ptr) {
				if (iblData->nextIdx >= 6 * (1 + IBLData::PreFilterMapMipLevels))
					return;
			}
			else {
				auto defaultSkyboxGpuHandle = PipelineCommonResourceMngr::GetInstance().GetDefaultSkyboxGpuHandle();
				if (skyboxSrvGpuHandle.ptr == defaultSkyboxGpuHandle.ptr) {
					iblData->lastSkybox.ptr = defaultSkyboxGpuHandle.ptr;
					return;
				}
				iblData->lastSkybox = skyboxSrvGpuHandle;
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

				cmdList->SetGraphicsRootDescriptorTable(0, skyboxSrvGpuHandle);
				//for (UINT i = 0; i < 6; i++) {
				UINT i = static_cast<UINT>(iblData->nextIdx);
				// Specify the buffers we are going to render to.
				const auto iblRTVHandle = iblData->RTVsDH.GetCpuHandle(i);
				cmdList->OMSetRenderTargets(1, &iblRTVHandle, false, nullptr);
				cmdList->SetGraphicsRootConstantBufferView(1, PipelineCommonResourceMngr::GetInstance().GetQuadPositionLocalGpuAddress(i));

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

				cmdList->SetGraphicsRootDescriptorTable(0, skyboxSrvGpuHandle);
				//size_t size = Impl::IBLData::PreFilterMapSize;
				//for (UINT mip = 0; mip < Impl::IBLData::PreFilterMapMipLevels; mip++) {
				UINT mip = static_cast<UINT>((iblData->nextIdx - 6) / 6);
				size_t size = IBLData::PreFilterMapSize >> mip;
				cmdList->SetGraphicsRootConstantBufferView(2, PipelineCommonResourceMngr::GetInstance().GetMipInfoGpuAddress(mip));

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
				cmdList->SetGraphicsRootConstantBufferView(1, PipelineCommonResourceMngr::GetInstance().GetQuadPositionLocalGpuAddress(i));

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
}
