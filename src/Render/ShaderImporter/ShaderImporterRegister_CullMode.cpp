#include "ShaderImporterRegisterImpl.h"

#include <Utopia/Render/RenderState.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::ShaderImporterRegister_CullMode() {
	UDRefl::Mngr.RegisterType<CullMode>();
	UDRefl::Mngr.SimpleAddField<CullMode::NONE>("NONE");
	UDRefl::Mngr.SimpleAddField<CullMode::FRONT>("FRONT");
	UDRefl::Mngr.SimpleAddField<CullMode::BACK>("BACK");
}
