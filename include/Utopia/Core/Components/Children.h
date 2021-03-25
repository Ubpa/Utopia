#pragma once

#include <UECS/Entity.hpp>

#include <set>
#include <memory_resource>

namespace Ubpa::Utopia {
	struct Children {
		using allocator_type = std::pmr::polymorphic_allocator<UECS::Entity>;
		Children(const allocator_type& alloc) : value(alloc) {}
		Children(const Children& other, const allocator_type& alloc) : value(other.value, alloc) {}
		Children(Children&& other, const allocator_type& alloc) : value(std::move(other.value), alloc) {}

		std::pmr::set<UECS::Entity> value;
	};
}
