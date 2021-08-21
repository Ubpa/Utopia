#include "UDRefl_Register_Core_impl.h"

#include <Utopia/Core/Components/WorldToLocal.h>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_WorldToLocal() {
	Mngr.RegisterType<WorldToLocal>();
	Mngr.SimpleAddField<&WorldToLocal::value>("value");
}
