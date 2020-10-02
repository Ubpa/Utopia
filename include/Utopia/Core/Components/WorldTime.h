#pragma once

namespace Ubpa::Utopia {
	// singleton
	struct WorldTime {
		double elapsedTime; // in seconds
		float deltaTime; // in seconds
	};
}

#include "details/WorldTime_AutoRefl.inl"
