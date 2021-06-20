#include <Utopia/Render/DX12/GPURsrcMngrDX12.h>

#include <Utopia/Render/Texture2D.h>
#include <Utopia/Render/TextureCube.h>
#include <Utopia/Render/RenderTargetTexture2D.h>
#include <Utopia/Core/Image.h>
#include <Utopia/Render/HLSLFile.h>
#include <Utopia/Render/Shader.h>
#include <Utopia/Render/Mesh.h>
#include <Utopia/Render/DX12/MeshLayoutMngr.h>

#include <unordered_map>
#include <iostream>
#include <mutex>

using namespace Ubpa::Utopia;
using namespace Ubpa;
using namespace std;

struct GPURsrcMngrDX12::Impl {
	struct Texture2DGPUData {
		Texture2DGPUData() = default;
		Texture2DGPUData(const Texture2DGPUData&) = default;
		Texture2DGPUData(Texture2DGPUData&&) noexcept = default;
		~Texture2DGPUData() {
			if(!allocationSRV.IsNull())
				UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(allocationSRV));
		}

		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		UDX12::DescriptorHeapAllocation allocationSRV;
	};
	struct TextureCubeGPUData {
		TextureCubeGPUData() = default;
		TextureCubeGPUData(const TextureCubeGPUData&) = default;
		TextureCubeGPUData(TextureCubeGPUData&&) noexcept = default;
		~TextureCubeGPUData() {
			if (!allocationSRV.IsNull())
				UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(allocationSRV));
		}

		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		UDX12::DescriptorHeapAllocation allocationSRV;
	};
	struct RenderTargetTexture2DGPUData {
		RenderTargetTexture2DGPUData() = default;
		RenderTargetTexture2DGPUData(const RenderTargetTexture2DGPUData&) = default;
		RenderTargetTexture2DGPUData(RenderTargetTexture2DGPUData&&) noexcept = default;
		~RenderTargetTexture2DGPUData() {
			if (!allocationSRV.IsNull())
				UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(allocationSRV));
			if (!allocationRTV.IsNull())
				UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Free(std::move(allocationRTV));
		}

		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		UDX12::DescriptorHeapAllocation allocationSRV;
		UDX12::DescriptorHeapAllocation allocationRTV;
	};
	struct ShaderCompileData {
		struct PassData {
			Microsoft::WRL::ComPtr<ID3DBlob> vsByteCode;
			Microsoft::WRL::ComPtr<ID3DBlob> psByteCode;
			Microsoft::WRL::ComPtr<ID3D12ShaderReflection> vsRefl;
			Microsoft::WRL::ComPtr<ID3D12ShaderReflection> psRefl;

			struct PSOKey {
				PSOKey() { memset(this, 0, sizeof(PSOKey)); }
				size_t layoutID;
				size_t rtNum;
				DXGI_FORMAT rtFormat;
				bool operator==(const PSOKey& rhs) const noexcept {
					return UDX12::FG::detail::bitwise_equal(*this, rhs);
				}
			};
			struct PSOKeyHasher {
				size_t operator()(const PSOKey& key) const noexcept {
					return UDX12::FG::detail::hash_of(key);
				}
			};
			std::unordered_map<PSOKey, Microsoft::WRL::ComPtr<ID3D12PipelineState>, PSOKeyHasher> PSOs;
		};
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
		std::vector<PassData> passes;
	};
	struct DXRMeshData {
		DXRMeshData() = default;
		DXRMeshData(const DXRMeshData&) = default;
		DXRMeshData(DXRMeshData&&) noexcept = default;
		~DXRMeshData() {
			if (!allocation.IsNull())
				UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(allocation));
		}

		Microsoft::WRL::ComPtr<ID3D12Resource> blas;
		UDX12::DescriptorHeapAllocation allocation; // {vb, ib}, ...
	};

	bool isInit{ false };
	bool enable_DXR{ false };
	ID3D12Device* device{ nullptr };
	DirectX::ResourceUploadBatch* upload{ nullptr };
	bool hasUpload = false;
	UDX12::ResourceDeleteBatch deleteBatch;

	unordered_map<std::size_t, Texture2DGPUData> texture2DMap;
	unordered_map<std::size_t, TextureCubeGPUData> textureCubeMap;
	unordered_map<std::size_t, RenderTargetTexture2DGPUData> rtTexture2DMap;
	unordered_map<std::size_t, ShaderCompileData> shaderMap;
	unordered_map<std::size_t, UDX12::MeshGPUBuffer> meshMap;
	unordered_map<std::size_t, DXRMeshData> dxrMeshDataMap;

	std::mutex mutex_shaderMap;

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap{
		0,                               // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT,  // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP  // addressW
	};

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp{
		1,                                // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT,   // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP  // addressW
	};

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap{
		2,                               // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP  // addressW
	};

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp{
		3,                                // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,  // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP  // addressW
	};

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap{
		4,                               // shaderRegister
		D3D12_FILTER_ANISOTROPIC,        // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressW
		0.0f,                            // mipLODBias
		8                                // maxAnisotropy
	};

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp{
		5,                                // shaderRegister
		D3D12_FILTER_ANISOTROPIC,         // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressW
		0.0f,                             // mipLODBias
		8                                 // maxAnisotropy
	};
};

GPURsrcMngrDX12::GPURsrcMngrDX12()
	: pImpl(new Impl)
{
}

GPURsrcMngrDX12::~GPURsrcMngrDX12() {
	assert(!pImpl->isInit);
	delete(pImpl);
}

GPURsrcMngrDX12& GPURsrcMngrDX12::Init(ID3D12Device* device, bool enable_DXR) {
	assert(!pImpl->isInit);

	pImpl->device = device;
	pImpl->enable_DXR = enable_DXR;

	pImpl->upload = new DirectX::ResourceUploadBatch{ device };
	pImpl->upload->Begin();

	pImpl->isInit = true;
	return *this;
}

void GPURsrcMngrDX12::Clear(ID3D12CommandQueue* cmdQueue) {
	assert(pImpl->isInit);

	pImpl->upload->End(cmdQueue);
	pImpl->deleteBatch.Release(); // maybe we need to return the delete callback

	pImpl->device = nullptr;
	delete pImpl->upload;

	pImpl->texture2DMap.clear();
	pImpl->textureCubeMap.clear();
	pImpl->rtTexture2DMap.clear();
	pImpl->meshMap.clear();
	pImpl->dxrMeshDataMap.clear();
	pImpl->shaderMap.clear();

	pImpl->enable_DXR = false;

	pImpl->isInit = false;
}

UDX12::ResourceDeleteBatch GPURsrcMngrDX12::CommitUploadAndTakeDeleteBatch(ID3D12CommandQueue* cmdQueue) {
	if (pImpl->hasUpload) {
		pImpl->upload->End(cmdQueue);
		pImpl->hasUpload = false;
		pImpl->upload->Begin();
	}

	return std::move(pImpl->deleteBatch);
}

GPURsrcMngrDX12& GPURsrcMngrDX12::RegisterTexture2D(Texture2D& tex2D) {
	if (tex2D.IsDirty()) {
		UnregisterTexture2D(tex2D.GetInstanceID());
		UnregisterRenderTargetTexture2D(tex2D.GetInstanceID());
	}
	else {
		auto target = pImpl->texture2DMap.find(tex2D.GetInstanceID());
		if (target != pImpl->texture2DMap.end())
			return *this;
	}

	Impl::Texture2DGPUData tex;

	tex.allocationSRV = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(static_cast<uint32_t>(1));

	constexpr DXGI_FORMAT channelMap[] = {
		DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
		DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
	};

	D3D12_SUBRESOURCE_DATA data;
	data.pData = tex2D.image.GetData();
	data.RowPitch = tex2D.image.GetWidth() * tex2D.image.GetChannel() * sizeof(float);
	data.SlicePitch = 0; // this field is useless for texture 2d

	pImpl->hasUpload = true;
	DirectX::CreateTextureFromMemory(
		pImpl->device,
		*pImpl->upload,
		tex2D.image.GetWidth(),
		tex2D.image.GetHeight(),
		channelMap[tex2D.image.GetChannel() - 1],
		data,
		&tex.resource
	);

	auto tex2DSRVDesc = UDX12::Desc::SRV::Tex2D(tex.resource->GetDesc().Format);
	std::uint8_t srv_srcs[4];
	for (std::uint8_t i = 0; i < tex2D.image.GetChannel(); i++)
		srv_srcs[i] = i;
	for (std::uint8_t i = tex2D.image.GetChannel(); i < 4; i++)
		srv_srcs[i] = D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1;
	tex2DSRVDesc.Shader4ComponentMapping =
		D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(srv_srcs[0], srv_srcs[1], srv_srcs[2], srv_srcs[3]);
	pImpl->device->CreateShaderResourceView(
		tex.resource.Get(),
		&tex2DSRVDesc,
		tex.allocationSRV.GetCpuHandle(static_cast<uint32_t>(0))
	);

	pImpl->texture2DMap.emplace(tex2D.GetInstanceID(), move(tex));

	tex2D.SetClean();
	tex2D.destroyed.Connect<&GPURsrcMngrDX12::UnregisterTexture2D>(this);
	return *this;
}

GPURsrcMngrDX12& GPURsrcMngrDX12::UnregisterTexture2D(std::size_t ID) {
	static std::mutex m;
	std::lock_guard guard{ m };
	if (auto target = pImpl->texture2DMap.find(ID); target != pImpl->texture2DMap.end()) {
		auto& tex = target->second;
		pImpl->deleteBatch.Add(std::move(tex.resource));
		pImpl->deleteBatch.AddCallback([alloc = std::make_shared<UDX12::DescriptorHeapAllocation>(std::move(tex.allocationSRV))]() {
			UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(*alloc));
		});
		pImpl->texture2DMap.erase(target);
	}
	return *this;
}

GPURsrcMngrDX12& GPURsrcMngrDX12::RegisterTextureCube(TextureCube& texcube) {
	if (texcube.IsDirty())
		UnregisterTextureCube(texcube.GetInstanceID());
	else {
		auto target = pImpl->textureCubeMap.find(texcube.GetInstanceID());
		if (target != pImpl->textureCubeMap.end())
			return *this;
	}

	Impl::TextureCubeGPUData tex;

	tex.allocationSRV = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(static_cast<uint32_t>(1));

	constexpr DXGI_FORMAT channelMap[] = {
		DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
		DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
	};

	size_t w = texcube.GetSixSideImages().front().GetWidth();
	size_t h = texcube.GetSixSideImages().front().GetHeight();
	size_t c = texcube.GetSixSideImages().front().GetChannel();

	std::array<D3D12_SUBRESOURCE_DATA, 6> datas;
	for (size_t i = 0; i < datas.size(); i++) {
		datas[i].pData = texcube.GetSixSideImages()[i].GetData();
		datas[i].RowPitch = texcube.GetSixSideImages()[i].GetWidth() * texcube.GetSixSideImages()[i].GetChannel() * sizeof(float);
		datas[i].SlicePitch = texcube.GetSixSideImages()[i].GetHeight() * datas[i].RowPitch; // this field is useless for texture 2d
	}

	pImpl->hasUpload = true;
	UDX12::Util::CreateTexture2DArrayFromMemory(
		pImpl->device,
		*pImpl->upload,
		w, h, 6,
		channelMap[c-1],
		datas.data(),
		&tex.resource
	);

	const auto texCubeSRVDesc = UDX12::Desc::SRV::TexCube(tex.resource->GetDesc().Format);
	pImpl->device->CreateShaderResourceView(
		tex.resource.Get(),
		&texCubeSRVDesc,
		tex.allocationSRV.GetCpuHandle(static_cast<uint32_t>(0))
	);

	pImpl->textureCubeMap.emplace(std::make_pair(texcube.GetInstanceID(), std::move(tex)));

	texcube.SetClean();
	texcube.destroyed.Connect<&GPURsrcMngrDX12::UnregisterTextureCube>(this);
	return *this;
}

GPURsrcMngrDX12& GPURsrcMngrDX12::UnregisterTextureCube(std::size_t ID) {
	static std::mutex m;
	std::lock_guard guard{ m };
	if (auto target = pImpl->textureCubeMap.find(ID); target != pImpl->textureCubeMap.end()) {
		auto& tex = target->second;
		pImpl->deleteBatch.Add(std::move(tex.resource));
		pImpl->deleteBatch.AddCallback([alloc = std::make_shared<UDX12::DescriptorHeapAllocation>(std::move(tex.allocationSRV))]() {
			UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(*alloc));
		});
		pImpl->textureCubeMap.erase(target);
	}
	return *this;
}

GPURsrcMngrDX12& GPURsrcMngrDX12::RegisterRenderTargetTexture2D(RenderTargetTexture2D& rtTex2D) {
	if (rtTex2D.IsDirty()) {
		UnregisterTexture2D(rtTex2D.GetInstanceID());
		UnregisterRenderTargetTexture2D(rtTex2D.GetInstanceID());
	}
	else {
		auto target = pImpl->rtTexture2DMap.find(rtTex2D.GetInstanceID());
		if (target != pImpl->rtTexture2DMap.end())
			return *this;
	}

	Impl::RenderTargetTexture2DGPUData data;

	DXGI_FORMAT formatMap[] = {
		DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT_R32G32B32_FLOAT,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
	};
	DXGI_FORMAT format = formatMap[rtTex2D.image.GetChannel()];
	Ubpa::rgbaf background = { 0.f,0.f,0.f,1.f };
	CD3DX12_CLEAR_VALUE clearvalue(format, background.data());
	auto desc = Ubpa::UDX12::Desc::RSRC::RT2D(rtTex2D.image.GetWidth(), (UINT)rtTex2D.image.GetHeight(), format);
	const auto defaultHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(pImpl->device->CreateCommittedResource(
		&defaultHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_PRESENT,
		&clearvalue,
		IID_PPV_ARGS(&data.resource)
	));

	data.allocationSRV = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(1);
	data.allocationRTV = UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Allocate(1);
	pImpl->device->CreateShaderResourceView(data.resource.Get(), nullptr, data.allocationSRV.GetCpuHandle());
	pImpl->device->CreateRenderTargetView(data.resource.Get(), nullptr, data.allocationRTV.GetCpuHandle());

	pImpl->rtTexture2DMap.emplace(rtTex2D.GetInstanceID(), std::move(data));

	rtTex2D.SetClean();
	rtTex2D.destroyed.Connect<&GPURsrcMngrDX12::UnregisterRenderTargetTexture2D>(this);
	rtTex2D.destroyed.Connect<&GPURsrcMngrDX12::UnregisterTexture2D>(this);

	return *this;
}

GPURsrcMngrDX12& GPURsrcMngrDX12::UnregisterRenderTargetTexture2D(std::size_t ID) {
	static std::mutex m;
	std::lock_guard guard{ m };
	if (auto target = pImpl->rtTexture2DMap.find(ID); target != pImpl->rtTexture2DMap.end()) {
		auto& tex = target->second;
		pImpl->deleteBatch.Add(std::move(tex.resource));
		pImpl->deleteBatch.AddCallback(
			[allocSRV = std::make_shared<UDX12::DescriptorHeapAllocation>(std::move(tex.allocationSRV)),
			allocRTV = std::make_shared<UDX12::DescriptorHeapAllocation>(std::move(tex.allocationRTV))]()
		{
			UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(*allocSRV));
			UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Free(std::move(*allocRTV));
		});
		pImpl->rtTexture2DMap.erase(target);
	}
	return *this;
}

D3D12_CPU_DESCRIPTOR_HANDLE GPURsrcMngrDX12::GetTexture2DSrvCpuHandle(const Texture2D& tex2D) const {
	// render target [first]
	if (auto target = pImpl->rtTexture2DMap.find(tex2D.GetInstanceID()); target != pImpl->rtTexture2DMap.end())
		return target->second.allocationSRV.GetCpuHandle();

	return pImpl->texture2DMap.at(tex2D.GetInstanceID()).allocationSRV.GetCpuHandle(0);
}
D3D12_GPU_DESCRIPTOR_HANDLE GPURsrcMngrDX12::GetTexture2DSrvGpuHandle(const Texture2D& tex2D) const {
	// render target [first]
	if (auto target = pImpl->rtTexture2DMap.find(tex2D.GetInstanceID()); target != pImpl->rtTexture2DMap.end())
		return target->second.allocationSRV.GetGpuHandle();
	return pImpl->texture2DMap.at(tex2D.GetInstanceID()).allocationSRV.GetGpuHandle(0);
}
ID3D12Resource* GPURsrcMngrDX12::GetTexture2DResource(const Texture2D& tex2D) const {
	// render target [first]
	if (auto target = pImpl->rtTexture2DMap.find(tex2D.GetInstanceID()); target != pImpl->rtTexture2DMap.end())
		return target->second.resource.Get();
	return pImpl->texture2DMap.at(tex2D.GetInstanceID()).resource.Get();
}
D3D12_CPU_DESCRIPTOR_HANDLE GPURsrcMngrDX12::GetTextureCubeSrvCpuHandle(const TextureCube& texCube) const {
	return pImpl->textureCubeMap.at(texCube.GetInstanceID()).allocationSRV.GetCpuHandle(0);
}
D3D12_GPU_DESCRIPTOR_HANDLE GPURsrcMngrDX12::GetTextureCubeSrvGpuHandle(const TextureCube& texCube) const {
	return pImpl->textureCubeMap.at(texCube.GetInstanceID()).allocationSRV.GetGpuHandle(0);
}
ID3D12Resource* GPURsrcMngrDX12::GetTextureCubeResource(const TextureCube& texCube) const {
	return pImpl->textureCubeMap.at(texCube.GetInstanceID()).resource.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE GPURsrcMngrDX12::GetRenderTargetTexture2DSrvCpuHandle(const RenderTargetTexture2D& rtTex2D) const {
	return pImpl->rtTexture2DMap.at(rtTex2D.GetInstanceID()).allocationSRV.GetCpuHandle();
}
D3D12_GPU_DESCRIPTOR_HANDLE GPURsrcMngrDX12::GetRenderTargetTexture2DSrvGpuHandle(const RenderTargetTexture2D& rtTex2D) const {
	return pImpl->rtTexture2DMap.at(rtTex2D.GetInstanceID()).allocationSRV.GetGpuHandle();
}
D3D12_CPU_DESCRIPTOR_HANDLE GPURsrcMngrDX12::GetRenderTargetTexture2DRtvCpuHandle(const RenderTargetTexture2D& rtTex2D) const {
	return pImpl->rtTexture2DMap.at(rtTex2D.GetInstanceID()).allocationRTV.GetCpuHandle();
}
ID3D12Resource* GPURsrcMngrDX12::GetRenderTargetTexture2DResource(const RenderTargetTexture2D& rtTex2D) const {
	return pImpl->rtTexture2DMap.at(rtTex2D.GetInstanceID()).resource.Get();
}

UDX12::MeshGPUBuffer& GPURsrcMngrDX12::RegisterMesh(ID3D12GraphicsCommandList* cmdList, Mesh& mesh) {
	bool needCleanVB = false;
	auto target = pImpl->meshMap.find(mesh.GetInstanceID());
	if (target == pImpl->meshMap.end()) {
		mesh.destroyed.Connect<&GPURsrcMngrDX12::UnregisterMesh>(this);
		if (mesh.IsEditable()) {
			auto [iter, success] = pImpl->meshMap.try_emplace(
				mesh.GetInstanceID(),
				pImpl->device, cmdList,
				mesh.GetVertexBufferData(),
				(UINT)mesh.GetVertexBufferVertexCount(),
				(UINT)mesh.GetVertexBufferVertexStride(),
				mesh.GetIndices().data(),
				(UINT)mesh.GetIndices().size(),
				DXGI_FORMAT_R32_UINT
			);
			assert(success);
			target = iter;
		}
		else {
			pImpl->hasUpload = true;
			auto [iter, success] = pImpl->meshMap.try_emplace(
				mesh.GetInstanceID(),
				pImpl->device, *pImpl->upload,
				mesh.GetVertexBufferData(),
				(UINT)mesh.GetVertexBufferVertexCount(),
				(UINT)mesh.GetVertexBufferVertexStride(),
				mesh.GetIndices().data(),
				(UINT)mesh.GetIndices().size(),
				DXGI_FORMAT_R32_UINT
			);
			assert(success);
			//mesh.ClearVertexBuffer();
			needCleanVB = true;
			target = iter;
		}
	}
	else {
		auto& meshGpuBuffer = target->second;
		if (meshGpuBuffer.IsStatic()) {
			if (mesh.IsEditable()) {
				meshGpuBuffer.ConvertToDynamic(pImpl->device);
				if (mesh.IsDirty()) {
					meshGpuBuffer.Update(
						pImpl->device, cmdList,
						mesh.GetVertexBufferData(),
						(UINT)mesh.GetVertexBufferVertexCount(),
						(UINT)mesh.GetVertexBufferVertexStride(),
						mesh.GetIndices().data(),
						(UINT)mesh.GetIndices().size(),
						DXGI_FORMAT_R32_UINT
					);
				}
			}
			else { // non-editable
				if (mesh.IsDirty()) {
					meshGpuBuffer.ConvertToDynamic(pImpl->device);
					meshGpuBuffer.Update(
						pImpl->device, cmdList,
						mesh.GetVertexBufferData(),
						(UINT)mesh.GetVertexBufferVertexCount(),
						(UINT)mesh.GetVertexBufferVertexStride(),
						mesh.GetIndices().data(),
						(UINT)mesh.GetIndices().size(),
						DXGI_FORMAT_R32_UINT
					);
					meshGpuBuffer.ConvertToStatic(pImpl->deleteBatch);
				}
				//mesh.ClearVertexBuffer();
				needCleanVB = true;
			}
		}
		else { // dynamic
			if (mesh.IsEditable()) {
				if (mesh.IsDirty()) {
					meshGpuBuffer.Update(
						pImpl->device, cmdList,
						mesh.GetVertexBufferData(),
						(UINT)mesh.GetVertexBufferVertexCount(),
						(UINT)mesh.GetVertexBufferVertexStride(),
						mesh.GetIndices().data(),
						(UINT)mesh.GetIndices().size(),
						DXGI_FORMAT_R32_UINT
					);
				}
			}
			else {
				if (mesh.IsDirty()) {
					meshGpuBuffer.Update(
						pImpl->device, cmdList,
						mesh.GetVertexBufferData(),
						(UINT)mesh.GetVertexBufferVertexCount(),
						(UINT)mesh.GetVertexBufferVertexStride(),
						mesh.GetIndices().data(),
						(UINT)mesh.GetIndices().size(),
						DXGI_FORMAT_R32_UINT
					);
				}
				//mesh.ClearVertexBuffer();
				needCleanVB = true;
				meshGpuBuffer.ConvertToStatic(pImpl->deleteBatch);
			}
		}
	}

	// uv, normal, tangent
	if (pImpl->enable_DXR
		&& (MeshLayoutMngr::Instance().GetMeshLayoutID(1, 1, 1, 1) == MeshLayoutMngr::Instance().GetMeshLayoutID(mesh)
			|| MeshLayoutMngr::Instance().GetMeshLayoutID(1, 1, 1, 0) == MeshLayoutMngr::Instance().GetMeshLayoutID(mesh)))
	{
		auto& meshGpuBuffer = target->second;

		if (mesh.IsDirty())
			pImpl->dxrMeshDataMap.erase(mesh.GetInstanceID());

		auto dxrMeshDataTarget = pImpl->dxrMeshDataMap.find(mesh.GetInstanceID());
		if (dxrMeshDataTarget == pImpl->dxrMeshDataMap.end()) {
			std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geomDescs;
			for (const auto& submesh : mesh.GetSubMeshes()) {
				assert(submesh.topology == MeshTopology::Triangles && submesh.baseVertex == 0);

				D3D12_RAYTRACING_GEOMETRY_DESC geomDesc;
				geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
				geomDesc.Triangles.VertexBuffer.StartAddress = meshGpuBuffer.VertexBufferView().BufferLocation;
				geomDesc.Triangles.VertexBuffer.StrideInBytes = meshGpuBuffer.VertexBufferView().StrideInBytes;
				geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
				geomDesc.Triangles.VertexCount = meshGpuBuffer.VertexBufferView().SizeInBytes / meshGpuBuffer.VertexBufferView().StrideInBytes;
				
				geomDesc.Triangles.IndexBuffer = meshGpuBuffer.IndexBufferView().BufferLocation + sizeof(std::uint32_t) * submesh.indexStart;
				geomDesc.Triangles.IndexCount = (UINT)submesh.indexCount;
				geomDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;

				geomDesc.Triangles.Transform3x4 = 0; // identity

				geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

				geomDescs.push_back(geomDesc);
			}

			if (!geomDescs.empty()) {
				// Get the size requirements for the scratch and AS buffers
				D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
				inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
				inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
				inputs.NumDescs = (UINT)geomDescs.size();
				inputs.pGeometryDescs = geomDescs.data();
				inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

				D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
				Microsoft::WRL::ComPtr<ID3D12Device5> device5;
				Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> gcmdlist4;
				ThrowIfFailed(pImpl->device->QueryInterface(IID_PPV_ARGS(&device5)));
				ThrowIfFailed(cmdList->QueryInterface(IID_PPV_ARGS(&gcmdlist4)));
				device5->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);
				// Create the buffers. They need to support UAV, and since we are going to immediately use them, we create them with an unordered-access state
				const auto default_heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

				Microsoft::WRL::ComPtr<ID3D12Resource> result_rsrc;
				{ // create result_rsrc
					auto desc = CD3DX12_RESOURCE_DESC::Buffer(info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
					pImpl->device->CreateCommittedResource(
						&default_heap_properties,
						D3D12_HEAP_FLAG_NONE,
						&desc,
						D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
						nullptr,
						IID_PPV_ARGS(&result_rsrc));
				}

				Microsoft::WRL::ComPtr<ID3D12Resource> scratch_rsrc;
				{ // create scratch_rsrc
					auto desc = CD3DX12_RESOURCE_DESC::Buffer(info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
					pImpl->device->CreateCommittedResource(
						&default_heap_properties,
						D3D12_HEAP_FLAG_NONE,
						&desc,
						D3D12_RESOURCE_STATE_COMMON,
						nullptr,
						IID_PPV_ARGS(&scratch_rsrc));
				}

				// Create the bottom-level AS
				D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
				asDesc.Inputs = inputs;
				asDesc.DestAccelerationStructureData = result_rsrc->GetGPUVirtualAddress();
				asDesc.ScratchAccelerationStructureData = scratch_rsrc->GetGPUVirtualAddress();

				gcmdlist4->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

				// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
				D3D12_RESOURCE_BARRIER uavBarrier = {};
				uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
				uavBarrier.UAV.pResource = result_rsrc.Get();
				cmdList->ResourceBarrier(1, &uavBarrier);

				// result_rsrc to dxrMeshDataMap
				Impl::DXRMeshData dxrMeshData;
				dxrMeshData.blas = result_rsrc;
				dxrMeshData.allocation = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(2 * geomDescs.size());
				for (std::size_t i = 0; i < geomDescs.size(); i++) {
					{ // vertex buffer
						D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc = {}; // structure buffer
						srvdesc.Format = DXGI_FORMAT_UNKNOWN;
						srvdesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
						srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
						srvdesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
						srvdesc.Buffer.FirstElement = (geomDescs[i].Triangles.VertexBuffer.StartAddress - meshGpuBuffer.VertexBufferView().BufferLocation)
							/ geomDescs[i].Triangles.VertexBuffer.StrideInBytes;
						srvdesc.Buffer.NumElements = geomDescs[i].Triangles.VertexCount;
						srvdesc.Buffer.StructureByteStride = meshGpuBuffer.VertexBufferView().StrideInBytes; // pos(3) + uv(2) + normal(3) + tangent(3) [+ color(3)]
						pImpl->device->CreateShaderResourceView(meshGpuBuffer.GetVertexBufferResource().Get(), &srvdesc,
							dxrMeshData.allocation.GetCpuHandle(2 * i));
					}

					{ // index buffer
						D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc = {}; // texture 1d
						srvdesc.Format = DXGI_FORMAT_UNKNOWN;
						srvdesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
						srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
						srvdesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
						srvdesc.Buffer.FirstElement = (geomDescs[i].Triangles.IndexBuffer - meshGpuBuffer.IndexBufferView().BufferLocation)
							/ sizeof(std::uint32_t);
						srvdesc.Buffer.NumElements = geomDescs[i].Triangles.IndexCount;
						srvdesc.Buffer.StructureByteStride = sizeof(std::uint32_t);
						pImpl->device->CreateShaderResourceView(meshGpuBuffer.GetIndexBufferResource().Get(), &srvdesc,
							dxrMeshData.allocation.GetCpuHandle(2 * i + 1));
					}
				}
				pImpl->dxrMeshDataMap.emplace(mesh.GetInstanceID(), std::move(dxrMeshData));

				// clear scratch
				pImpl->deleteBatch.Add(std::move(scratch_rsrc));
			}
		}
	}

	if (mesh.IsDirty())
		mesh.SetClean();
	if (needCleanVB)
		mesh.ClearVertexBuffer();
	return target->second;
}

GPURsrcMngrDX12& GPURsrcMngrDX12::UnregisterMesh(std::size_t ID) {
	static std::mutex m;
	std::lock_guard guard{ m };
	if (auto target = pImpl->meshMap.find(ID); target != pImpl->meshMap.end()) {
		target->second.Delete(pImpl->deleteBatch);
		pImpl->meshMap.erase(target);
		if (pImpl->enable_DXR) {
			auto dxrMeshDataTarget = pImpl->dxrMeshDataMap.find(ID);
			if (dxrMeshDataTarget != pImpl->dxrMeshDataMap.end()) {
				pImpl->deleteBatch.Add(dxrMeshDataTarget->second.blas);
				pImpl->dxrMeshDataMap.erase(dxrMeshDataTarget);
			}
		}
	}
	return *this;
}

UDX12::MeshGPUBuffer& GPURsrcMngrDX12::GetMeshGPUBuffer(const Mesh& mesh) const {
	return pImpl->meshMap.at(mesh.GetInstanceID());
}

ID3D12Resource* GPURsrcMngrDX12::GetMeshBLAS(const Mesh& mesh) const {
	assert(pImpl->enable_DXR);
	auto target = pImpl->dxrMeshDataMap.find(mesh.GetInstanceID());
	if (target == pImpl->dxrMeshDataMap.end())
		return nullptr;
	return target->second.blas.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE GPURsrcMngrDX12::GetMeshBufferTableCpuHandle(const Mesh& mesh, size_t geometryIdx) const {
	assert(pImpl->enable_DXR);
	auto target = pImpl->dxrMeshDataMap.find(mesh.GetInstanceID());
	if (target == pImpl->dxrMeshDataMap.end())
		return D3D12_CPU_DESCRIPTOR_HANDLE{};
	return target->second.allocation.GetCpuHandle(2 * geometryIdx);
}
D3D12_GPU_DESCRIPTOR_HANDLE GPURsrcMngrDX12::GetMeshBufferTableGpuHandle(const Mesh& mesh, size_t geometryIdx) const {
	assert(pImpl->enable_DXR);
	auto target = pImpl->dxrMeshDataMap.find(mesh.GetInstanceID());
	if (target == pImpl->dxrMeshDataMap.end())
		return D3D12_GPU_DESCRIPTOR_HANDLE{};
	return target->second.allocation.GetGpuHandle(2 * geometryIdx);
}

bool GPURsrcMngrDX12::RegisterShader(Shader& shader) {
	std::unique_lock<std::mutex> lk{ pImpl->mutex_shaderMap };

	if (shader.IsDirty()) {
		lk.unlock();
		UnregisterShader(shader.GetInstanceID());
		lk.lock();
	}
	else {
		auto target = pImpl->shaderMap.find(shader.GetInstanceID());
		if (target != pImpl->shaderMap.end())
			return true;
	}

	Ubpa::UDX12::D3DInclude d3dInclude{ shader.hlslFile->GetLocalDir(), "../" };
	Impl::ShaderCompileData shaderCompileData;
	shaderCompileData.passes.resize(shader.passes.size());
	for (size_t i = 0; i < shader.passes.size(); i++) {
		const auto& pass = shader.passes[i];
		Impl::ShaderCompileData::PassData passdata;
		std::vector<D3D_SHADER_MACRO> marcos;
		for (const auto& [name, def] : pass.macros)
			marcos.push_back(D3D_SHADER_MACRO{ name.c_str(), def.c_str() });
		marcos.push_back(D3D_SHADER_MACRO{ nullptr,nullptr });

		shaderCompileData.passes[i].vsByteCode = UDX12::Util::CompileShader(
			shader.hlslFile->GetText(),
			marcos.data(),
			pass.vertexName,
			"vs_5_0",
			&d3dInclude,
			shader.name.data()
		);
		if (!shaderCompileData.passes[i].vsByteCode)
			return false;
		shaderCompileData.passes[i].psByteCode = UDX12::Util::CompileShader(
			shader.hlslFile->GetText(),
			marcos.data(),
			pass.fragmentName,
			"ps_5_0",
			&d3dInclude,
			shader.name.data()
		);
		if (!shaderCompileData.passes[i].psByteCode)
			pImpl->shaderMap.erase(shader.GetInstanceID());
		ThrowIfFailed(D3DReflect(
			shaderCompileData.passes[i].vsByteCode->GetBufferPointer(),
			shaderCompileData.passes[i].vsByteCode->GetBufferSize(),
			IID_PPV_ARGS(&shaderCompileData.passes[i].vsRefl)
		));
		ThrowIfFailed(D3DReflect(
			shaderCompileData.passes[i].psByteCode->GetBufferPointer(),
			shaderCompileData.passes[i].psByteCode->GetBufferSize(),
			IID_PPV_ARGS(&shaderCompileData.passes[i].psRefl)
		));
	}

	auto RangeTypeMap = [](RootDescriptorType type) {
		switch (type)
		{
		case Ubpa::Utopia::RootDescriptorType::CBV:
			return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		case Ubpa::Utopia::RootDescriptorType::SRV:
			return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		case Ubpa::Utopia::RootDescriptorType::UAV:
			return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		default:
			assert(false);
			return D3D12_DESCRIPTOR_RANGE_TYPE{};
		}
	};

	size_t N = shader.rootParameters.size();
	if (N == 0)
		return true;
	std::vector<CD3DX12_ROOT_PARAMETER> rootParamters(N);
	std::vector<std::vector<CD3DX12_DESCRIPTOR_RANGE>> rangesVec;
	for (size_t i = 0; i < N; i++) {
		std::visit([&](const auto& parameter) {
			using Type = std::decay_t<decltype(parameter)>;
			if constexpr (std::is_same_v<Type, RootDescriptorTable>) {
				const RootDescriptorTable& table = parameter;
				std::vector<CD3DX12_DESCRIPTOR_RANGE> ranges(table.size());
				for (size_t i = 0; i < table.size(); i++) {
					ranges[i].Init(
						RangeTypeMap(table[i].RangeType),
						table[i].NumDescriptors,
						table[i].BaseShaderRegister,
						table[i].RegisterSpace
					);
				}
				rootParamters[i].InitAsDescriptorTable((UINT)ranges.size(), ranges.data());
				rangesVec.emplace_back(std::move(ranges));
			}
			else if constexpr (std::is_same_v<Type, RootConstants>) {
				assert(false);
			}
			else if constexpr (std::is_same_v<Type, RootDescriptor>) {
				const RootDescriptor& descriptor = parameter;
				switch (descriptor.DescriptorType)
				{
				case Ubpa::Utopia::RootDescriptorType::CBV:
					rootParamters[i].InitAsConstantBufferView(descriptor.ShaderRegister, descriptor.RegisterSpace);
					break;
				case Ubpa::Utopia::RootDescriptorType::SRV:
					rootParamters[i].InitAsShaderResourceView(descriptor.ShaderRegister, descriptor.RegisterSpace);
					break;
				case Ubpa::Utopia::RootDescriptorType::UAV:
					rootParamters[i].InitAsUnorderedAccessView(descriptor.ShaderRegister, descriptor.RegisterSpace);
					break;
				default:
					assert(false);
					break;
				}
			}
			else
				static_assert(always_false<void>);
		}, shader.rootParameters[i]);
	}

	const auto samplers = GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		(UINT)rootParamters.size(), rootParamters.data(),
		(UINT)samplers.size(), samplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	shaderCompileData.rootSignature = UDX12::Device{ Microsoft::WRL::ComPtr<ID3D12Device>(pImpl->device) }
		.CreateRootSignature(rootSigDesc);

	if (!shaderCompileData.rootSignature)
		return false;

	pImpl->shaderMap.emplace(shader.GetInstanceID(), std::move(shaderCompileData));

	shader.SetClean();
	shader.destroyed.Connect<&GPURsrcMngrDX12::UnregisterShader>(this);

	return true;
}

GPURsrcMngrDX12& GPURsrcMngrDX12::UnregisterShader(std::size_t ID) {
	std::lock_guard guard{ pImpl->mutex_shaderMap };
	if (auto target = pImpl->shaderMap.find(ID); target != pImpl->shaderMap.end()) {
		pImpl->deleteBatch.AddCallback([data = std::move(target->second)]() {});
		pImpl->shaderMap.erase(target);
	}
	return *this;
}

const ID3DBlob* GPURsrcMngrDX12::GetShaderByteCode_vs(const Shader& shader, size_t passIdx) const {
	std::lock_guard guard{ pImpl->mutex_shaderMap };
	return pImpl->shaderMap.at(shader.GetInstanceID()).passes[passIdx].vsByteCode.Get();
}

const ID3DBlob* GPURsrcMngrDX12::GetShaderByteCode_ps(const Shader& shader, size_t passIdx) const {
	std::lock_guard guard{ pImpl->mutex_shaderMap };
	return pImpl->shaderMap.at(shader.GetInstanceID()).passes[passIdx].psByteCode.Get();
}

ID3D12ShaderReflection* GPURsrcMngrDX12::GetShaderRefl_vs(const Shader& shader, size_t passIdx) const {
	std::lock_guard guard{ pImpl->mutex_shaderMap };
	return pImpl->shaderMap.at(shader.GetInstanceID()).passes[passIdx].vsRefl.Get();
}

ID3D12ShaderReflection* GPURsrcMngrDX12::GetShaderRefl_ps(const Shader& shader, size_t passIdx) const {
	std::lock_guard guard{ pImpl->mutex_shaderMap };
	return pImpl->shaderMap.at(shader.GetInstanceID()).passes[passIdx].psRefl.Get();
}

ID3D12RootSignature* GPURsrcMngrDX12::GetShaderRootSignature(const Shader& shader) const {
	std::lock_guard guard{ pImpl->mutex_shaderMap };
	return pImpl->shaderMap.at(shader.GetInstanceID()).rootSignature.Get();
}

ID3D12PipelineState* GPURsrcMngrDX12::GetOrCreateShaderPSO(
	const Shader& shader,
	size_t passIdx,
	size_t layoutID,
	size_t rtNum,
	DXGI_FORMAT rtFormat,
	DXGI_FORMAT dsvFormat)
{
	std::lock_guard guard{ pImpl->mutex_shaderMap };

	auto starget = pImpl->shaderMap.find(shader.GetInstanceID());
	if (starget == pImpl->shaderMap.end())
		return nullptr;
	auto& shaderdata = starget->second;
	auto& passdata = shaderdata.passes[passIdx];

	Impl::ShaderCompileData::PassData::PSOKey key;
	key.layoutID = layoutID;
	key.rtNum = rtNum;
	key.rtFormat = rtFormat;
	auto psotarget = passdata.PSOs.find(key);
	if (psotarget == passdata.PSOs.end()) {
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
		const D3D12_INPUT_ELEMENT_DESC* layout_ptr;
		UINT layout_cnt;
		if (layoutID == static_cast<std::size_t>(-1)) {
			layout_ptr = nullptr;
			layout_cnt = 0;
		}
		else {
			const auto& layout = MeshLayoutMngr::Instance().GetMeshLayoutValue(layoutID);
			layout_ptr = layout.data();
			layout_cnt = (UINT)layout.size();
		}
		auto desc = UDX12::Desc::PSO::MRT(
			shaderdata.rootSignature.Get(),
			layout_ptr, layout_cnt,
			passdata.vsByteCode.Get(),
			passdata.psByteCode.Get(),
			(UINT)rtNum,
			rtFormat,
			dsvFormat
		);
		desc.RasterizerState.FrontCounterClockwise = TRUE;
		const auto& renderState = shader.passes[passIdx].renderState;
		desc.RasterizerState.FillMode = static_cast<D3D12_FILL_MODE>(renderState.fillMode);
		desc.RasterizerState.CullMode = static_cast<D3D12_CULL_MODE>(renderState.cullMode);
		if (dsvFormat == DXGI_FORMAT_UNKNOWN) {
			desc.DepthStencilState.DepthEnable = false;
			desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_ALWAYS;
			desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ZERO;
		}
		else {
			desc.DepthStencilState.DepthEnable = true;
			desc.DepthStencilState.DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(renderState.zTest);
			desc.DepthStencilState.DepthWriteMask = static_cast<D3D12_DEPTH_WRITE_MASK>(renderState.zWrite);
		}

		for (size_t i = 0; i < 8; i++) {
			if (renderState.blendStates[i].enable) {
				desc.BlendState.RenderTarget[i].BlendEnable = TRUE;
				desc.BlendState.RenderTarget[i].SrcBlend = static_cast<D3D12_BLEND>(renderState.blendStates[i].src);
				desc.BlendState.RenderTarget[i].DestBlend = static_cast<D3D12_BLEND>(renderState.blendStates[i].dest);
				desc.BlendState.RenderTarget[i].BlendOp = static_cast<D3D12_BLEND_OP>(renderState.blendStates[i].op);
				desc.BlendState.RenderTarget[i].SrcBlendAlpha = static_cast<D3D12_BLEND>(renderState.blendStates[i].srcAlpha);
				desc.BlendState.RenderTarget[i].DestBlendAlpha = static_cast<D3D12_BLEND>(renderState.blendStates[i].destAlpha);
				desc.BlendState.RenderTarget[i].BlendOpAlpha = static_cast<D3D12_BLEND_OP>(renderState.blendStates[i].opAlpha);
			}
		}

		if (renderState.stencilState.enable) {
			desc.DepthStencilState.StencilEnable = TRUE;
			desc.DepthStencilState.StencilReadMask = renderState.stencilState.readMask;
			desc.DepthStencilState.StencilWriteMask = renderState.stencilState.writeMask;
			D3D12_DEPTH_STENCILOP_DESC stencilDesc;
			stencilDesc.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(renderState.stencilState.depthFailOp);
			stencilDesc.StencilFailOp = static_cast<D3D12_STENCIL_OP>(renderState.stencilState.failOp);
			stencilDesc.StencilPassOp = static_cast<D3D12_STENCIL_OP>(renderState.stencilState.passOp);
			stencilDesc.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(renderState.stencilState.func);
			desc.DepthStencilState.FrontFace = stencilDesc;
			desc.DepthStencilState.BackFace = stencilDesc;
		}
		for (size_t i = 0; i < 8; i++)
			desc.BlendState.RenderTarget[i].RenderTargetWriteMask = renderState.colorMask[i];

		pImpl->device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));
		psotarget = passdata.PSOs.emplace(key, std::move(pso)).first;
	}
	return psotarget->second.Get();
}

std::array<CD3DX12_STATIC_SAMPLER_DESC, 6> GPURsrcMngrDX12::GetStaticSamplers() const {
	return {
		pImpl->pointWrap, pImpl->pointClamp,
		pImpl->linearWrap, pImpl->linearClamp,
		pImpl->anisotropicWrap, pImpl->anisotropicClamp
	};
}
