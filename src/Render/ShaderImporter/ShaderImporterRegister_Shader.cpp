#include "ShaderImporterRegisterImpl.h"

#include <Utopia/Render/Shader.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::ShaderImporterRegister_Shader() {
	UDRefl::Mngr.RegisterType<Shader>();
	UDRefl::Mngr.AddField<&Shader::hlslFile>("hlslFile");
	UDRefl::Mngr.SimpleAddField<&Shader::name>("name");
}
