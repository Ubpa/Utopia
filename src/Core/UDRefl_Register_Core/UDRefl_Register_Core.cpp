#include <Utopia/Core/UDRefl_Register_Core.h>

#include "UDRefl_Register_Core_impl.h"

using namespace Ubpa::Utopia;

void Ubpa::Utopia::UDRefl_Register_Core() {
	details::UDRefl_Register_Children();
	details::UDRefl_Register_Input();
	details::UDRefl_Register_LocalToParent();
	details::UDRefl_Register_LocalSerializeToWorld();
	details::UDRefl_Register_Name();
	details::UDRefl_Register_NonUniformScale();
	details::UDRefl_Register_Parent();
	details::UDRefl_Register_Roamer();
	details::UDRefl_Register_Rotation();
	details::UDRefl_Register_RotationEuler();
	details::UDRefl_Register_Scale();
	details::UDRefl_Register_Translation();
	details::UDRefl_Register_WorldTime();
	details::UDRefl_Register_WorldToLocal();
	details::UDRefl_Register_UECS();
}
