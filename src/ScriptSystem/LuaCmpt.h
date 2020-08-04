#pragma once

#include <UECS/CmptPtr.h>
#include <ULuaPP/ULuaPP.h>

namespace Ubpa::DustEngine {
	class LuaCmpt {
	public:
		LuaCmpt(UECS::CmptPtr ptr) : ptr{ ptr } {}
		LuaCmpt(UECS::CmptType type, void* ptr) : ptr{ UECS::CmptPtr{type, ptr} } {}

		UECS::CmptPtr GetCmptPtr() const noexcept { return ptr; }

		void SetZero();

		void MemCpy(void* src);

		void MemCpy(void* src, size_t offset, size_t size);

		// Get/Set + Bool / (U)Int[8|16|32|64] / Float / Double / String
		// DefaultConstruct / CopyConstruct / Destruct + Table

		bool GetBool(size_t offset) { return Get<bool>(offset); }
		void SetBool(size_t offset, bool value) { Set(offset, value); }

		int8_t GetInt8(size_t offset) { return Get<int8_t>(offset); }
		void SetInt8(size_t offset, int8_t value) { Set(offset, value); }

		int16_t GetInt16(size_t offset) { return Get<int16_t>(offset); }
		void SetInt16(size_t offset, int16_t value) { Set(offset, value); }

		int32_t GetInt32(size_t offset) { return Get<int32_t>(offset); }
		void SetInt32(size_t offset, int32_t value) { Set(offset, value); }

		int64_t GetInt64(size_t offset) { return Get<int64_t>(offset); }
		void SetInt64(size_t offset, int64_t value) { Set(offset, value); }

		uint8_t GetUInt8(size_t offset) { return Get<uint8_t>(offset); }
		void SetUInt8(size_t offset, uint8_t value) { Set(offset, value); }

		uint16_t GetUInt16(size_t offset) { return Get<uint16_t>(offset); }
		void SetUInt16(size_t offset, uint16_t value) { Set(offset, value); }

		uint32_t GetUInt32(size_t offset) { return Get<uint32_t>(offset); }
		void SetUInt32(size_t offset, uint32_t value) { Set(offset, value); }

		uint64_t GetUInt64(size_t offset) { return Get<uint64_t>(offset); }
		void SetUInt64(size_t offset, uint64_t value) { Set(offset, value); }

		float GetFloat(size_t offset) { return Get<float>(offset); }
		void SetFloat(size_t offset, float value) { Set(offset, value); }

		double GetDouble(size_t offset) { return Get<double>(offset); }
		void SetDouble(size_t offset, double value) { Set(offset, value); }

		std::string_view GetString(size_t offset, size_t size) { return { Offset<char>(offset),size }; }
		void SetString(size_t offset, std::string_view value) { memcpy(Offset(offset), value.data(), value.size()); }

		sol::table* DefaultConstructTable(size_t offset) {
			return DefaultConstruct<sol::table>(offset);
		}

		sol::table* CopyConstructTable(LuaCmpt src, size_t offset) {
			return CopyConstruct<sol::table>(src, offset);
		}

		sol::table* MoveConstructTable(LuaCmpt src, size_t offset) {
			return MoveConstruct<sol::table>(src, offset);
		}

		sol::table& MoveAssignTable(LuaCmpt src, size_t offset) {
			return MoveAssign<sol::table>(src, offset);
		}

		void DestructTable(size_t offset) {
			assert(ptr.Type().GetAccessMode() == UECS::AccessMode::WRITE);
			auto t = Offset<sol::table>(offset);
			if(t->valid())
				*t = nullptr;
		}

		sol::table GetTable(size_t offset) { return Get<sol::table>(offset); }
		void SetTable(size_t offset, sol::table value) { Set(offset, value); }

	private:
		template<typename T = void>
		T* Offset(size_t offset) {
			return reinterpret_cast<T*>(ptr.As<uint8_t>() + offset);
		}

		template<typename T>
		T* DefaultConstruct(size_t offset) {
			assert(ptr.Type().GetAccessMode() == UECS::AccessMode::WRITE);
			return new(Offset(offset))T;
		}

		template<typename T>
		T* CopyConstruct(LuaCmpt src, size_t offset) {
			assert(ptr.Type().GetAccessMode() == UECS::AccessMode::WRITE);
			assert(ptr.Type() == src.ptr.Type());
			return new(Offset(offset))T{ src.Get<T>(offset) };
		}

		template<typename T>
		T* MoveConstruct(LuaCmpt src, size_t offset) {
			assert(ptr.Type().GetAccessMode() == UECS::AccessMode::WRITE);
			assert(ptr.Type() == src.ptr.Type());
			return new(Offset(offset))T{ std::move(src.Get<T>(offset)) };
		}

		template<typename T>
		T& MoveAssign(LuaCmpt src, size_t offset) {
			assert(ptr.Type().GetAccessMode() == UECS::AccessMode::WRITE);
			assert(ptr.Type() == src.ptr.Type());
			return *Offset<T>(offset) = std::move(src.Get<T>(offset));
		}

		template<typename T>
		void Destruct(size_t offset) {
			assert(ptr.Type().GetAccessMode() == UECS::AccessMode::WRITE);
			Offset<T>(offset)->~T();
		}

		template<typename T>
		T& Get(size_t offset) {
			return *Offset<T>(offset);
		}

		template<typename T>
		void Set(size_t offset, T value) {
			assert(ptr.Type().GetAccessMode() == UECS::AccessMode::WRITE);
			*Offset<T>(offset) = value;
		}

		UECS::CmptPtr ptr;
	};
}

#include "detail/LuaCmpt_AutoRefl.inl"
