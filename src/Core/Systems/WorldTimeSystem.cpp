#include <Utopia/Core/Systems/WorldTimeSystem.h>

#include <Utopia/Core/Components/WorldTime.h>

#include <Utopia/Core/GameTimer.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UECS;

void WorldTimeSystem::OnUpdate(Schedule& schedule) {
	schedule.RegisterJob([](Singleton<WorldTime> time) {
		time->elapsedTime = GameTimer::Instance().TotalTime();
		time->deltaTime = GameTimer::Instance().DeltaTime();
	}, SystemFuncName);
}
