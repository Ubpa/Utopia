#include "ShaderImporterRegisterImpl.h"

#include <Utopia/Render/RenderState.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::ShaderImporterRegister_StencilOp() {
	UDRefl::Mngr.RegisterType<StencilOp>();
	UDRefl::Mngr.SimpleAddField<StencilOp::KEEP>("KEEP");
	UDRefl::Mngr.SimpleAddField<StencilOp::ZERO>("ZERO");
	UDRefl::Mngr.SimpleAddField<StencilOp::REPLACE>("REPLACE");
	UDRefl::Mngr.SimpleAddField<StencilOp::INCR_SAT>("INCR_SAT");
	UDRefl::Mngr.SimpleAddField<StencilOp::DECR_SAT>("DECR_SAT");
	UDRefl::Mngr.SimpleAddField<StencilOp::INVERT>("INVERT");
	UDRefl::Mngr.SimpleAddField<StencilOp::INCR>("INCR");
	UDRefl::Mngr.SimpleAddField<StencilOp::DECR>("DECR");
}
