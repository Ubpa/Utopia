#include "UDRefl_Register_Core_impl.h"

#include <Utopia/Core/Components/RotationEuler.h>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_RotationEuler() {
	Mngr.RegisterType<RotationEuler>();
	Mngr.SimpleAddField<&RotationEuler::value>("value");
}
