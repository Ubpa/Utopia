#include "InitCore.h"

#include <DustEngine/Core/Components/Components.h>

void Ubpa::DustEngine::detail::InitCore(lua_State* L) {
	ULuaPP::Register<Children>(L);
	ULuaPP::Register<Input>(L);
	ULuaPP::Register<LocalToParent>(L);
	ULuaPP::Register<LocalToWorld>(L);
	ULuaPP::Register<Name>(L);
	ULuaPP::Register<Parent>(L);
	ULuaPP::Register<Roamer>(L);
	ULuaPP::Register<Rotation>(L);
	ULuaPP::Register<RotationEuler>(L);
	ULuaPP::Register<Scale>(L);
	ULuaPP::Register<Translation>(L);
	ULuaPP::Register<WorldTime>(L);
	ULuaPP::Register<WorldToLocal>(L);
}
