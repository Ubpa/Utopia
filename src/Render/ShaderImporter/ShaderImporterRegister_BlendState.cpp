#include "ShaderImporterRegisterImpl.h"

#include <Utopia/Render/RenderState.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::ShaderImporterRegister_BlendState() {
	UDRefl::Mngr.RegisterType<BlendState>();
	UDRefl::Mngr.SimpleAddField<&BlendState::enable>("enable");
	UDRefl::Mngr.SimpleAddField<&BlendState::src>("src");
	UDRefl::Mngr.SimpleAddField<&BlendState::dest>("dest");
	UDRefl::Mngr.SimpleAddField<&BlendState::op>("op");
	UDRefl::Mngr.SimpleAddField<&BlendState::srcAlpha>("srcAlpha");
	UDRefl::Mngr.SimpleAddField<&BlendState::destAlpha>("destAlpha");
	UDRefl::Mngr.SimpleAddField<&BlendState::opAlpha>("opAlpha");
}
