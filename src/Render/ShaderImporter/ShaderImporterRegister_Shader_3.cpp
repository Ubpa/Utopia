#include "ShaderImporterRegisterImpl.h"

#include <Utopia/Render/Shader.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::ShaderImporterRegister_Shader_3() {
	UDRefl::Mngr.AddField<&Shader::passes>("passes");
}
