#pragma once

#include <UECS/Entity.hpp>
#include <_deps/crossguid/guid.hpp>

namespace Ubpa::Utopia {
	struct Inspector {
		enum class Mode {
			Entity,
			Asset
		};

		bool lock{ false };
		Mode mode;
		UECS::Entity entity{ UECS::Entity::Invalid() };
		xg::Guid asset;
	};
}
