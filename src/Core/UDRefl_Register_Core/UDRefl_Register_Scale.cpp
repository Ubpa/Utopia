#include "UDRefl_Register_Core_impl.h"

#include <Utopia/Core/Components/Scale.h>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_Scale() {
	Mngr.RegisterType<Scale>();
	Mngr.SimpleAddField<&Scale::value>("value");
}
