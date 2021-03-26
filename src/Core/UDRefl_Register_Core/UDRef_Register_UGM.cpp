#include "UDRefl_Register_Core_impl.h"

#include <UGM/UGM.hpp>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_UGM() {
	Mngr.RegisterType<vecf2>();
	Mngr.RegisterType<vecf3>();
	Mngr.RegisterType<vecf4>();
	Mngr.RegisterType<pointf2>();
	Mngr.RegisterType<pointf3>();
	Mngr.RegisterType<pointf4>();
	Mngr.RegisterType<valf2>();
	Mngr.RegisterType<valf3>();
	Mngr.RegisterType<valf4>();
	Mngr.RegisterType<matf2>();
	Mngr.RegisterType<matf3>();
	Mngr.RegisterType<matf4>();
	Mngr.RegisterType<transformf>();
	Mngr.RegisterType<eulerf>();
	Mngr.RegisterType<quatf>();
	Mngr.RegisterType<rgbf>();
	Mngr.RegisterType<rgbaf>();
	Mngr.RegisterType<trianglef2>();
	Mngr.RegisterType<trianglef3>();
	Mngr.RegisterType<bboxf2>();
	Mngr.RegisterType<bboxf3>();
}
