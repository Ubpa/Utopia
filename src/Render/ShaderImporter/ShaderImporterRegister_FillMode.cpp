#include "ShaderImporterRegisterImpl.h"

#include <Utopia/Render/RenderState.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::ShaderImporterRegister_FillMode() {
	UDRefl::Mngr.RegisterType<FillMode>();
	UDRefl::Mngr.SimpleAddField<FillMode::WIREFRAME>("WIREFRAME");
	UDRefl::Mngr.SimpleAddField<FillMode::SOLID>("SOLID");
}
