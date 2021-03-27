#include "ShaderImporterRegisterImpl.h"

#include <Utopia/Render/RenderState.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::ShaderImporterRegister_Blend() {
	UDRefl::Mngr.RegisterType<Blend>();
	UDRefl::Mngr.SimpleAddField<Blend::ZERO>("ZERO");
	UDRefl::Mngr.SimpleAddField<Blend::ONE>("ONE");
	UDRefl::Mngr.SimpleAddField<Blend::SRC_COLOR>("SRC_COLOR");
	UDRefl::Mngr.SimpleAddField<Blend::INV_SRC_COLOR>("INV_SRC_COLOR");
	UDRefl::Mngr.SimpleAddField<Blend::SRC_ALPHA>("SRC_ALPHA");
	UDRefl::Mngr.SimpleAddField<Blend::INV_SRC_ALPHA>("INV_SRC_ALPHA");
	UDRefl::Mngr.SimpleAddField<Blend::DEST_ALPHA>("DEST_ALPHA");
	UDRefl::Mngr.SimpleAddField<Blend::INV_DEST_ALPHA>("INV_DEST_ALPHA");
	UDRefl::Mngr.SimpleAddField<Blend::DEST_COLOR>("DEST_COLOR");
	UDRefl::Mngr.SimpleAddField<Blend::INV_DEST_COLOR>("INV_DEST_COLOR");
	UDRefl::Mngr.SimpleAddField<Blend::SRC_ALPHA_SAT>("SRC_ALPHA_SAT");
	UDRefl::Mngr.SimpleAddField<Blend::BLEND_FACTOR>("BLEND_FACTOR");
	UDRefl::Mngr.SimpleAddField<Blend::INV_BLEND_FACTOR>("INV_BLEND_FACTOR");
	UDRefl::Mngr.SimpleAddField<Blend::SRC1_COLOR>("SRC1_COLOR");
	UDRefl::Mngr.SimpleAddField<Blend::INV_SRC1_COLOR>("INV_SRC1_COLOR");
	UDRefl::Mngr.SimpleAddField<Blend::SRC1_ALPHA>("SRC1_ALPHA");
	UDRefl::Mngr.SimpleAddField<Blend::INV_SRC1_ALPHA>("INV_SRC1_ALPHA");
}
