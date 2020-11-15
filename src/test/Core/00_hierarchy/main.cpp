#include <Utopia/Core/Components/Components.h>
#include <Utopia/Core/Systems/Systems.h>

#include <iostream>

using namespace Ubpa::UECS;
using namespace Ubpa::Utopia;
using namespace Ubpa;
using namespace std;

struct PrintSystem {
	static void OnUpdate(Schedule& schedule) {
		schedule.RegisterEntityJob(
			[&](const LocalToWorld* l2w) {
				l2w->value.print();
			},
			"print",
			false
		);
	}
};

int main() {
	World w;

	w.entityMngr.cmptTraits.Register<
		Children,
		LocalToParent,
		LocalToWorld,
		Parent,
		Rotation,
		RotationEuler,
		Scale,
		Translation,
		WorldToLocal
	>();

	auto systemIDs = w.systemMngr.systemTraits.Register<
		PrintSystem,
		LocalToParentSystem,
		RotationEulerSystem,
		TRSToLocalToParentSystem,
		TRSToLocalToWorldSystem,
		WorldToLocalSystem
	>();
	for (auto idx : systemIDs)
		w.systemMngr.Activate(idx);

	auto [r_e, r_c, r_l2w, r_t] = w.entityMngr.Create<
		Children,
		LocalToWorld,
		Translation
	>();

	auto [c0_e, c0_p, c0_l2p, c0_l2w, c0_t] = w.entityMngr.Create<
		Parent,
		LocalToParent,
		LocalToWorld,
		Translation
	>();

	auto [c1_e, c1_p, c1_l2p, c1_l2w, c1_t] = w.entityMngr.Create<
		Parent,
		LocalToParent,
		LocalToWorld,
		Translation
	>();

	r_c->value = { c0_e , c1_e };
	r_t->value = { 0, 1, 0 };

	c0_p->value = r_e;
	c0_t->value = { 1,0,0 };

	c1_p->value = r_e;
	c1_t->value = { 0,0,1 };

	w.Update();

	cout << w.GenUpdateFrameGraph().Dump() << endl;
	cout << w.DumpUpdateJobGraph() << endl;
}
