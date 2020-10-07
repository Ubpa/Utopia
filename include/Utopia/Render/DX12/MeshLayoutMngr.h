#pragma once

#include <d3d12.h>

#include <vector>
#include <unordered_map>
#include <array>

namespace Ubpa::Utopia {
	class Mesh;

	/*
	position
	uv
	normal
	tangent
	color
	uv2
	uv3
	uv4
	*/
	class MeshLayoutMngr {
	public:
		static MeshLayoutMngr& Instance() {
			static MeshLayoutMngr instance;
			return instance;
		}

		const std::vector<D3D12_INPUT_ELEMENT_DESC>& GetMeshLayoutValue(size_t ID);

		static constexpr size_t GetMeshLayoutID(
			bool uv,
			bool normal,
			bool tangent,
			bool color
		) noexcept;

		// uv, normal, tangent, color
		static constexpr std::array<bool, 4> DecodeMeshLayoutID(size_t ID) noexcept;

		static size_t GetMeshLayoutID(const Mesh& mesh) noexcept;

	private:
		MeshLayoutMngr();

		// if not exist attribute, set it to offset 0
		static std::vector<D3D12_INPUT_ELEMENT_DESC> GenerateDesc(
			bool uv,
			bool normal,
			bool tangent,
			bool color
		);

		std::unordered_map<size_t, std::vector<D3D12_INPUT_ELEMENT_DESC>> layoutMap;
	};
}

#include "details/MeshLayoutMngr.inl"
