#include "InitUECS.h"

#include "details/UECS_AutoRefl/UECS_AutoRefl.h"

void Ubpa::Utopia::detail::InitUECS(lua_State* L) {
	ULuaPP::Register<UECS::AccessMode>(L);
	ULuaPP::Register<UECS::ArchetypeFilter>(L);
	ULuaPP::Register<UECS::ChunkView>(L);
	ULuaPP::Register<UECS::CmptLocator>(L);
	ULuaPP::Register<UECS::CmptPtr>(L);
	ULuaPP::Register<UECS::CmptAccessPtr>(L);
	ULuaPP::Register<UECS::CmptsView>(L);
	ULuaPP::Register<UECS::CmptType>(L);
	ULuaPP::Register<UECS::CmptAccessType>(L);
	ULuaPP::Register<UECS::Entity>(L);
	ULuaPP::Register<UECS::EntityMngr>(L);
	ULuaPP::Register<UECS::EntityQuery>(L);
	ULuaPP::Register<UECS::RTDCmptTraits>(L);
	ULuaPP::Register<UECS::Schedule>(L);
	ULuaPP::Register<UECS::SingletonLocator>(L);
	ULuaPP::Register<UECS::SingletonsView>(L);
	ULuaPP::Register<UECS::SystemMngr>(L);
	ULuaPP::Register<UECS::SystemTraits>(L);
	ULuaPP::Register<UECS::World>(L);
}
