#include "ShaderImporterRegisterImpl.h"

#include <Utopia/Render/ShaderPass.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::ShaderImporterRegister_ShaderPass() {
	UDRefl::Mngr.RegisterType<ShaderPass>();
	UDRefl::Mngr.SimpleAddField<&ShaderPass::vertexName>("vertexName");
	UDRefl::Mngr.SimpleAddField<&ShaderPass::fragmentName>("fragmentName");
	UDRefl::Mngr.SimpleAddField<&ShaderPass::renderState>("renderState");
	UDRefl::Mngr.AddField<&ShaderPass::tags>("tags");
	UDRefl::Mngr.SimpleAddField<&ShaderPass::queue>("queue");
}
