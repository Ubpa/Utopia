#pragma once

#include "../Material.h"
#include "../../Core/SharedVar.h"

#include <vector>

namespace Ubpa::Utopia {
	struct MeshRenderer {
		using allocator_type = std::pmr::vector<SharedVar<Material>>::allocator_type;
		MeshRenderer() = default;
		MeshRenderer(const allocator_type& alloc) : materials(alloc) {}
		MeshRenderer(const MeshRenderer& other, const allocator_type& alloc) : materials(other.materials, alloc) {}
		MeshRenderer(MeshRenderer&& other, const allocator_type& alloc) : materials(std::move(other.materials), alloc) {}

		std::pmr::vector<SharedVar<Material>> materials;
	};
}
