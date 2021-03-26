#include "UDRefl_Register_Core_impl.h"

#include <UECS/Entity.hpp>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_UECS() {
	Mngr.RegisterType<UECS::Entity>();
	Mngr.SimpleAddField<&UECS::Entity::index>("index");
	Mngr.SimpleAddField<&UECS::Entity::version>("version");
}
