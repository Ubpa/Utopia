#pragma once

#include <UECS/UECS.hpp>

#include <string>
#include <set>

namespace Ubpa::Utopia {
	struct Hierarchy {
		UECS::World* world{ nullptr };
		std::set<UECS::Entity> selecties;
		UECS::Entity hover{ UECS::Entity::Invalid() };

		bool is_saving_world{ false };
		bool is_saving_entities{ false };
		std::string saved_path;
	};
}
