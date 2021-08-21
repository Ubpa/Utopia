#include "UDRefl_Register_Core_impl.h"

#include <UGM/UGM.hpp>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_UGM_4() {
	Mngr.RegisterType<transformf>();
	Mngr.RegisterType<eulerf>();
	Mngr.RegisterType<quatf>();
	Mngr.RegisterType<rgbf>();
	Mngr.RegisterType<rgbaf>();
}
