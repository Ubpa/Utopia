#pragma once

#include <UDX12/UDX12.h>

#include <array>
#include <string>

namespace Ubpa::Utopia {
	struct Texture2D;
	class TextureCube;
	struct Shader;
	class Mesh;

	class GPURsrcMngrDX12 {
	public:
		static GPURsrcMngrDX12& Instance() noexcept {
			static GPURsrcMngrDX12 instance;
			return instance;
		}

		GPURsrcMngrDX12& Init(ID3D12Device* device);
		void Clear(ID3D12CommandQueue* cmdQueue);

		std::function<void()> CommitUploadAndPackDelete(ID3D12CommandQueue* cmdQueue);

		//
		// Register
		/////////////

		GPURsrcMngrDX12& RegisterTexture2D(Texture2D& tex2D);

		GPURsrcMngrDX12& RegisterTextureCube(TextureCube& texcube);

		// [sync]
		// - (maybe) construct resized upload buffer
		// - (maybe) construct resized default buffer
		// - (maybe) cpu buffer -> upload buffer
		// [async]
		// - (maybe) upload buffer -> default buffer
		// - (maybe) delete upload buffer
		UDX12::MeshGPUBuffer& RegisterMesh(ID3D12GraphicsCommandList* cmdList, Mesh& mesh);

		bool RegisterShader(Shader& shader);

		size_t RegisterPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc);

		//
		// Unregister
		///////////////

		GPURsrcMngrDX12& UnregisterTexture2D(std::size_t ID);
		GPURsrcMngrDX12& UnregisterTextureCube(std::size_t ID);
		GPURsrcMngrDX12& UnregisterMesh(std::size_t ID);
		GPURsrcMngrDX12& UnregisterShader(std::size_t ID);

		//
		// Get
		////////

		D3D12_CPU_DESCRIPTOR_HANDLE GetTexture2DSrvCpuHandle(const Texture2D& tex2D) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetTexture2DSrvGpuHandle(const Texture2D& tex2D) const;
		ID3D12Resource* GetTexture2DResource(const Texture2D& tex2D) const;

		D3D12_CPU_DESCRIPTOR_HANDLE GetTextureCubeSrvCpuHandle(const TextureCube& texcube) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetTextureCubeSrvGpuHandle(const TextureCube& texcube) const;
		ID3D12Resource* GetTextureCubeResource(const TextureCube& texcube) const;

		UDX12::MeshGPUBuffer& GetMeshGPUBuffer(const Mesh& mesh) const;

		const ID3DBlob* GetShaderByteCode_vs(const Shader& shader, size_t passIdx) const;
		const ID3DBlob* GetShaderByteCode_ps(const Shader& shader, size_t passIdx) const;
		ID3D12ShaderReflection* GetShaderRefl_vs(const Shader& shader, size_t passIdx) const;
		ID3D12ShaderReflection* GetShaderRefl_ps(const Shader& shader, size_t passIdx) const;
		ID3D12RootSignature* GetShaderRootSignature(const Shader& shader) const;

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

		GPURsrcMngrDX12();
		~GPURsrcMngrDX12();
	};
}
