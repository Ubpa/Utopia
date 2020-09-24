#include <DustEngine/Core/Systems/RotationEulerSystem.h>

#include <DustEngine/Core/Components/Rotation.h>
#include <DustEngine/Core/Components/RotationEuler.h>

using namespace Ubpa::DustEngine;

void RotationEulerSystem::OnUpdate(UECS::Schedule& schedule) {
	schedule.RegisterEntityJob([](Rotation* rot, const RotationEuler* rot_euler) {
		rot->value = rot_euler->value.to_quat();
	}, SystemFuncName);
}
