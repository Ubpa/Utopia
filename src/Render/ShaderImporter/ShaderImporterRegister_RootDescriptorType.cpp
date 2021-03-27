#include "ShaderImporterRegisterImpl.h"

#include <Utopia/Render/RootParameter.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::ShaderImporterRegister_RootDescriptorType() {
	UDRefl::Mngr.RegisterType<RootDescriptorType>();
	UDRefl::Mngr.SimpleAddField<RootDescriptorType::SRV>("SRV");
	UDRefl::Mngr.SimpleAddField<RootDescriptorType::UAV>("UAV");
	UDRefl::Mngr.SimpleAddField<RootDescriptorType::CBV>("CBV");
}
