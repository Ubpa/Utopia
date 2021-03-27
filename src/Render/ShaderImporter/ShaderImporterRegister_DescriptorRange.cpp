#include "ShaderImporterRegisterImpl.h"

#include <Utopia/Render/RootParameter.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::ShaderImporterRegister_DescriptorRange() {
	UDRefl::Mngr.RegisterType<DescriptorRange>();
	UDRefl::Mngr.SimpleAddField<&DescriptorRange::RangeType>("RangeType");
	UDRefl::Mngr.SimpleAddField<&DescriptorRange::NumDescriptors>("NumDescriptors");
	UDRefl::Mngr.SimpleAddField<&DescriptorRange::BaseShaderRegister>("BaseShaderRegister");
	UDRefl::Mngr.SimpleAddField<&DescriptorRange::RegisterSpace>("RegisterSpace");
}
