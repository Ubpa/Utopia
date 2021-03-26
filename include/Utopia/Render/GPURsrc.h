#pragma once

#include <atomic>

namespace Ubpa::Utopia {
	class GPURsrcMngrDX12;

	class GPURsrc {
	public:
		GPURsrc& operator=(const GPURsrc&) noexcept {
			dirty = true;
			return *this;
		}

		GPURsrc& operator=(GPURsrc&& rhs) noexcept {
			id = rhs.id;
			dirty = rhs.dirty;
			rhs.id = static_cast<size_t>(-1);
			rhs.dirty = true;
			return *this;
		}

		size_t GetInstanceID() const noexcept { return id; }
		bool Valid() const noexcept { return id != static_cast<std::size_t>(-1); }

		bool IsDirty() const noexcept { return dirty; }
		void SetDirty() noexcept { dirty = true; }
		void SetClean() noexcept { dirty = false; }

		bool IsEditable() const noexcept { return editable; }
		void SetEditable() noexcept { editable = true; }
		void SetReadOnly() noexcept { editable = false; }

	protected:
		GPURsrc() noexcept : id{ curID++ } {}

		GPURsrc(const GPURsrc&) noexcept : id{ curID++ } {}

		GPURsrc(GPURsrc&& rhs) noexcept : id{ rhs.id } {
			rhs.id = static_cast<size_t>(-1);
			rhs.dirty = true;
		}

		virtual ~GPURsrc() = default;

	private:
		size_t id;
		bool dirty{ true };
		bool editable{ true };
		inline static std::atomic<size_t> curID{ 0 };
	};
}
