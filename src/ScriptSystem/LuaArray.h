#pragma once

#include <vector>
#include <UContainer/Span.h>

namespace Ubpa::Utopia {
	template<typename T>
	class LuaArray {
	public:
		void PushBack(T val) { elems.push_back(val); }
		T* Data() { return elems.data(); }
		size_t Size() const { return elems.size(); }
		Span<const T> ToConstSpan() const { return elems; }
	private:
		std::vector<T> elems;
	};
}

#include "detail/LuaArray_AutoRefl.inl"
