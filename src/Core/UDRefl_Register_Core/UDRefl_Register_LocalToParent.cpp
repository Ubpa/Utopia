#include "UDRefl_Register_Core_impl.h"

#include <Utopia/Core/Components/LocalToParent.h>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_LocalToParent() {
	Mngr.RegisterType<LocalToParent>();
	Mngr.SimpleAddField<&LocalToParent::value>("value");
}
