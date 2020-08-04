#pragma once

#include <string_view>

namespace Ubpa::DustEngine {
	// Get/Set + Bool / (U)Int[8|16|32|64] / Float / Double / String / Pointer
	struct LuaBuffer {
		void* ptr;
		uint64_t size; // uint64_t

		LuaBuffer() : ptr{ nullptr }, size{ static_cast<uint64_t>(-1) } {}
		LuaBuffer(void* ptr, uint64_t size) : ptr{ ptr }, size{ size } {}

		void* GetPointer(size_t offset) const { return Get<void*>(offset); }
		void SetPointer(size_t offset, void* p) { Set(offset, p); }

		LuaBuffer GetBuffer(size_t offset) const { return Get<LuaBuffer>(offset); }
		void SetBuffer(size_t offset, LuaBuffer buffer) { Set(offset, buffer); }

		bool GetBool(size_t offset) const { return Get<bool>(offset); }
		void SetBool(size_t offset, bool value) { Set(offset, value); }

		int8_t GetInt8(size_t offset) const { return Get<int8_t>(offset); }
		void SetInt8(size_t offset, int8_t value) { Set(offset, value); }

		int16_t GetInt16(size_t offset) const { return Get<int16_t>(offset); }
		void SetInt16(size_t offset, int16_t value) { Set(offset, value); }

		int32_t GetInt32(size_t offset) const { return Get<int32_t>(offset); }
		void SetInt32(size_t offset, int32_t value) { Set(offset, value); }

		int64_t GetInt64(size_t offset) const { return Get<int64_t>(offset); }
		void SetInt64(size_t offset, int64_t value) { Set(offset, value); }

		uint8_t GetUInt8(size_t offset) const { return Get<uint8_t>(offset); }
		void SetUInt8(size_t offset, uint8_t value) { Set(offset, value); }

		uint16_t GetUInt16(size_t offset) const { return Get<uint16_t>(offset); }
		void SetUInt16(size_t offset, uint16_t value) { Set(offset, value); }

		uint32_t GetUInt32(size_t offset) const { return Get<uint32_t>(offset); }
		void SetUInt32(size_t offset, uint32_t value) { Set(offset, value); }

		uint64_t GetUInt64(size_t offset) const { return Get<uint64_t>(offset); }
		void SetUInt64(size_t offset, uint64_t value) { Set(offset, value); }

		float GetFloat(size_t offset) const { return Get<float>(offset); }
		void SetFloat(size_t offset, float value) { Set(offset, value); }

		double GetDouble(size_t offset) const { return Get<double>(offset); }
		void SetDouble(size_t offset, double value) { Set(offset, value); }

		std::string_view GetCString(size_t offset) const { return Offset<char>(offset); }
		void SetCString(size_t offset, const char* value) { strcpy(Offset<char>(offset), value); }

	private:
		template<typename T = void>
		T* Offset(size_t offset) const {
			return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(ptr)+ offset);
		}

		template<typename T>
		T Get(size_t offset) const {
			return *Offset<T>(offset);
		}

		template<typename T>
		void Set(size_t offset, T value) {
			*Offset<T>(offset) = value;
		}
	};
}

#include "detail/LuaBuffer_AutoRefl.inl"
