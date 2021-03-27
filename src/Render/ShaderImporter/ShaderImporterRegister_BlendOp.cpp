#include "ShaderImporterRegisterImpl.h"

#include <Utopia/Render/RenderState.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::ShaderImporterRegister_BlendOp() {
	UDRefl::Mngr.RegisterType<BlendOp>();
	UDRefl::Mngr.SimpleAddField<BlendOp::ADD>("ADD");
	UDRefl::Mngr.SimpleAddField<BlendOp::SUBTRACT>("SUBTRACT");
	UDRefl::Mngr.SimpleAddField<BlendOp::REV_SUBTRACT>("REV_SUBTRACT");
	UDRefl::Mngr.SimpleAddField<BlendOp::MIN>("MIN");
	UDRefl::Mngr.SimpleAddField<BlendOp::MAX>("MAX");
}
