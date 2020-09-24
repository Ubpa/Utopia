#include "InitRender.h"

#include <DustEngine/Render/Components/Components.h>

void Ubpa::DustEngine::detail::InitRender(lua_State* L) {
	ULuaPP::Register<Camera>(L);
	ULuaPP::Register<Light>(L);
	ULuaPP::Register<MeshFilter>(L);
	ULuaPP::Register<MeshRenderer>(L);
	ULuaPP::Register<Skybox>(L);
}
