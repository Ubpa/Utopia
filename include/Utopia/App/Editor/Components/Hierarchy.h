#pragma once

#include <UECS/UECS.hpp>

#include <string>

namespace Ubpa::Utopia {
	struct Hierarchy {
		UECS::World* world{ nullptr };
		UECS::Entity select{ UECS::Entity::Invalid() };
		UECS::Entity hover{ UECS::Entity::Invalid() };

		bool is_saving_world{ false };
		std::string saved_path;
	};
}
