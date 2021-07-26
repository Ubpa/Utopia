#include "UDRefl_Register_Core_impl.h"

#include <Utopia/Core/Components/Roamer.h>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_Roamer() {
	Mngr.RegisterType<Roamer>();
	Mngr.SimpleAddField<&Roamer::moveSpeed>("moveSpeed");
	Mngr.SimpleAddField<&Roamer::rotateSpeed>("rotateSpeed");
	Mngr.SimpleAddField<&Roamer::reverseUpDown>("reverseUpDown");
	Mngr.SimpleAddField<&Roamer::reverseLeftRight>("reverseLeftRight");
	Mngr.SimpleAddField<&Roamer::reverseFrontBack>("reverseFrontBack");
}
