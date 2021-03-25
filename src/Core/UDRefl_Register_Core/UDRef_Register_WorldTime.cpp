#include "UDRefl_Register_Core_impl.h"

#include <Utopia/Core/Components/WorldTime.h>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_WorldTime() {
	Mngr.RegisterType<WorldTime>();
	Mngr.AddField<&WorldTime::deltaTime>("deltaTime");
	Mngr.AddField<&WorldTime::elapsedTime>("elapsedTime");
}
