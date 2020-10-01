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

	enum class BLEND {
		ZERO = 1,
		ONE = 2,
		SRC_COLOR = 3,
		INV_SRC_COLOR = 4,
		SRC_ALPHA = 5,
		INV_SRC_ALPHA = 6,
		DEST_ALPHA = 7,
		INV_DEST_ALPHA = 8,
		DEST_COLOR = 9,
		INV_DEST_COLOR = 10,
		SRC_ALPHA_SAT = 11,
		BLEND_FACTOR = 14,
		INV_BLEND_FACTOR = 15,
		SRC1_COLOR = 16,
		INV_SRC1_COLOR = 17,
		SRC1_ALPHA = 18,
		INV_SRC1_ALPHA = 19
	};

	enum class BLEND_OP {
		ADD = 1,
		SUBTRACT = 2,
		REV_SUBTRACT = 3,
		MIN = 4,
		MAX = 5
	};

	struct BlendState {
		bool blendEnable{ false };
		BLEND srcBlend{ BLEND::SRC_ALPHA };
		BLEND destBlend{ BLEND::INV_SRC_ALPHA };
		BLEND_OP blendOp{ BLEND_OP::ADD };
		BLEND srcBlendAlpha{ BLEND::ONE };
		BLEND destBlendAlpha{ BLEND::INV_SRC_ALPHA };
		BLEND_OP blendOpAlpha{ BLEND_OP::ADD };
	};

	struct RenderState {
		CullMode cullMode{ CullMode::BACK };
		ZTest zTest{ ZTest::LESS };
		bool zWrite{ true };
		BlendState blendState;
	};
}

#include "details/RenderState_AutoRefl.inl"
