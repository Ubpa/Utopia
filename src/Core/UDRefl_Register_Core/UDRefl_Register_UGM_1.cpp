#include "UDRefl_Register_Core_impl.h"

#include <UGM/UGM.hpp>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_UGM_1() {
	Mngr.RegisterType<pointf2>();
	Mngr.RegisterType<pointf3>();
	Mngr.RegisterType<pointf4>();
}
