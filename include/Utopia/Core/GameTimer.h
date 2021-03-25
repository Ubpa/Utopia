#pragma once

#include <cstdint>

namespace Ubpa::Utopia {
	class GameTimer {
	public:
		static GameTimer& Instance() noexcept {
			static GameTimer instance;
			return instance;
		}

		float TotalTime()const; // in seconds
		float DeltaTime()const; // in seconds

		void Reset(); // Call before message loop.
		void Start(); // Call when unpaused.
		void Stop();  // Call when paused.
		void Tick();  // Call every frame.

	private:
		GameTimer();

		double mSecondsPerCount;
		double mDeltaTime;

		std::int64_t mBaseTime;
		std::int64_t mPausedTime;
		std::int64_t mStopTime;
		std::int64_t mPrevTime;
		std::int64_t mCurrTime;

		bool mStopped;
	};
}
