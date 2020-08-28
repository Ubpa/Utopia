#include "InitCore.h"

#include <DustEngine/Core/Components/Camera.h>
#include <DustEngine/Core/Components/MeshFilter.h>
#include <DustEngine/Core/Components/MeshRenderer.h>
#include <DustEngine/Core/Components/WorldTime.h>

void Ubpa::DustEngine::detail::InitCore(lua_State* L) {
	ULuaPP::Register<Camera>(L);
	ULuaPP::Register<MeshFilter>(L);
	ULuaPP::Register<MeshRenderer>(L);
	ULuaPP::Register<WorldTime>(L);
}
