#include <Utopia/Core/Systems/RoamerSystem.h>

#include <Utopia/Core/Components/Roamer.h>
#include <Utopia/Core/Components/Input.h>
#include <Utopia/Core/Components/WorldTime.h>
#include <Utopia/Core/Components/Translation.h>
#include <Utopia/Core/Components/Rotation.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UECS;

void RoamerSystem::OnUpdate(Schedule& schedule) {
	schedule.RegisterEntityJob(
		[](
			const Roamer* roamer, Latest<Singleton<Input>> input, Latest<Singleton<WorldTime>> time,
			Translation* translation, Rotation* rotation
		) {
			if (!input->MouseDown[1] || !input->MouseInDisplay)
				return;

			auto forward = (roamer->reverseFrontBack ? -1.f : 1.f) * (rotation->value * vecf3{ 0,0,1 });
			auto right = rotation->value * vecf3{ (roamer->reverseLeftRight ? 1.f : -1.f),0,0 };
			vecf3 worldUp{ 0,(roamer->reverseUpDown ? -1.f : 1.f),0 };

			float dt = time->deltaTime;
			float dx = input->MouseDelta[0];
			float dy = input->MouseDelta[1];

			vecf3 move{ 0.f };
			if (input->KeysDown['W'])
				move += roamer->moveSpeed * dt * forward;
			if (input->KeysDown['S'])
				move -= roamer->moveSpeed * dt * forward;
			if (input->KeysDown['A'])
				move -= roamer->moveSpeed * dt * right;
			if (input->KeysDown['D'])
				move += roamer->moveSpeed * dt * right;
			if (input->KeysDown['Q'])
				move += roamer->moveSpeed * dt * worldUp;
			if (input->KeysDown['E'])
				move -= roamer->moveSpeed * dt * worldUp;
			translation->value += move;

			// right button
			if (input->MouseDown[1]) {
				quatf rx{ worldUp, -roamer->rotateSpeed * dt * dx };
				quatf ry{ right, -roamer->rotateSpeed * dt * dy };
				rotation->value = rx * ry * rotation->value;
			}
		},
		SystemFuncName
	);
}
