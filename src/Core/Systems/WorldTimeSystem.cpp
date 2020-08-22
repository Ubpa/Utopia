#include <DustEngine/Core/Systems/WorldTimeSystem.h>

#include <DustEngine/Core/Components/WorldTime.h>

#include <DustEngine/Core/GameTimer.h>

using namespace Ubpa::DustEngine;
using namespace Ubpa::UECS;

void WorldTimeSystem::OnUpdate(Schedule& schedule) {
	schedule.RegisterJob([](Singleton<WorldTime> time) {
		time->elapsedTime = GameTimer::Instance().TotalTime();
		time->deltaTime = GameTimer::Instance().DeltaTime();
	}, SystemFuncName);
}
