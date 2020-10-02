#pragma once

namespace Ubpa::Utopia {
	struct Roamer {
		float moveSpeed{ 1.f };
		float rotateSpeed{ 1.f };
		bool reverseUpDown{ false };
		bool reverseLeftRight{ false };
		bool reverseFrontBack{ false };
	};
}

#include "details/Roamer_AutoRefl.inl"
