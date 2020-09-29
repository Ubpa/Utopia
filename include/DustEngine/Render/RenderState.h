#pragma once

namespace Ubpa::DustEngine {
	enum class CullMode {
		NONE = 1,
		FRONT = 2,
		BACK = 3
	};

	enum class ZTest {
		NEVER = 1,
		LESS = 2,
		EQUAL = 3,
		LESS_EQUAL = 4,
		GREATER = 5,
		NOT_EQUAL = 6,
		GREATER_EQUAL = 7,
		ALWAYS = 8
	};

	struct RenderState {
		CullMode cullMode{ CullMode::BACK };
		ZTest zTest{ ZTest::LESS };
		bool zWrite{ true };
	};
}

#include "details/RenderState_AutoRefl.inl"
