#include "UDRefl_Register_Core_impl.h"

#include <Utopia/Core/Components/Name.h>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_Name() {
	Mngr.RegisterType<Name>();
	Mngr.AddField<&Name::value>("value");
}
