#include "ShaderImporterRegisterImpl.h"

#include <Utopia/Render/RootParameter.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::ShaderImporterRegister_RootConstants() {
	UDRefl::Mngr.RegisterType<RootConstants>();
	UDRefl::Mngr.SimpleAddField<&RootConstants::ShaderRegister>("ShaderRegister");
	UDRefl::Mngr.SimpleAddField<&RootConstants::RegisterSpace>("RegisterSpace");
	UDRefl::Mngr.SimpleAddField<&RootConstants::Num32BitValues>("Num32BitValues");
}
