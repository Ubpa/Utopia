#include <Utopia/Core/Systems/WorldToLocalSystem.h>

#include <Utopia/Core/Components/LocalToWorld.h>
#include <Utopia/Core/Components/WorldToLocal.h>

using namespace Ubpa::Utopia;

void WorldToLocalSystem::OnUpdate(UECS::Schedule& schedule) {
	schedule.RegisterEntityJob([](WorldToLocal* w2l, const LocalToWorld* l2w) {
		w2l->value = l2w->value.inverse();
	}, SystemFuncName);
}
