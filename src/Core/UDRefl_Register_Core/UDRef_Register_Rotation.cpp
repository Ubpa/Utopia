#include "UDRefl_Register_Core_impl.h"

#include <Utopia/Core/Components/Rotation.h>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_Rotation() {
	Mngr.RegisterType<Rotation>();
	Mngr.AddField<&Rotation::value>("value");
}
