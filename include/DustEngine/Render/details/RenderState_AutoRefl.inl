// This file is generated by Ubpa::USRefl::AutoRefl

#pragma once

#include <USRefl/USRefl.h>

template<>
struct Ubpa::USRefl::TypeInfo<Ubpa::DustEngine::CullMode>
	: Ubpa::USRefl::TypeInfoBase<Ubpa::DustEngine::CullMode>
{
	static constexpr AttrList attrs = {};

	static constexpr FieldList fields = {
		Field{"NONE", Ubpa::DustEngine::CullMode::NONE},
		Field{"FRONT", Ubpa::DustEngine::CullMode::FRONT},
		Field{"BACK", Ubpa::DustEngine::CullMode::BACK},
	};
};

template<>
struct Ubpa::USRefl::TypeInfo<Ubpa::DustEngine::ZTest>
	: Ubpa::USRefl::TypeInfoBase<Ubpa::DustEngine::ZTest>
{
	static constexpr AttrList attrs = {};

	static constexpr FieldList fields = {
		Field{"NEVER", Ubpa::DustEngine::ZTest::NEVER},
		Field{"LESS", Ubpa::DustEngine::ZTest::LESS},
		Field{"EQUAL", Ubpa::DustEngine::ZTest::EQUAL},
		Field{"LESS_EQUAL", Ubpa::DustEngine::ZTest::LESS_EQUAL},
		Field{"GREATER", Ubpa::DustEngine::ZTest::GREATER},
		Field{"NOT_EQUAL", Ubpa::DustEngine::ZTest::NOT_EQUAL},
		Field{"GREATER_EQUAL", Ubpa::DustEngine::ZTest::GREATER_EQUAL},
		Field{"ALWAYS", Ubpa::DustEngine::ZTest::ALWAYS},
	};
};

template<>
struct Ubpa::USRefl::TypeInfo<Ubpa::DustEngine::BLEND>
	: Ubpa::USRefl::TypeInfoBase<Ubpa::DustEngine::BLEND>
{
	static constexpr AttrList attrs = {};

	static constexpr FieldList fields = {
		Field{"ZERO", Ubpa::DustEngine::BLEND::ZERO},
		Field{"ONE", Ubpa::DustEngine::BLEND::ONE},
		Field{"SRC_COLOR", Ubpa::DustEngine::BLEND::SRC_COLOR},
		Field{"INV_SRC_COLOR", Ubpa::DustEngine::BLEND::INV_SRC_COLOR},
		Field{"SRC_ALPHA", Ubpa::DustEngine::BLEND::SRC_ALPHA},
		Field{"INV_SRC_ALPHA", Ubpa::DustEngine::BLEND::INV_SRC_ALPHA},
		Field{"DEST_ALPHA", Ubpa::DustEngine::BLEND::DEST_ALPHA},
		Field{"INV_DEST_ALPHA", Ubpa::DustEngine::BLEND::INV_DEST_ALPHA},
		Field{"DEST_COLOR", Ubpa::DustEngine::BLEND::DEST_COLOR},
		Field{"INV_DEST_COLOR", Ubpa::DustEngine::BLEND::INV_DEST_COLOR},
		Field{"SRC_ALPHA_SAT", Ubpa::DustEngine::BLEND::SRC_ALPHA_SAT},
		Field{"BLEND_FACTOR", Ubpa::DustEngine::BLEND::BLEND_FACTOR},
		Field{"INV_BLEND_FACTOR", Ubpa::DustEngine::BLEND::INV_BLEND_FACTOR},
		Field{"SRC1_COLOR", Ubpa::DustEngine::BLEND::SRC1_COLOR},
		Field{"INV_SRC1_COLOR", Ubpa::DustEngine::BLEND::INV_SRC1_COLOR},
		Field{"SRC1_ALPHA", Ubpa::DustEngine::BLEND::SRC1_ALPHA},
		Field{"INV_SRC1_ALPHA", Ubpa::DustEngine::BLEND::INV_SRC1_ALPHA},
	};
};

template<>
struct Ubpa::USRefl::TypeInfo<Ubpa::DustEngine::BLEND_OP>
	: Ubpa::USRefl::TypeInfoBase<Ubpa::DustEngine::BLEND_OP>
{
	static constexpr AttrList attrs = {};

	static constexpr FieldList fields = {
		Field{"ADD", Ubpa::DustEngine::BLEND_OP::ADD},
		Field{"SUBTRACT", Ubpa::DustEngine::BLEND_OP::SUBTRACT},
		Field{"REV_SUBTRACT", Ubpa::DustEngine::BLEND_OP::REV_SUBTRACT},
		Field{"MIN", Ubpa::DustEngine::BLEND_OP::MIN},
		Field{"MAX", Ubpa::DustEngine::BLEND_OP::MAX},
	};
};

template<>
struct Ubpa::USRefl::TypeInfo<Ubpa::DustEngine::BlendState>
	: Ubpa::USRefl::TypeInfoBase<Ubpa::DustEngine::BlendState>
{
	static constexpr AttrList attrs = {};

	static constexpr FieldList fields = {
		Field{"blendEnable", &Ubpa::DustEngine::BlendState::blendEnable},
		Field{"srcBlend", &Ubpa::DustEngine::BlendState::srcBlend},
		Field{"destBlend", &Ubpa::DustEngine::BlendState::destBlend},
		Field{"blendOp", &Ubpa::DustEngine::BlendState::blendOp},
		Field{"srcBlendAlpha", &Ubpa::DustEngine::BlendState::srcBlendAlpha},
		Field{"destBlendAlpha", &Ubpa::DustEngine::BlendState::destBlendAlpha},
		Field{"blendOpAlpha", &Ubpa::DustEngine::BlendState::blendOpAlpha},
	};
};

template<>
struct Ubpa::USRefl::TypeInfo<Ubpa::DustEngine::RenderState>
	: Ubpa::USRefl::TypeInfoBase<Ubpa::DustEngine::RenderState>
{
	static constexpr AttrList attrs = {};

	static constexpr FieldList fields = {
		Field{"cullMode", &Ubpa::DustEngine::RenderState::cullMode},
		Field{"zTest", &Ubpa::DustEngine::RenderState::zTest},
		Field{"zWrite", &Ubpa::DustEngine::RenderState::zWrite},
		Field{"blendState", &Ubpa::DustEngine::RenderState::blendState},
	};
};
