#include <Utopia/Core/Systems/RotationEulerSystem.h>

#include <Utopia/Core/Components/Rotation.h>
#include <Utopia/Core/Components/RotationEuler.h>

using namespace Ubpa::Utopia;

void RotationEulerSystem::OnUpdate(UECS::Schedule& schedule) {
	schedule.RegisterEntityJob([](Rotation* rot, const RotationEuler* rot_euler) {
		rot->value = rot_euler->value.to_quat();
	}, SystemFuncName);
}
