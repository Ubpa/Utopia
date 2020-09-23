#include <DustEngine/Transform/Systems/LocalToParentSystem.h>
#include <DustEngine/Transform/Systems/TRSToLocalToWorldSystem.h>

#include <DustEngine/Transform/Systems/TRSToLocalToParentSystem.h>
#include <DustEngine/Transform/Components/LocalToParent.h>
#include <DustEngine/Transform/Components/LocalToWorld.h>
#include <DustEngine/Transform/Components/Parent.h>
#include <DustEngine/Transform/Components/Children.h>

using namespace Ubpa::DustEngine;
using namespace Ubpa::UECS;

void LocalToParentSystem::ChildLocalToWorld(World* w, const transformf& parent_l2w, Entity e) {
	transformf l2w;
	if (w->entityMngr.Have(e, CmptType::Of<LocalToWorld>)) {
		auto child_l2w = w->entityMngr.Get<LocalToWorld>(e);
		auto child_l2p = w->entityMngr.Get<LocalToParent>(e);
		l2w = parent_l2w * (child_l2p ? child_l2p->value : child_l2w->value);
		child_l2w->value = l2w;
	}
	else
		l2w = parent_l2w;

	if (w->entityMngr.Have(e, CmptType::Of<Children>)) {
		auto children = w->entityMngr.Get<Children>(e);
		for (const auto& child : children->value)
			ChildLocalToWorld(w, l2w, child);
	}
}

void LocalToParentSystem::OnUpdate(Schedule& schedule) {
	ArchetypeFilter rootFilter;
	rootFilter.none = { CmptType::Of<Parent> };

	schedule.InsertNone(TRSToLocalToWorldSystem::SystemFuncName, CmptType::Of<LocalToParent>);
	schedule.RegisterEntityJob(
		[](World* w, LocalToWorld* l2w, const Children* children) {
			for (const auto& child : children->value)
				ChildLocalToWorld(w, l2w->value, child);
		},
		SystemFuncName,
		true,
		rootFilter
	);
	schedule.Order(TRSToLocalToParentSystem::SystemFuncName, SystemFuncName);
	schedule.Order(TRSToLocalToWorldSystem::SystemFuncName, SystemFuncName);
}
