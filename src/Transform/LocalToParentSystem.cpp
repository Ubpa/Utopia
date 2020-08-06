#include <DustEngine/Transform/Systems/LocalToParentSystem.h>
#include <DustEngine/Transform/Systems/TRSToLocalToWorldSystem.h>

#include <DustEngine/Transform/Systems/TRSToLocalToParentSystem.h>
#include <DustEngine/Transform/Components/LocalToParent.h>
#include <DustEngine/Transform/Components/LocalToWorld.h>
#include <DustEngine/Transform/Components/Parent.h>
#include <DustEngine/Transform/Components/Children.h>

using namespace Ubpa::DustEngine;
using namespace Ubpa::UECS;

void LocalToParentSystem::ChildLocalToWorld(const transformf& parent_l2w, Entity e) {
	transformf l2w;
	auto w = GetWorld();
	if (w->entityMngr.Have(e, CmptType::Of<LocalToWorld>) && w->entityMngr.Have(e, CmptType::Of<LocalToParent>)) {
		auto child_l2w = w->entityMngr.Get<LocalToWorld>(e);
		auto child_l2p = w->entityMngr.Get<LocalToParent>(e);
		l2w = parent_l2w * child_l2p->value;
		child_l2w->value = l2w;
	}
	else
		l2w = parent_l2w;

	if (w->entityMngr.Have(e, CmptType::Of<Children>)) {
		auto children = w->entityMngr.Get<Children>(e);
		for (const auto& child : children->value)
			ChildLocalToWorld(l2w, e);
	}
}

void LocalToParentSystem::OnUpdate(UECS::Schedule& schedule) {
	UECS::EntityFilter rootFilter{
		TypeList<>{},      // all
		TypeList<>{},      // any
		TypeList<Parent>{} // none
	};

	schedule.InsertNone(TRSToLocalToWorldSystem::SystemFuncName, UECS::CmptType::Of<Parent>);
	schedule.Register(
		[this](const LocalToWorld* l2w, const Children* children) {
			for (const auto& child : children->value)
				ChildLocalToWorld(l2w->value, child);
		},
		SystemFuncName,
		rootFilter
	);
	schedule.Order(TRSToLocalToParentSystem::SystemFuncName, SystemFuncName);
}
