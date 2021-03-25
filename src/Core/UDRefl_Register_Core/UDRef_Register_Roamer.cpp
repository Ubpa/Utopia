#include "UDRefl_Register_Core_impl.h"

#include <Utopia/Core/Components/Roamer.h>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_Roamer() {
	Mngr.RegisterType<Roamer>();
	Mngr.AddField<&Roamer::moveSpeed>("moveSpeed");
	Mngr.AddField<&Roamer::rotateSpeed>("rotateSpeed");
	Mngr.AddField<&Roamer::reverseUpDown>("reverseUpDown");
	Mngr.AddField<&Roamer::reverseLeftRight>("reverseLeftRight");
	Mngr.AddField<&Roamer::reverseFrontBack>("reverseFrontBack");
}
