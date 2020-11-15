#pragma once

namespace Ubpa::Utopia {
	enum class FillMode {
		WIREFRAME = 2,
		SOLID = 3
	};

	enum class CullMode {
		NONE = 1,
		FRONT = 2,
		BACK = 3
	};

	enum class CompareFunc {
		NEVER = 1,
		LESS = 2,
		EQUAL = 3,
		LESS_EQUAL = 4,
		GREATER = 5,
		NOT_EQUAL = 6,
		GREATER_EQUAL = 7,
		ALWAYS = 8
	};

	enum class Blend {
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

	enum class BlendOp {
		ADD = 1,
		SUBTRACT = 2,
		REV_SUBTRACT = 3,
		MIN = 4,
		MAX = 5
	};

	// op(src * src_rgb, dest * dest_rgb)
	struct BlendState {
		bool enable{ false };
		Blend src{ Blend::SRC_ALPHA };
		Blend dest{ Blend::INV_SRC_ALPHA };
		BlendOp op{ BlendOp::ADD };
		Blend srcAlpha{ Blend::ONE };
		Blend destAlpha{ Blend::INV_SRC_ALPHA };
		BlendOp opAlpha{ BlendOp::ADD };
	};

	enum class StencilOp {
		KEEP = 1,
		ZERO = 2,
		REPLACE = 3,
		INCR_SAT = 4,
		DECR_SAT = 5,
		INVERT = 6,
		INCR = 7,
		DECR = 8
	};

	struct StencilState {
		bool enable{ false };
		uint8_t ref{ 0 };
		uint8_t readMask{ 0xff };
		uint8_t writeMask{ 0xff };
		StencilOp failOp{ StencilOp::KEEP };
		StencilOp depthFailOp{ StencilOp::KEEP };
		StencilOp passOp{ StencilOp::KEEP };
		CompareFunc func{ CompareFunc::ALWAYS };
	};

	struct RenderState {
		FillMode fillMode{ FillMode::SOLID };

		CullMode cullMode{ CullMode::BACK };

		CompareFunc zTest{ CompareFunc::LESS };
		bool zWrite{ true };

		StencilState stencilState;

		BlendState blendStates[8];

		uint8_t colorMask[8] = { 0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f };
	};
}

#include "details/RenderState_AutoRefl.inl"
