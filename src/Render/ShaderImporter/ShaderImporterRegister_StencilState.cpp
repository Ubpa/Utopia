#include "ShaderImporterRegisterImpl.h"

#include <Utopia/Render/RenderState.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::ShaderImporterRegister_StencilState() {
	UDRefl::Mngr.RegisterType<StencilState>();
	UDRefl::Mngr.SimpleAddField<&StencilState::enable>("enable");
	UDRefl::Mngr.SimpleAddField<&StencilState::ref>("ref");
	UDRefl::Mngr.SimpleAddField<&StencilState::readMask>("readMask");
	UDRefl::Mngr.SimpleAddField<&StencilState::writeMask>("writeMask");
	UDRefl::Mngr.SimpleAddField<&StencilState::failOp>("failOp");
	UDRefl::Mngr.SimpleAddField<&StencilState::depthFailOp>("depthFailOp");
	UDRefl::Mngr.SimpleAddField<&StencilState::passOp>("passOp");
	UDRefl::Mngr.SimpleAddField<&StencilState::func>("func");
}
