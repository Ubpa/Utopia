#pragma once

namespace Ubpa::Utopia {
	// singleton
	struct WorldTime {
		double elapsedTime; // in seconds
		float deltaTime; // in seconds
	};
}
