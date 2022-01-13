#include <Utopia/Core/Systems/PrevLocalToWorldSystem.h>

#include <Utopia/Core/Components/PrevLocalToWorld.h>
#include <Utopia/Core/Components/LocalToWorld.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UECS;

void PrevLocalToWorldSystem::OnUpdate(Schedule& schedule) {
	schedule.RegisterEntityJob(
		[](LastFrame<LocalToWorld> L2W, Write<PrevLocalToWorld> prevL2W) {
			prevL2W->value = L2W->value;
		},
		SystemFuncName,
		Schedule::EntityJobConfig{
			.changeFilter = {
				.types = {
					TypeID_of<LocalToWorld>
				}
			}
		});
}
