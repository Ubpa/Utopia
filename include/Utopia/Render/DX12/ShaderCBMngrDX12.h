#pragma once

#include <USignal/Connection.hpp>
#include <UDX12/UploadBuffer.h>

#include <memory>
#include <unordered_map>
#include <mutex>

namespace Ubpa::Utopia {
	struct Shader;

	// every frame resource have a ShaderCBMngrDX12
	// every shader have a self constant buffer
	// TODO: share buffer
	class ShaderCBMngrDX12 {
	public:
		ShaderCBMngrDX12(ID3D12Device* device) : device{ device } {}
		UDX12::DynamicUploadBuffer* GetBuffer(const Shader& shader);
		UDX12::DynamicUploadBuffer* GetCommonBuffer();
		void Unregister(std::size_t ID);

		ShaderCBMngrDX12(const ShaderCBMngrDX12&) = delete;
		ShaderCBMngrDX12(ShaderCBMngrDX12&&) noexcept = delete;
		ShaderCBMngrDX12 operator=(const ShaderCBMngrDX12&) = delete;
		ShaderCBMngrDX12 operator=(ShaderCBMngrDX12&&) noexcept = delete;
	private:
		std::unordered_map<std::size_t, std::unique_ptr<UDX12::DynamicUploadBuffer>> bufferMap;
		std::vector<ScopedConnection<void(std::size_t)>> connections;
		std::mutex m;
		ID3D12Device* device;
	};
}
