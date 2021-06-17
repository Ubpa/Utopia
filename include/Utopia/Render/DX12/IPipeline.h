#pragma once

#include <UDX12/UDX12.h>

#include <UECS/Entity.hpp>

#include <unordered_map>
#include <map>
#include <set>
#include <unordered_set>
#include <vector>
#include <memory>
#include <span>

namespace Ubpa::UECS {
	class World;
}

namespace Ubpa::Utopia {
	struct Material;
	struct Shader;
	struct RenderState;

	class IPipeline {
	public:
		struct InitDesc {
			size_t numFrame;
			ID3D12Device* device;
			ID3D12CommandQueue* cmdQueue;
		};
		struct CameraData {
			UECS::Entity entity;
			const UECS::World* world;
		};

		virtual ~IPipeline() = default;

		virtual void Init(InitDesc desc) = 0;
		virtual void Render(const std::vector<const UECS::World*>& worlds, std::span<const CameraData> cameraData, ID3D12Resource* default_rtb) = 0;

	public:
		//
		// Utils
		//////////

		struct ShaderCBDesc {
			// whole size == materialCBSize * indexMap.size()
			// offset = indexMap[material] * materialCBSize + offsetMap[register index]
			size_t begin_offset;
			size_t materialCBSize{ 0 };
			std::map<size_t, size_t> offsetMap; // register index -> local offset
			std::unordered_map<size_t, size_t> indexMap; // material ID -> index
		};
		static ShaderCBDesc UpdateShaderCBs(
			UDX12::DynamicUploadVector& cb,
			const Shader& shader,
			const std::unordered_set<const Material*>& materials,
			const std::set<std::string_view>& commonCBs
		);

		static void SetGraphicsRoot_CBV_SRV(
			ID3D12GraphicsCommandList* cmdList,
			UDX12::DynamicUploadVector& cb,
			const ShaderCBDesc& shaderCBDesc,
			const Material& material,
			const std::map<std::string_view, D3D12_GPU_VIRTUAL_ADDRESS>& commonCBs,
			const std::map<std::string_view, D3D12_GPU_DESCRIPTOR_HANDLE>& commonSRVs
		);
	};
}
