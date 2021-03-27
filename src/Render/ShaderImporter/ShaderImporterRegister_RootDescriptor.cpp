#include "ShaderImporterRegisterImpl.h"

#include <Utopia/Render/RootParameter.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::ShaderImporterRegister_RootDescriptor() {
	UDRefl::Mngr.RegisterType<RootDescriptor>();
	UDRefl::Mngr.SimpleAddField<&RootDescriptor::DescriptorType>("DescriptorType");
	UDRefl::Mngr.SimpleAddField<&RootDescriptor::ShaderRegister>("ShaderRegister");
	UDRefl::Mngr.SimpleAddField<&RootDescriptor::RegisterSpace>("RegisterSpace");
}
