#include <Utopia/Core/Systems/LocalToParentSystem.h>
#include <Utopia/Core/Systems/TRSToLocalToWorldSystem.h>

#include <Utopia/Core/Systems/TRSToLocalToParentSystem.h>
#include <Utopia/Core/Components/LocalToParent.h>
#include <Utopia/Core/Components/LocalToWorld.h>
#include <Utopia/Core/Components/Parent.h>
#include <Utopia/Core/Components/Children.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UECS;

void LocalToParentSystem::ChildLocalSerializeToWorld(World* w, const transformf& parent_l2w, Entity e) {
	transformf l2w;
	if (w->entityMngr.Have(e, TypeID_of<LocalToWorld>) && w->entityMngr.Have(e, TypeID_of<LocalToParent>)) {
		auto child_l2w = w->entityMngr.WriteComponent<LocalToWorld>(e);
		auto child_l2p = w->entityMngr.ReadComponent<LocalToParent>(e);
		l2w = parent_l2w * child_l2p->value;
		child_l2w->value = l2w;
	}
	else
		l2w = parent_l2w;

	if (w->entityMngr.Have(e, TypeID_of<Children>)) {
		auto children = w->entityMngr.ReadComponent<Children>(e);
		for (const auto& child : children->value)
			ChildLocalSerializeToWorld(w, l2w, child);
	}
}

void LocalToParentSystem::OnUpdate(Schedule& schedule) {
	schedule.AddNone(TRSToLocalToWorldSystem::SystemFuncName, TypeID_of<Parent>);
	schedule.RegisterEntityJob(
		[](World* w, const LocalToWorld* l2w, const Children* children) {
			for (const auto& child : children->value)
				ChildLocalSerializeToWorld(w, l2w->value, child);
		},
		SystemFuncName,
		Schedule::EntityJobConfig {
			.archetypeFilter = {
				.none = { TypeID_of<Parent> }
			},
			.randomAccessor = {
				.types = {
					UECS::AccessTypeID_of<Write<LocalToWorld>>,
					UECS::AccessTypeID_of<Latest<Children>>,
					UECS::AccessTypeID_of<Latest<LocalToParent>>
				},
			}
		}
	);
	schedule.Order(TRSToLocalToWorldSystem::SystemFuncName, LocalToParentSystem::SystemFuncName);
}
