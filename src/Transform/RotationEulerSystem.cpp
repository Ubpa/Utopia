#include <DustEngine/Transform/Systems/RotationEulerSystem.h>

#include <DustEngine/Transform/Components/Rotation.h>
#include <DustEngine/Transform/Components/RotationEuler.h>

using namespace Ubpa::DustEngine;

void RotationEulerSystem::OnUpdate(UECS::Schedule& schedule) {
	schedule.Register([](Rotation* rot, const RotationEuler* rot_euler) {
			rot->value = rot_euler->value.to_quat();
		}, SystemFuncName);
}
