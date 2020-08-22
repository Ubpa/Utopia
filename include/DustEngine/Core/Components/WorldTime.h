#pragma once

namespace Ubpa::DustEngine {
	struct WorldTime {
		double elapsedTime; // in seconds
		float deltaTime; // in seconds
	};
}

#include "details/WorldTime_AutoRefl.inl"
