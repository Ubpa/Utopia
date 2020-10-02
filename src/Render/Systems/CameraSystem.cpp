#include <Utopia/Render/Systems/CameraSystem.h>

#include <Utopia/Render/Components/Camera.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UECS;

void CameraSystem::OnUpdate(Schedule& schedule) {
	schedule.RegisterEntityJob(
		[](Camera* camera) {
			camera->prjectionMatrix = transformf::perspective(
				to_radian(camera->fov),
				camera->aspect,
				camera->clippingPlaneMin,
				camera->clippingPlaneMax,
				0.f
			);
		},
		SystemFuncName
	);
}
