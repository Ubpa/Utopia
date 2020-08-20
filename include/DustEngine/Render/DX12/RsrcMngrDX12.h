#pragma once

#include <UDX12/UDX12.h>

#include <array>
#include <string>

namespace Ubpa::DustEngine {
	struct Texture2D;
	struct Shader;
	class Mesh;

	// TODO
	// 1. IsRegistered
	// 2. DynamicMesh
	// 3. Shader multi-compile
	class RsrcMngrDX12 {
	public:
		static RsrcMngrDX12& Instance() noexcept {
			static RsrcMngrDX12 instance;
			return instance;
		}

		DirectX::ResourceUploadBatch& GetUpload() const;

		RsrcMngrDX12& Init(ID3D12Device* device);
		void Clear();

		// support tex2d and tex cube
		/*
		RsrcMngrDX12& RegisterTexture2D(DirectX::ResourceUploadBatch& upload,
			size_t id, std::wstring_view filename);
		RsrcMngrDX12& RegisterDDSTextureArrayFromFile(DirectX::ResourceUploadBatch& upload,
			size_t id, const std::wstring_view* filenameArr, UINT num);
		*/
		RsrcMngrDX12& RegisterTexture2D(
			DirectX::ResourceUploadBatch& upload,
			const Texture2D* tex2D
		);
		/*RsrcMngrDX12& RegisterTexture2DArray(DirectX::ResourceUploadBatch& upload,
			size_t id, const Texture2D** tex2Ds, size_t num);*/

		// non-const mesh -> update
		UDX12::MeshGPUBuffer& RegisterStaticMesh(
			DirectX::ResourceUploadBatch& upload,
			Mesh* mesh
		);

		/*UDX12::MeshGPUBuffer& RegisterDynamicMesh(
			size_t id,
			const void* vb_data, UINT vb_count, UINT vb_stride,
			const void* ib_data, UINT ib_count, DXGI_FORMAT ib_format);*/
		
		RsrcMngrDX12& RegisterShader(const Shader* shader);

		RsrcMngrDX12& RegisterRootSignature(
			size_t id,
			const D3D12_ROOT_SIGNATURE_DESC* descs);

		RsrcMngrDX12& RegisterPSO(
			size_t id,
			const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc);

		/*RsrcMngrDX12& RegisterRenderTexture2D(size_t id, UINT width, UINT height, DXGI_FORMAT format);
		RsrcMngrDX12& RegisterRenderTextureCube(size_t id, UINT size, DXGI_FORMAT format);*/

		D3D12_CPU_DESCRIPTOR_HANDLE GetTexture2DSrvCpuHandle(const Texture2D* tex2D) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetTexture2DSrvGpuHandle(const Texture2D* tex2D) const;
		ID3D12Resource* GetTexture2DResource(const Texture2D* tex2D) const;

		//UDX12::DescriptorHeapAllocation& GetTextureRtvs(const Texture2D* tex2D) const;

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
