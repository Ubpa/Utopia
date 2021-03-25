#include "UDRefl_Register_Core_impl.h"

#include <Utopia/Core/Components/Children.h>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_Children() {
	Mngr.RegisterType<Children>();
	Mngr.AddField<&Children::value>("value");
}
