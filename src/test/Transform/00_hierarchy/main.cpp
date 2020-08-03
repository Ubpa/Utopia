#include <DustEngine/Transform/Transform.h>

#include <iostream>

using namespace Ubpa::UECS;
using namespace Ubpa::DustEngine;
using namespace Ubpa;
using namespace std;

class PrintSystem : public System {
public:
	using System::System;
	mutex m;

	virtual void OnUpdate(Schedule& schedule) override {
		schedule.Register([&](const LocalToWorld* l2w) {
			m.lock();
			l2w->value.print();
			m.unlock();
		}, "print");
	}
};

int main() {
	RTDCmptTraits::Instance().Register<
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

	World w;

	w.systemMngr.Register<
		PrintSystem,
		LocalToParentSystem,
		RotationEulerSystem,
		TRSToLocalToParentSystem,
		TRSToLocalToWorldSystem,
		WorldToLocalSystem
	>();

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
}
