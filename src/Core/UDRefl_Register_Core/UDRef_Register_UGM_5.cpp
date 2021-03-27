#include "UDRefl_Register_Core_impl.h"

#include <UGM/UGM.hpp>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_UGM_5() {
	Mngr.RegisterType<trianglef2>();
	Mngr.RegisterType<trianglef3>();
	Mngr.RegisterType<bboxf2>();
	Mngr.RegisterType<bboxf3>();
}
