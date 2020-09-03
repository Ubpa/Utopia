#pragma once

#include <UECS/Entity.h>

namespace Ubpa::DustEngine {
	struct Inspector {
		bool lock{ false };
		UECS::Entity target;
	};
}
