#include <Utopia/Render/DX12/RsrcMngrDX12.h>

#include <Utopia/Render/Texture2D.h>
#include <Utopia/Render/TextureCube.h>
#include <Utopia/Core/Image.h>
#include <Utopia/Render/HLSLFile.h>
#include <Utopia/Render/Shader.h>
#include <Utopia/Render/Mesh.h>

#include <unordered_map>
#include <iostream>

using namespace Ubpa::Utopia;
using namespace Ubpa;
using namespace std;

struct RsrcMngrDX12::Impl {
	struct Texture2DGPUData {
		ID3D12Resource* resource;
		UDX12::DescriptorHeapAllocation allocationSRV;
	};
	struct TextureCubeGPUData {
		ID3D12Resource* resource;
		UDX12::DescriptorHeapAllocation allocationSRV;
	};
	struct RenderTargetGPUData {
		vector<ID3D12Resource*> resources;
		UDX12::DescriptorHeapAllocation allocationSRV;
		UDX12::DescriptorHeapAllocation allocationRTV;
	};
	struct ShaderCompileData {
		struct PassData {
			Microsoft::WRL::ComPtr<ID3DBlob> vsByteCode;
			Microsoft::WRL::ComPtr<ID3DBlob> psByteCode;
			Microsoft::WRL::ComPtr<ID3D12ShaderReflection> vsRefl;
			Microsoft::WRL::ComPtr<ID3D12ShaderReflection> psRefl;
		};
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
		std::vector<PassData> passes;
	};

	bool isInit{ false };
	ID3D12Device* device{ nullptr };
	DirectX::ResourceUploadBatch* upload{ nullptr };
	bool hasUpload = false;
	UDX12::ResourceDeleteBatch deleteBatch;

	unordered_map<size_t, Texture2DGPUData> texture2DMap;
	unordered_map<size_t, TextureCubeGPUData> textureCubeMap;
	unordered_map<size_t, RenderTargetGPUData> renderTargetMap;
	unordered_map<size_t, ShaderCompileData> shaderMap;

	unordered_map<size_t, UDX12::MeshGPUBuffer> meshMap;
	vector<ID3D12PipelineState*> PSOs;

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

RsrcMngrDX12::RsrcMngrDX12()
	: pImpl(new Impl)
{
}

RsrcMngrDX12::~RsrcMngrDX12() {
	assert(!pImpl->isInit);
	delete(pImpl);
}

RsrcMngrDX12& RsrcMngrDX12::Init(ID3D12Device* device) {
	assert(!pImpl->isInit);

	pImpl->device = device;
	pImpl->upload = new DirectX::ResourceUploadBatch{ device };
	pImpl->upload->Begin();

	pImpl->isInit = true;
	return *this;
}

void RsrcMngrDX12::Clear(ID3D12CommandQueue* cmdQueue) {
	assert(pImpl->isInit);

	pImpl->upload->End(cmdQueue);
	pImpl->deleteBatch.Commit(pImpl->device, cmdQueue);

	for (auto& [name, tex] : pImpl->texture2DMap) {
		UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(move(tex.allocationSRV));
		tex.resource->Release();
	}

	for (auto& [name, tex] : pImpl->textureCubeMap) {
		UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(move(tex.allocationSRV));
		tex.resource->Release();
	}

	for (auto& [name, tex] : pImpl->renderTargetMap) {
		UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(move(tex.allocationSRV));
		UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Free(move(tex.allocationRTV));
		for (auto rsrc : tex.resources)
			rsrc->Release();
	}

	for (auto PSO : pImpl->PSOs)
		PSO->Release();

	pImpl->device = nullptr;
	delete pImpl->upload;

	pImpl->texture2DMap.clear();
	pImpl->textureCubeMap.clear();
	pImpl->renderTargetMap.clear();
	pImpl->meshMap.clear();
	pImpl->PSOs.clear();
	pImpl->shaderMap.clear();

	pImpl->isInit = false;
}

void RsrcMngrDX12::CommitUploadAndDelete(ID3D12CommandQueue* cmdQueue) {
	if (pImpl->hasUpload) {
		pImpl->upload->End(cmdQueue);
		pImpl->upload->Begin();
	}

	pImpl->deleteBatch.Commit(pImpl->device, cmdQueue);
}

//RsrcMngrDX12& RsrcMngrDX12::RegisterTexture2D(
//	DirectX::ResourceUploadBatch& upload,
//	size_t id,
//	wstring_view filename)
//{
//	return RegisterDDSTextureArrayFromFile(upload, id, &filename, 1);
//}
//
//RsrcMngrDX12& RsrcMngrDX12::RegisterDDSTextureArrayFromFile(DirectX::ResourceUploadBatch& upload,
//	size_t id, const wstring_view* filenameArr, UINT num)
//{
//	Impl::Texture tex;
//	tex.resources.resize(num);
//
//	tex.allocationSRV = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(num);
//
//	for (UINT i = 0; i < num; i++) {
//		bool isCubeMap;
//		DirectX::CreateDDSTextureFromFile(
//			pImpl->device,
//			upload,
//			filenameArr[i].data(),
//			&tex.resources[i],
//			false,
//			0,
//			nullptr,
//			&isCubeMap);
//
//		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc =
//			isCubeMap ?
//			UDX12::Desc::SRV::TexCube(tex.resources[i]->GetDesc().Format)
//			: UDX12::Desc::SRV::Tex2D(tex.resources[i]->GetDesc().Format);
//
//		pImpl->device->CreateShaderResourceView(tex.resources[i], &srvDesc, tex.allocationSRV.GetCpuHandle(i));
//	}
//
//	pImpl->textureMap.emplace(id, move(tex));
//
//	return *this;
//}

RsrcMngrDX12& RsrcMngrDX12::RegisterTexture2D(const Texture2D& tex2D) {
	auto target = pImpl->texture2DMap.find(tex2D.GetInstanceID());
	if (target != pImpl->texture2DMap.end())
		return *this;

	Impl::Texture2DGPUData tex;

	tex.allocationSRV = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(static_cast<uint32_t>(1));

	constexpr DXGI_FORMAT channelMap[] = {
		DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
		DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
	};

	D3D12_SUBRESOURCE_DATA data;
	data.pData = tex2D.image->data;
	data.RowPitch = tex2D.image->width * tex2D.image->channel * sizeof(float);
	data.SlicePitch = tex2D.image->height* data.RowPitch; // this field is useless for texture 2d

	pImpl->hasUpload = true;
	DirectX::CreateTextureFromMemory(
		pImpl->device,
		*pImpl->upload,
		tex2D.image->width.get(),
		tex2D.image->height.get(),
		channelMap[tex2D.image->channel - 1],
		data,
		&tex.resource
	);

	const auto tex2DSRVDesc = UDX12::Desc::SRV::Tex2D(tex.resource->GetDesc().Format);
	pImpl->device->CreateShaderResourceView(
		tex.resource,
		&tex2DSRVDesc,
		tex.allocationSRV.GetCpuHandle(static_cast<uint32_t>(0))
	);

	pImpl->texture2DMap.emplace_hint(target, std::make_pair(tex2D.GetInstanceID(), move(tex)));

	return *this;
}

RsrcMngrDX12& RsrcMngrDX12::UnregisterTexture2D(const Texture2D& tex2D) {
	if (auto target = pImpl->texture2DMap.find(tex2D.GetInstanceID()); target != pImpl->texture2DMap.end()) {
		auto& tex = target->second;
		UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(move(tex.allocationSRV));
		pImpl->deleteBatch.Add(tex.resource);
		tex.resource = nullptr;
		pImpl->texture2DMap.erase(target);
	}
	return *this;
}

RsrcMngrDX12& RsrcMngrDX12::RegisterTextureCube(const TextureCube& texcube) {
	auto target = pImpl->textureCubeMap.find(texcube.GetInstanceID());
	if (target != pImpl->textureCubeMap.end())
		return *this;

	Impl::TextureCubeGPUData tex;

	tex.allocationSRV = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(static_cast<uint32_t>(1));

	constexpr DXGI_FORMAT channelMap[] = {
		DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
		DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
	};

	size_t w = texcube.images->front()->width;
	size_t h = texcube.images->front()->height;
	size_t c = texcube.images->front()->channel;

	std::array<D3D12_SUBRESOURCE_DATA, 6> datas;
	for (size_t i = 0; i < datas.size(); i++) {
		datas[i].pData = texcube.images[i]->data;
		datas[i].RowPitch = texcube.images[i]->width * texcube.images[i]->channel * sizeof(float);
		datas[i].SlicePitch = texcube.images[i]->height * datas[i].RowPitch; // this field is useless for texture 2d
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
		tex.resource,
		&texCubeSRVDesc,
		tex.allocationSRV.GetCpuHandle(static_cast<uint32_t>(0))
	);

	pImpl->textureCubeMap.emplace_hint(target, std::make_pair(texcube.GetInstanceID(), move(tex)));

	return *this;
}

RsrcMngrDX12& RsrcMngrDX12::UnregisterTextureCube(const TextureCube& texcube) {
	if (auto target = pImpl->textureCubeMap.find(texcube.GetInstanceID()); target != pImpl->textureCubeMap.end()) {
		auto& tex = target->second;
		UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(move(tex.allocationSRV));
		pImpl->deleteBatch.Add(tex.resource);
		tex.resource = nullptr;
		pImpl->textureCubeMap.erase(target);
	}
	return *this;
}

//RsrcMngrDX12& RsrcMngrDX12::RegisterTexture2DArray(
//	DirectX::ResourceUploadBatch& upload,
//	size_t id, const Texture2D&* tex2Ds, size_t num)
//{
//	Impl::Texture tex;
//	tex.resources.resize(num);
//
//	tex.allocationSRV = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(static_cast<uint32_t>(num));
//
//	constexpr DXGI_FORMAT channelMap[] = {
//		DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT,
//		DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,
//		DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
//		DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT,
//	};
//
//	for (size_t i = 0; i < num; i++) {
//		D3D12_SUBRESOURCE_DATA data;
//		data.pData = tex2Ds[i]->image->data;
//		data.RowPitch = tex2Ds[i]->image->width * tex2Ds[i]->image->channel * sizeof(float);
//		data.SlicePitch = tex2Ds[i]->image->height * data.RowPitch;
//		
//		DirectX::CreateTextureFromMemory(
//			pImpl->device,
//			upload,
//			tex2Ds[i]->image->width.get(),
//			tex2Ds[i]->image->height.get(),
//			channelMap[tex2Ds[i]->image->channel - 1],
//			data,
//			&tex.resources[i]
//		);
//
//		pImpl->device->CreateShaderResourceView(
//			tex.resources[i],
//			&UDX12::Desc::SRV::Tex2D(tex.resources[i]->GetDesc().Format),
//			tex.allocationSRV.GetCpuHandle(static_cast<uint32_t>(i))
//		);
//	}
//
//	pImpl->textureMap.emplace(id, move(tex));
//
//	return *this;
//}

D3D12_CPU_DESCRIPTOR_HANDLE RsrcMngrDX12::GetTexture2DSrvCpuHandle(const Texture2D& tex2D) const {
	return pImpl->texture2DMap.find(tex2D.GetInstanceID())->second.allocationSRV.GetCpuHandle(0);
}
D3D12_GPU_DESCRIPTOR_HANDLE RsrcMngrDX12::GetTexture2DSrvGpuHandle(const Texture2D& tex2D) const {
	return pImpl->texture2DMap.find(tex2D.GetInstanceID())->second.allocationSRV.GetGpuHandle(0);
}
ID3D12Resource* RsrcMngrDX12::GetTexture2DResource(const Texture2D& tex2D) const {
	return pImpl->texture2DMap.find(tex2D.GetInstanceID())->second.resource;
}
D3D12_CPU_DESCRIPTOR_HANDLE RsrcMngrDX12::GetTextureCubeSrvCpuHandle(const TextureCube& texCube) const {
	return pImpl->textureCubeMap.find(texCube.GetInstanceID())->second.allocationSRV.GetCpuHandle(0);
}
D3D12_GPU_DESCRIPTOR_HANDLE RsrcMngrDX12::GetTextureCubeSrvGpuHandle(const TextureCube& texCube) const {
	return pImpl->textureCubeMap.find(texCube.GetInstanceID())->second.allocationSRV.GetGpuHandle(0);
}
ID3D12Resource* RsrcMngrDX12::GetTextureCubeResource(const TextureCube& texCube) const {
	return pImpl->textureCubeMap.find(texCube.GetInstanceID())->second.resource;
}

//UDX12::DescriptorHeapAllocation& RsrcMngrDX12::GetTextureRtvs(const Texture2D& tex2D) const {
//	return pImpl->textureMap.find(tex2D.GetInstanceID())->second.allocationRTV;
//}

UDX12::MeshGPUBuffer& RsrcMngrDX12::RegisterMesh(ID3D12GraphicsCommandList* cmdList, Mesh& mesh) {
	auto target = pImpl->meshMap.find(mesh.GetInstanceID());
	if (target == pImpl->meshMap.end()) {
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
			return iter->second;
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
			mesh.ClearVertexBuffer();
			return iter->second;
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
				mesh.ClearVertexBuffer();
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
				mesh.ClearVertexBuffer();
				meshGpuBuffer.ConvertToStatic(pImpl->deleteBatch);
			}
		}

		return meshGpuBuffer;
	}
	return pImpl->meshMap.find(mesh.GetInstanceID())->second;
}

RsrcMngrDX12& RsrcMngrDX12::UnregisterMesh(const Mesh& mesh) {
	if (auto target = pImpl->meshMap.find(mesh.GetInstanceID()); target != pImpl->meshMap.end()) {
		target->second.Delete(pImpl->deleteBatch);
		pImpl->meshMap.erase(target);
	}
	return *this;
}

UDX12::MeshGPUBuffer& RsrcMngrDX12::GetMeshGPUBuffer(const Mesh& mesh) const {
	return pImpl->meshMap.find(mesh.GetInstanceID())->second;
}

bool RsrcMngrDX12::RegisterShader(const Shader& shader) {
	auto target = pImpl->shaderMap.find(shader.GetInstanceID());
	if (target != pImpl->shaderMap.end())
		return true;

	D3D_SHADER_MACRO macros[] = {
		{nullptr, nullptr}
	};
	Ubpa::UDX12::D3DInclude d3dInclude{ shader.hlslFile->GetLocalDir(), "../" };
	auto& shaderCompileData = pImpl->shaderMap[shader.GetInstanceID()];
	shaderCompileData.passes.resize(shader.passes.size());
	for (size_t i = 0; i < shader.passes.size(); i++) {
		const auto& pass = shader.passes[i];
		Impl::ShaderCompileData::PassData passdata;
		shaderCompileData.passes[i].vsByteCode = UDX12::Util::CompileShader(
			shader.hlslFile->GetText(),
			macros,
			pass.vertexName,
			"vs_5_0",
			&d3dInclude
		);
		if (!shaderCompileData.passes[i].vsByteCode) {
			pImpl->shaderMap.erase(shader.GetInstanceID());
			return false;
		}
		shaderCompileData.passes[i].psByteCode = UDX12::Util::CompileShader(
			shader.hlslFile->GetText(),
			macros,
			pass.fragmentName,
			"ps_5_0",
			&d3dInclude
		);
		if (!shaderCompileData.passes[i].psByteCode) {
			pImpl->shaderMap.erase(shader.GetInstanceID());
			return false;
		}
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
				static_assert(false);
		}, shader.rootParameters[i]);
	}

	const auto samplers = GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		(UINT)rootParamters.size(), rootParamters.data(),
		(UINT)samplers.size(), samplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	ID3DBlob* serializedRootSig = nullptr;
	ID3DBlob* errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		&serializedRootSig, &errorBlob);

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		errorBlob->Release();
		pImpl->shaderMap.erase(shader.GetInstanceID());
		return false;
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(pImpl->device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&shaderCompileData.rootSignature)
	));

	serializedRootSig->Release();

	return true;
}

const ID3DBlob* RsrcMngrDX12::GetShaderByteCode_vs(const Shader& shader, size_t passIdx) const {
	return pImpl->shaderMap.at(shader.GetInstanceID()).passes[passIdx].vsByteCode.Get();
}

const ID3DBlob* RsrcMngrDX12::GetShaderByteCode_ps(const Shader& shader, size_t passIdx) const {
	return pImpl->shaderMap.at(shader.GetInstanceID()).passes[passIdx].psByteCode.Get();
}

ID3D12ShaderReflection* RsrcMngrDX12::GetShaderRefl_vs(const Shader& shader, size_t passIdx) const {
	return pImpl->shaderMap.at(shader.GetInstanceID()).passes[passIdx].vsRefl.Get();
}

ID3D12ShaderReflection* RsrcMngrDX12::GetShaderRefl_ps(const Shader& shader, size_t passIdx) const {
	return pImpl->shaderMap.at(shader.GetInstanceID()).passes[passIdx].psRefl.Get();
}

ID3D12RootSignature* RsrcMngrDX12::GetShaderRootSignature(const Shader& shader) const {
	return pImpl->shaderMap.at(shader.GetInstanceID()).rootSignature.Get();
}

//RsrcMngrDX12& RsrcMngrDX12::RegisterRenderTexture2D(size_t id, UINT width, UINT height, DXGI_FORMAT format) {
//	Impl::Texture tex;
//	tex.resources.resize(1);
//
//	tex.allocationSRV = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(1);
//	tex.allocationRTV = UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Allocate(1);
//
//	// create resource
//	D3D12_RESOURCE_DESC texDesc;
//	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
//	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
//	texDesc.Width = width;
//	texDesc.Height = height;
//	texDesc.MipLevels = 1;
//	texDesc.Format = format;
//	texDesc.SampleDesc.Count = 1;
//	texDesc.SampleDesc.Quality = 0;
//	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
//	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
//	ThrowIfFailed(pImpl->device->CreateCommittedResource(
//		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
//		D3D12_HEAP_FLAG_NONE, &texDesc,
//		D3D12_RESOURCE_STATE_GENERIC_READ,
//		nullptr,
//		IID_PPV_ARGS(&tex.resources[0])));
//
//	// create SRV
//	pImpl->device->CreateShaderResourceView(
//		tex.resources[0],
//		&UDX12::Desc::SRV::Tex2D(format),
//		tex.allocationSRV.GetCpuHandle());
//
//	// create RTVs
//	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
//	ZeroMemory(&rtvDesc, sizeof(rtvDesc));
//	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
//	rtvDesc.Format = format;
//	rtvDesc.Texture2D.MipSlice = 0;
//	rtvDesc.Texture2D.PlaneSlice = 0; // ?
//	pImpl->device->CreateRenderTargetView(tex.resources[0], &rtvDesc, tex.allocationRTV.GetCpuHandle());
//
//	pImpl->textureMap.emplace(id, move(tex));
//
//	return *this;
//}
//
//RsrcMngrDX12& RsrcMngrDX12::RegisterRenderTextureCube(size_t id, UINT size, DXGI_FORMAT format) {
//	Impl::Texture tex;
//	tex.resources.resize(1);
//
//	tex.allocationSRV = UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Allocate(1);
//	tex.allocationRTV = UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Allocate(6);
//
//	// create resource
//	D3D12_RESOURCE_DESC texDesc;
//	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
//	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
//	texDesc.Alignment = 0;
//	texDesc.Width = size;
//	texDesc.Height = size;
//	texDesc.DepthOrArraySize = 6;
//	texDesc.MipLevels = 1;
//	texDesc.Format = format;
//	texDesc.SampleDesc.Count = 1;
//	texDesc.SampleDesc.Quality = 0;
//	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
//	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
//	ThrowIfFailed(pImpl->device->CreateCommittedResource(
//		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
//		D3D12_HEAP_FLAG_NONE, &texDesc,
//		D3D12_RESOURCE_STATE_GENERIC_READ,
//		nullptr,
//		IID_PPV_ARGS(&tex.resources[0])));
//
//	// create SRV
//	pImpl->device->CreateShaderResourceView(
//		tex.resources[0],
//		&UDX12::Desc::SRV::TexCube(format),
//		tex.allocationSRV.GetCpuHandle());
//
//	// create RTVs
//	for (UINT i = 0; i < 6; i++)
//	{
//		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
//		ZeroMemory(&rtvDesc, sizeof(rtvDesc));
//		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
//		rtvDesc.Format = format;
//		rtvDesc.Texture2DArray.MipSlice = 0;
//		rtvDesc.Texture2DArray.PlaneSlice = 0;
//		rtvDesc.Texture2DArray.FirstArraySlice = i;
//		rtvDesc.Texture2DArray.ArraySize = 1;
//		pImpl->device->CreateRenderTargetView(tex.resources[0], &rtvDesc, tex.allocationRTV.GetCpuHandle(i));
//	}
//
//	pImpl->textureMap.emplace(id, move(tex));
//
//	return *this;
//}

size_t RsrcMngrDX12::RegisterPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc) {
	ID3D12PipelineState* pso;
	pImpl->device->CreateGraphicsPipelineState(desc, IID_PPV_ARGS(&pso));
	size_t ID = pImpl->PSOs.size();
	pImpl->PSOs.push_back(pso);
	return ID;
}

ID3D12PipelineState* RsrcMngrDX12::GetPSO(size_t id) const {
	return pImpl->PSOs[id];
}

std::array<CD3DX12_STATIC_SAMPLER_DESC, 6> RsrcMngrDX12::GetStaticSamplers() const {
	return {
		pImpl->pointWrap, pImpl->pointClamp,
		pImpl->linearWrap, pImpl->linearClamp,
		pImpl->anisotropicWrap, pImpl->anisotropicClamp
	};
}
