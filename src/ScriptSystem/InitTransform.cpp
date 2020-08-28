#include "InitTransform.h"

#include <DustEngine/Transform/Components/Children.h>
#include <DustEngine/Transform/Components/LocalToParent.h>
#include <DustEngine/Transform/Components/LocalToWorld.h>
#include <DustEngine/Transform/Components/Parent.h>
#include <DustEngine/Transform/Components/Rotation.h>
#include <DustEngine/Transform/Components/RotationEuler.h>
#include <DustEngine/Transform/Components/Scale.h>
#include <DustEngine/Transform/Components/Translation.h>
#include <DustEngine/Transform/Components/WorldToLocal.h>

void Ubpa::DustEngine::detail::InitTransform(lua_State* L) {
	ULuaPP::Register<Children>(L);
	ULuaPP::Register<LocalToParent>(L);
	ULuaPP::Register<LocalToWorld>(L);
	ULuaPP::Register<Parent>(L);
	ULuaPP::Register<Rotation>(L);
	ULuaPP::Register<RotationEuler>(L);
	ULuaPP::Register<Scale>(L);
	ULuaPP::Register<Translation>(L);
	ULuaPP::Register<WorldToLocal>(L);
}
