#include "UDRefl_Register_Core_impl.h"

#include <UECS/Entity.hpp>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_UECS() {
	Mngr.RegisterType<UECS::Entity>();
	Mngr.AddField<&UECS::Entity::index>("index");
	Mngr.AddField<&UECS::Entity::version>("version");
}
