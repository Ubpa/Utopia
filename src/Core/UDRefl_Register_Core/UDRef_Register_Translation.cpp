#include "UDRefl_Register_Core_impl.h"

#include <Utopia/Core/Components/Translation.h>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_Translation() {
	Mngr.RegisterType<Translation>();
	Mngr.AddField<&Translation::value>("value");
}
