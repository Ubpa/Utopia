#include "UDRefl_Register_Core_impl.h"

#include <Utopia/Core/Components/NonUniformScale.h>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_NonUniformScale() {
	Mngr.RegisterType<NonUniformScale>();
	Mngr.AddField<&NonUniformScale::value>("value");
}
