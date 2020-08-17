#pragma once

#include <atomic>

namespace Ubpa::DustEngine {
	class Object {
	public:
		Object() : id{ curID++ } {}
		size_t GetInstanceID() const noexcept { return id; }
	private:
		const size_t id;
		inline static std::atomic<size_t> curID{ 0 };
	};
}
