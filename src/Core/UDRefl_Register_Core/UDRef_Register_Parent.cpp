#include "UDRefl_Register_Core_impl.h"

#include <Utopia/Core/Components/Parent.h>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_Parent() {
	Mngr.RegisterType<Parent>();
	Mngr.SimpleAddField<&Parent::value>("value");
}
