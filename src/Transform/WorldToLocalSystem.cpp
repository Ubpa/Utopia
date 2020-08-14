#include <DustEngine/Transform/Systems/WorldToLocalSystem.h>

#include <DustEngine/Transform/Components/LocalToWorld.h>
#include <DustEngine/Transform/Components/WorldToLocal.h>

using namespace Ubpa::DustEngine;

void WorldToLocalSystem::OnUpdate(UECS::Schedule& schedule) {
	schedule.RegisterEntityJob([](WorldToLocal* w2l, const LocalToWorld* l2w) {
		w2l->value = l2w->value.inverse();
	}, SystemFuncName);
}
