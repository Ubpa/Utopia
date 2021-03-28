#include "UDRefl_Register_Core_impl.h"

#include <Utopia/Core/Components/LocalToWorld.h>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_LocalToWorld() {
	Mngr.RegisterType<LocalToWorld>();
	Mngr.SimpleAddField<&LocalToWorld::value>("value");
}
