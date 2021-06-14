#pragma once

#include <UDX12/UDX12.h>

#include <UECS/Entity.hpp>

#include <unordered_map>
#include <map>
#include <set>
#include <unordered_set>
#include <vector>
#include <memory>

namespace Ubpa::UECS {
	class World;
}

namespace Ubpa::Utopia {
	struct Material;
	struct Shader;
	class ShaderCBMngrDX12;
	struct RenderState;

	class PipelineBase {
	public:
		struct InitDesc {
			size_t numFrame;
			ID3D12Device* device;
			ID3D12CommandQueue* cmdQueue;
		};
		struct CameraData {
			CameraData(UECS::Entity entity, const UECS::World& world)
				: entity{ entity }, world{ world } {}
			UECS::Entity entity;
			const UECS::World& world;
		};

		PipelineBase(InitDesc initDesc) : initDesc{ initDesc } {}

		virtual ~PipelineBase() = default;

		virtual void Render(const std::vector<const UECS::World*>& worlds, const CameraData& cameraData, ID3D12Resource* rt) = 0;

	protected:
		const InitDesc initDesc;

	public:
		//
		// Utils
		//////////

		struct ShaderCBDesc {
			// whole size == materialCBSize * indexMap.size()
			// offset = indexMap[material] * materialCBSize + offsetMap[register index]

			size_t materialCBSize{ 0 };
			std::map<size_t, size_t> offsetMap; // register index -> local offset
			std::unordered_map<size_t, size_t> indexMap; // material ID -> index
		};
		static ShaderCBDesc UpdateShaderCBs(
			ShaderCBMngrDX12& shaderCBMngr,
			const Shader& shader,
			const std::unordered_set<const Material*>& materials,
			const std::set<std::string_view>& commonCBs
		);
		static void SetGraphicsRoot_CBV_SRV(
			ID3D12GraphicsCommandList* cmdList,
			ShaderCBMngrDX12& shaderCBMngr,
			const ShaderCBDesc& shaderCBDescconst,
			const Material& material,
			const std::map<std::string_view, D3D12_GPU_VIRTUAL_ADDRESS>& commonCBs,
			const std::map<std::string_view, D3D12_GPU_DESCRIPTOR_HANDLE>& commonSRVs
		);
	};
}
