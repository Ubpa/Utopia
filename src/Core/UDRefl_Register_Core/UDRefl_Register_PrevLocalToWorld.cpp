#include "UDRefl_Register_Core_impl.h"

#include <Utopia/Core/Components/PrevLocalToWorld.h>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_PrevLocalToWorld() {
	Mngr.RegisterType<PrevLocalToWorld>();
	Mngr.SimpleAddField<&PrevLocalToWorld::value>("value");
}
