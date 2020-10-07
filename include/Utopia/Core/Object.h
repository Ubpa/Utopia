#pragma once

#include <atomic>

namespace Ubpa::Utopia {
	class Object {
	public:
		Object() noexcept : id{ curID++ } {}
		virtual ~Object() noexcept {}
		size_t GetInstanceID() const noexcept { return id; }
	private:
		const size_t id;
		inline static std::atomic<size_t> curID{ 0 };
	};
}

#include "details/Object_AutoRefl.inl"
