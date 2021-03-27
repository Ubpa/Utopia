#include "UDRefl_Register_Core_impl.h"

#include <UGM/UGM.hpp>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_UGM_2() {
	Mngr.RegisterType<valf2>();
	Mngr.RegisterType<valf3>();
	Mngr.RegisterType<valf4>();
}
