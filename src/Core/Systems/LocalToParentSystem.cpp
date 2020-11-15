#include <Utopia/Core/Systems/LocalToParentSystem.h>
#include <Utopia/Core/Systems/TRSToLocalToWorldSystem.h>

#include <Utopia/Core/Systems/TRSToLocalToParentSystem.h>
#include <Utopia/Core/Components/LocalToParent.h>
#include <Utopia/Core/Components/LocalToWorld.h>
#include <Utopia/Core/Components/Parent.h>
#include <Utopia/Core/Components/Children.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UECS;

void LocalToParentSystem::ChildLocalToWorld(World* w, const transformf& parent_l2w, Entity e) {
	transformf l2w;
	if (w->entityMngr.Have<LocalToWorld>(e) && w->entityMngr.Have<LocalToParent>(e)) {
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
			ChildLocalToWorld(w, l2w, child);
	}
}

void LocalToParentSystem::OnUpdate(Schedule& schedule) {
	ArchetypeFilter rootFilter;
	rootFilter.none = { CmptType::Of<Parent> };
	RandomAccessor randomAccessor;
	randomAccessor.types = {
		CmptAccessType::Of<Write<LocalToWorld>>,
		CmptAccessType::Of<Latest<Children>>,
		CmptAccessType::Of<Latest<LocalToParent>>,
	};
	schedule.InsertNone(TRSToLocalToWorldSystem::SystemFuncName, CmptType::Of<Parent>);
	schedule.RegisterEntityJob(
		[](World* w, const LocalToWorld* l2w, const Children* children) {
			for (const auto& child : children->value)
				ChildLocalToWorld(w, l2w->value, child);
		},
		SystemFuncName,
		true,
		rootFilter,
		{},
		{},
		randomAccessor
	);
	schedule.Order(TRSToLocalToWorldSystem::SystemFuncName, LocalToParentSystem::SystemFuncName);
}
