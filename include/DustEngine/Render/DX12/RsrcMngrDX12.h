#pragma once

#include <UDX12/UDX12.h>

#include <array>
#include <string>

namespace Ubpa::DustEngine {
	struct Texture2D;
	struct Shader;

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

		UDX12::MeshGPUBuffer& RegisterStaticMeshGPUBuffer(
			DirectX::ResourceUploadBatch& upload, size_t id,
			const void* vb_data, UINT vb_count, UINT vb_stride,
			const void* ib_data, UINT ib_count, DXGI_FORMAT ib_format);

		UDX12::MeshGPUBuffer& RegisterDynamicMeshGPUBuffer(
			size_t id,
			const void* vb_data, UINT vb_count, UINT vb_stride,
			const void* ib_data, UINT ib_count, DXGI_FORMAT ib_format);

		// [summary]
		// compile shader file to bytecode
		// [arguments]
		// - defines: marco array, end with {NULL, NULL}
		// - - e.g. #define zero 0 <-> D3D_SHADER_MACRO Shader_Macros[] = { "zero", "0", NULL, NULL };
		// - entrypoint: begin function name, like 'main'
		// - target: e.g. cs/ds/gs/hs/ps/vs + _5_ + 0/1
		// [ref] https://docs.microsoft.com/en-us/windows/win32/api/d3dcompiler/nf-d3dcompiler-d3dcompilefromfile
		ID3DBlob* RegisterShaderByteCode(
			size_t id,
			const std::wstring& filename,
			const D3D_SHADER_MACRO* defines,
			const std::string& entrypoint,
			const std::string& target);

		ID3DBlob* RegisterShaderByteCode(
			size_t id,
			std::string_view source,
			const D3D_SHADER_MACRO* defines,
			const std::string& entrypoint,
			const std::string& target);

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

		//UDX12::DescriptorHeapAllocation& GetTextureRtvs(const Texture2D* tex2D) const;

		UDX12::MeshGPUBuffer& GetMeshGPUBuffer(size_t id) const;

		ID3DBlob* GetShaderByteCode(size_t id) const;

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
