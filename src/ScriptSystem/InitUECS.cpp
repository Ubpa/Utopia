#include "InitUECS.h"

#include "detail/UECS_AutoRefl/UECS_AutoRefl.h"

void Ubpa::DustEngine::detail::InitUECS(lua_State* L) {
	ULuaPP::Register<UECS::AccessMode>(L);
	ULuaPP::Register<UECS::ChunkView>(L);
	ULuaPP::Register<UECS::CmptPtr>(L);
	ULuaPP::Register<UECS::CmptsView>(L);
	ULuaPP::Register<UECS::CmptType>(L);
	ULuaPP::Register<UECS::EntityFilter>(L);
	ULuaPP::Register<UECS::EntityLocator>(L);
	ULuaPP::Register<UECS::EntityMngr>(L);
	ULuaPP::Register<UECS::EntityQuery>(L);
	ULuaPP::Register<UECS::Entity>(L);
	ULuaPP::Register<UECS::RTDCmptTraits>(L);
	ULuaPP::Register<UECS::Schedule>(L);
	ULuaPP::Register<UECS::System>(L);
	ULuaPP::Register<UECS::SystemMngr>(L);
	ULuaPP::Register<UECS::World>(L);
}
