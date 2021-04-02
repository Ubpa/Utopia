#include <Utopia/Render/DX12/GPURsrcMngrDX12.h>

#include <Utopia/Render/Texture2D.h>
#include <Utopia/Render/TextureCube.h>
#include <Utopia/Core/Image.h>
#include <Utopia/Render/HLSLFile.h>
#include <Utopia/Render/Shader.h>
#include <Utopia/Render/Mesh.h>

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
	struct RenderTargetGPUData {
		RenderTargetGPUData() = default;
		RenderTargetGPUData(const RenderTargetGPUData&) = default;
		RenderTargetGPUData(RenderTargetGPUData&&) noexcept = default;
		~RenderTargetGPUData() {
			if (!allocationSRV.IsNull())
				UDX12::DescriptorHeapMngr::Instance().GetCSUGpuDH()->Free(std::move(allocationSRV));
			if (!allocationRTV.IsNull())
				UDX12::DescriptorHeapMngr::Instance().GetRTVCpuDH()->Free(std::move(allocationRTV));
		}

		Microsoft::WRL::ComPtr<ID3D12Resource> resources;
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

	unordered_map<std::size_t, Texture2DGPUData> texture2DMap;
	unordered_map<std::size_t, TextureCubeGPUData> textureCubeMap;
	unordered_map<std::size_t, RenderTargetGPUData> renderTargetMap;
	unordered_map<std::size_t, ShaderCompileData> shaderMap;
	unordered_map<std::size_t, UDX12::MeshGPUBuffer> meshMap;
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

GPURsrcMngrDX12::GPURsrcMngrDX12()
	: pImpl(new Impl)
{
}

GPURsrcMngrDX12::~GPURsrcMngrDX12() {
	assert(!pImpl->isInit);
	delete(pImpl);
}

GPURsrcMngrDX12& GPURsrcMngrDX12::Init(ID3D12Device* device) {
	assert(!pImpl->isInit);

	pImpl->device = device;
	pImpl->upload = new DirectX::ResourceUploadBatch{ device };
	pImpl->upload->Begin();

	pImpl->isInit = true;
	return *this;
}

void GPURsrcMngrDX12::Clear(ID3D12CommandQueue* cmdQueue) {
	assert(pImpl->isInit);

	pImpl->upload->End(cmdQueue);
	pImpl->deleteBatch.Pack()(); // maybe we need to return the delete callback

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

std::function<void()> GPURsrcMngrDX12::CommitUploadAndPackDelete(ID3D12CommandQueue* cmdQueue) {
	if (pImpl->hasUpload) {
		pImpl->upload->End(cmdQueue);
		pImpl->hasUpload = false;
		pImpl->upload->Begin();
	}

	return pImpl->deleteBatch.Pack();
}

GPURsrcMngrDX12& GPURsrcMngrDX12::RegisterTexture2D(Texture2D& tex2D) {
	if (tex2D.IsDirty())
		pImpl->texture2DMap.erase(tex2D.GetInstanceID());

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

	const auto tex2DSRVDesc = UDX12::Desc::SRV::Tex2D(tex.resource->GetDesc().Format);
	pImpl->device->CreateShaderResourceView(
		tex.resource.Get(),
		&tex2DSRVDesc,
		tex.allocationSRV.GetCpuHandle(static_cast<uint32_t>(0))
	);

	pImpl->texture2DMap.emplace_hint(target, tex2D.GetInstanceID(), move(tex));

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
		pImpl->textureCubeMap.erase(texcube.GetInstanceID());

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

	pImpl->textureCubeMap.emplace_hint(target, std::make_pair(texcube.GetInstanceID(), std::move(tex)));

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

D3D12_CPU_DESCRIPTOR_HANDLE GPURsrcMngrDX12::GetTexture2DSrvCpuHandle(const Texture2D& tex2D) const {
	return pImpl->texture2DMap.find(tex2D.GetInstanceID())->second.allocationSRV.GetCpuHandle(0);
}
D3D12_GPU_DESCRIPTOR_HANDLE GPURsrcMngrDX12::GetTexture2DSrvGpuHandle(const Texture2D& tex2D) const {
	return pImpl->texture2DMap.find(tex2D.GetInstanceID())->second.allocationSRV.GetGpuHandle(0);
}
ID3D12Resource* GPURsrcMngrDX12::GetTexture2DResource(const Texture2D& tex2D) const {
	return pImpl->texture2DMap.find(tex2D.GetInstanceID())->second.resource.Get();
}
D3D12_CPU_DESCRIPTOR_HANDLE GPURsrcMngrDX12::GetTextureCubeSrvCpuHandle(const TextureCube& texCube) const {
	return pImpl->textureCubeMap.find(texCube.GetInstanceID())->second.allocationSRV.GetCpuHandle(0);
}
D3D12_GPU_DESCRIPTOR_HANDLE GPURsrcMngrDX12::GetTextureCubeSrvGpuHandle(const TextureCube& texCube) const {
	return pImpl->textureCubeMap.find(texCube.GetInstanceID())->second.allocationSRV.GetGpuHandle(0);
}
ID3D12Resource* GPURsrcMngrDX12::GetTextureCubeResource(const TextureCube& texCube) const {
	return pImpl->textureCubeMap.find(texCube.GetInstanceID())->second.resource.Get();
}

UDX12::MeshGPUBuffer& GPURsrcMngrDX12::RegisterMesh(ID3D12GraphicsCommandList* cmdList, Mesh& mesh) {
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
			mesh.SetClean();
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
			mesh.SetClean();
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
					mesh.SetClean();
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
					mesh.SetClean();
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
					mesh.SetClean();
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
					mesh.SetClean();
				}
				mesh.ClearVertexBuffer();
				meshGpuBuffer.ConvertToStatic(pImpl->deleteBatch);
			}
		}

		return meshGpuBuffer;
	}
}

GPURsrcMngrDX12& GPURsrcMngrDX12::UnregisterMesh(std::size_t ID) {
	static std::mutex m;
	std::lock_guard guard{ m };
	if (auto target = pImpl->meshMap.find(ID); target != pImpl->meshMap.end()) {
		target->second.Delete(pImpl->deleteBatch);
		pImpl->meshMap.erase(target);
	}
	return *this;
}

UDX12::MeshGPUBuffer& GPURsrcMngrDX12::GetMeshGPUBuffer(const Mesh& mesh) const {
	return pImpl->meshMap.at(mesh.GetInstanceID());
}

bool GPURsrcMngrDX12::RegisterShader(Shader& shader) {
	if (shader.IsDirty())
		pImpl->shaderMap.erase(shader.GetInstanceID());

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

	shader.SetClean();
	shader.destroyed.Connect<&GPURsrcMngrDX12::UnregisterShader>(this);

	return true;
}

GPURsrcMngrDX12& GPURsrcMngrDX12::UnregisterShader(std::size_t ID) {
	static std::mutex m;
	std::lock_guard guard{ m };
	if (auto target = pImpl->shaderMap.find(ID); target != pImpl->shaderMap.end()) {
		pImpl->deleteBatch.AddCallback([data = std::move(target->second)]() {});
		pImpl->shaderMap.erase(target);
	}
	return *this;
}

const ID3DBlob* GPURsrcMngrDX12::GetShaderByteCode_vs(const Shader& shader, size_t passIdx) const {
	return pImpl->shaderMap.at(shader.GetInstanceID()).passes[passIdx].vsByteCode.Get();
}

const ID3DBlob* GPURsrcMngrDX12::GetShaderByteCode_ps(const Shader& shader, size_t passIdx) const {
	return pImpl->shaderMap.at(shader.GetInstanceID()).passes[passIdx].psByteCode.Get();
}

ID3D12ShaderReflection* GPURsrcMngrDX12::GetShaderRefl_vs(const Shader& shader, size_t passIdx) const {
	return pImpl->shaderMap.at(shader.GetInstanceID()).passes[passIdx].vsRefl.Get();
}

ID3D12ShaderReflection* GPURsrcMngrDX12::GetShaderRefl_ps(const Shader& shader, size_t passIdx) const {
	return pImpl->shaderMap.at(shader.GetInstanceID()).passes[passIdx].psRefl.Get();
}

ID3D12RootSignature* GPURsrcMngrDX12::GetShaderRootSignature(const Shader& shader) const {
	return pImpl->shaderMap.at(shader.GetInstanceID()).rootSignature.Get();
}

size_t GPURsrcMngrDX12::RegisterPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc) {
	ID3D12PipelineState* pso;
	pImpl->device->CreateGraphicsPipelineState(desc, IID_PPV_ARGS(&pso));
	size_t ID = pImpl->PSOs.size();
	pImpl->PSOs.push_back(pso);
	return ID;
}

ID3D12PipelineState* GPURsrcMngrDX12::GetPSO(size_t id) const {
	return pImpl->PSOs[id];
}

std::array<CD3DX12_STATIC_SAMPLER_DESC, 6> GPURsrcMngrDX12::GetStaticSamplers() const {
	return {
		pImpl->pointWrap, pImpl->pointClamp,
		pImpl->linearWrap, pImpl->linearClamp,
		pImpl->anisotropicWrap, pImpl->anisotropicClamp
	};
}
