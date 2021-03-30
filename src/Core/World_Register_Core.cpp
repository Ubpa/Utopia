#include <Utopia/Core/Register_Core.h>

#include <Utopia/Core/Components/Components.h>
#include <Utopia/Core/Systems/Systems.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UECS;

void Ubpa::Utopia::World_Register_Core(World* w) {
	w->entityMngr.cmptTraits.Register<
		Children,
		Input,
		LocalToParent,
		LocalToWorld,
		Name,
		Parent,
		Roamer,
		Rotation,
		RotationEuler,
		Scale,
		NonUniformScale,
		Translation,
		WorldTime,
		WorldToLocal
	>();

	w->systemMngr.systemTraits.Register<
		InputSystem,
		LocalToParentSystem,
		RoamerSystem,
		RotationEulerSystem,
		TRSToLocalToParentSystem,
		TRSToLocalToWorldSystem,
		WorldToLocalSystem,
		WorldTimeSystem
	>();
}
