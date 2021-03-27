#include "ShaderImporterRegisterImpl.h"

#include <Utopia/Render/RenderState.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::ShaderImporterRegister_CompareFunc() {
	UDRefl::Mngr.RegisterType<CompareFunc>();
	UDRefl::Mngr.SimpleAddField<CompareFunc::NEVER>("NEVER");
	UDRefl::Mngr.SimpleAddField<CompareFunc::LESS>("LESS");
	UDRefl::Mngr.SimpleAddField<CompareFunc::EQUAL>("EQUAL");
	UDRefl::Mngr.SimpleAddField<CompareFunc::LESS_EQUAL>("LESS_EQUAL");
	UDRefl::Mngr.SimpleAddField<CompareFunc::GREATER>("GREATER");
	UDRefl::Mngr.SimpleAddField<CompareFunc::NOT_EQUAL>("NOT_EQUAL");
	UDRefl::Mngr.SimpleAddField<CompareFunc::GREATER_EQUAL>("GREATER_EQUAL");
	UDRefl::Mngr.SimpleAddField<CompareFunc::ALWAYS>("ALWAYS");
}
