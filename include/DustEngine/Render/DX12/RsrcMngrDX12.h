#pragma once

#include <UDX12/UDX12.h>

#include <array>
#include <string>

namespace Ubpa::DustEngine {
	struct Texture2D;
	class TextureCube;
	struct Shader;
	class Mesh;

	class RsrcMngrDX12 {
	public:
		static RsrcMngrDX12& Instance() noexcept {
			static RsrcMngrDX12 instance;
			return instance;
		}

		DirectX::ResourceUploadBatch& GetUpload() const;
		UDX12::ResourceDeleteBatch& GetDeleteBatch() const;

		RsrcMngrDX12& Init(ID3D12Device* device);
		void Clear();

		RsrcMngrDX12& RegisterTexture2D(
			DirectX::ResourceUploadBatch& upload,
			const Texture2D* tex2D
		);

		RsrcMngrDX12& RegisterTextureCube(
			DirectX::ResourceUploadBatch& upload,
			const TextureCube* texcube
		);
		// [sync]
		// - (maybe) construct resized upload buffer
		// - (maybe) construct resized default buffer
		// - (maybe) cpu buffer -> upload buffer
		// [async]
		// - (maybe) upload buffer -> default buffer
		// - (maybe) delete upload buffer
		UDX12::MeshGPUBuffer& RegisterMesh(
			DirectX::ResourceUploadBatch& upload,
			UDX12::ResourceDeleteBatch& deleteBatch,
			ID3D12GraphicsCommandList* cmdList,
			Mesh* mesh
		);
		
		RsrcMngrDX12& RegisterShader(const Shader* shader);

		RsrcMngrDX12& RegisterRootSignature(
			size_t id,
			const D3D12_ROOT_SIGNATURE_DESC* descs);

		size_t RegisterPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc);

		D3D12_CPU_DESCRIPTOR_HANDLE GetTexture2DSrvCpuHandle(const Texture2D* tex2D) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetTexture2DSrvGpuHandle(const Texture2D* tex2D) const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetTextureCubeSrvCpuHandle(const TextureCube* texcube) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetTextureCubeSrvGpuHandle(const TextureCube* texcube) const;
		ID3D12Resource* GetTexture2DResource(const Texture2D* tex2D) const;
		ID3D12Resource* GetTextureCubeResource(const TextureCube* texcube) const;

		UDX12::MeshGPUBuffer& GetMeshGPUBuffer(const Mesh* mesh) const;
		 
		const ID3DBlob* GetShaderByteCode_vs(const Shader* shader) const;
		const ID3DBlob* GetShaderByteCode_ps(const Shader* shader) const;

		ID3D12RootSignature* GetRootSignature(size_t id) const;

		ID3D12PipelineState* GetPSO(size_t id) const;

		// 1. point wrap
		// 2. point clamp
		// 3. linear wrap
		// 4. linear clamp
		// 5. anisotropic wrap
		// 6. anisotropic clamp
		std::array<CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers() const;

	private:
		struct Impl;
		Impl* pImpl;

		RsrcMngrDX12();
		~RsrcMngrDX12();
	};
}
