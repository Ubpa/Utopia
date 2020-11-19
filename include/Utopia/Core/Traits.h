#pragma once

#include <UTemplate/Func.h>
#include <USRefl/USRefl.h>
#include <USTL/memory.h>
#include <UGM/UGM.h>

#include <vector>
#include <deque>
#include <map>
#include <unordered_map>
#include <list>
#include <set>
#include <unordered_set>
#include <array>
#include <tuple>
#include <forward_list>
#include <type_traits>

namespace Ubpa::Utopia {
	template<typename Void, typename T>
	struct HasDefinitionHelper : std::false_type {};
	template<typename T>
	struct HasDefinitionHelper<std::void_t<decltype(sizeof(T))>, T> : std::true_type {};
	template<typename T>
	struct HasDefinition : HasDefinitionHelper<void, T> {};

	template<typename T>
	struct HasTypeInfo : HasDefinition<Ubpa::USRefl::TypeInfo<T>> {};

	template<typename T>
	constexpr size_t GetID() noexcept { return TypeID<T>; }

	// =========================================================================================

	template<typename T> struct ArrayTraits {
		static constexpr bool isArray = false;
	};
	template<size_t N> struct ArrayTraitsBase {
		static constexpr bool isArray = true;
		static constexpr size_t size = N;
	};
	template<typename Arr>
	auto& ArrayTraits_Get(Arr& arr, size_t idx) noexcept {
		assert(idx < ArrayTraits<Arr>::size);
		return arr[idx];
	}
	template<typename Arr>
	const auto& ArrayTraits_Get(const Arr& arr, size_t idx) noexcept {
		return ArrayTraits_Get(const_cast<Arr&>(arr), idx);
	}
	template<typename Arr>
	using ArrayTraits_ValueType = std::decay_t<decltype(ArrayTraits_Get(std::declval<Arr>(), 0))>;
	template<typename Arr>
	auto ArrayTraits_Data(Arr& arr) noexcept {
		return arr.data();
	}
	template<typename Arr>
	void ArrayTraits_Set(Arr& arr, size_t idx, ArrayTraits_ValueType<Arr>&& value) {
		assert(idx < ArrayTraits<Arr>::size);
		arr[idx] = std::move(value);
	}

	template<typename T, size_t N> struct ArrayTraits<std::array<T, N>> : ArrayTraitsBase<N> {};
	template<typename T, size_t N> struct ArrayTraits<bbox<T, N>> : ArrayTraitsBase<2> {};
	template<typename T, size_t N> struct ArrayTraits<hvec<T, N>> : ArrayTraitsBase<N> {};
	template<typename T, size_t N> struct ArrayTraits<mat<T, N>> : ArrayTraitsBase<N> {};
	template<typename T, size_t N> struct ArrayTraits<point<T, N>> : ArrayTraitsBase<N> {};
	template<typename T, size_t N> struct ArrayTraits<scale<T, N>> : ArrayTraitsBase<N> {};
	template<typename T, size_t N> struct ArrayTraits<triangle<T, N>> : ArrayTraitsBase<3> {};
	template<typename T, size_t N> struct ArrayTraits<val<T, N>> : ArrayTraitsBase<N> {};
	template<typename T, size_t N> struct ArrayTraits<vec<T, N>> : ArrayTraitsBase<N> {};
	template<typename T>           struct ArrayTraits<euler<T>> : ArrayTraitsBase<3> {};
	template<typename T>           struct ArrayTraits<normal<T>> : ArrayTraitsBase<3> {};
	template<typename T>           struct ArrayTraits<quat<T>> : ArrayTraitsBase<4> {};
	template<typename T>           struct ArrayTraits<rgb<T>> : ArrayTraitsBase<3> {};
	template<typename T>           struct ArrayTraits<rgba<T>> : ArrayTraitsBase<4> {};
	template<typename T>           struct ArrayTraits<svec<T>> : ArrayTraitsBase<3> {};
	template<typename T>           struct ArrayTraits<transform<T>> : ArrayTraitsBase<4> {};
	template<typename T, size_t N> struct ArrayTraits<T[N]> : ArrayTraitsBase<N> {};

	// =========================================================================================

	template<typename T> struct OrderContainerTraits {
		static constexpr bool isOrderContainer = false;
		static constexpr bool isResizable = false;
	};
	template<bool IsResizable>
	struct OrderContainerTraitsBase {
		static constexpr bool isOrderContainer = true;
		static constexpr bool isResizable = IsResizable;
	};
	template<typename OrderContainer>
	auto OrderContainerTraits_Begin(const OrderContainer& container) noexcept {
		return container.begin();
	}
	template<typename OrderContainer>
	auto OrderContainerTraits_End(const OrderContainer& container) noexcept {
		return container.end();
	}
	template<typename OrderContainer>
	auto OrderContainerTraits_Begin(OrderContainer& container) noexcept {
		return container.begin();
	}
	template<typename OrderContainer>
	auto OrderContainerTraits_End(OrderContainer& container) noexcept {
		return container.end();
	}
	template<typename OrderContainer>
	using OrderContainerTraits_ValueType = std::decay_t<decltype(*OrderContainerTraits_Begin(std::declval<OrderContainer>()))>;
	template<typename OrderContainer>
	void OrderContainerTraits_Add(OrderContainer& container, OrderContainerTraits_ValueType<OrderContainer>&& value) {
		container.push_back(std::move(value));
	}
	template<typename OrderContainer>
	void OrderContainerTraits_Resize(OrderContainer& container, size_t size) {
		container.resize(size);
	}
	template<typename OrderContainer>
	size_t OrderContainerTraits_Size(OrderContainer& container) {
		return container.size();
	}
	template<typename OrderContainer>
	void OrderContainerTraits_PostProcess(const OrderContainer& container) noexcept {}
	template<typename T, typename Alloc>
	struct OrderContainerTraits<std::vector<T, Alloc>> : OrderContainerTraitsBase<true> {};
	template<typename T, typename Alloc>
	struct OrderContainerTraits<std::deque<T, Alloc>> : OrderContainerTraitsBase<true> {};
	template<typename T, typename Alloc>
	struct OrderContainerTraits<std::list<T, Alloc>> : OrderContainerTraitsBase<true> {};
	template<typename T, typename Alloc>
	struct OrderContainerTraits<std::forward_list<T, Alloc>> : OrderContainerTraitsBase<true> {};
	template<typename T, typename Pr, typename Alloc>
	struct OrderContainerTraits<std::set<T, Pr, Alloc>> : OrderContainerTraitsBase<false> {};
	template<typename T, typename Pr, typename Alloc>
	struct OrderContainerTraits<std::multiset<T, Pr, Alloc>> : OrderContainerTraitsBase<false> {};
	template<typename T, typename Hasher, typename KeyEq, typename Alloc>
	struct OrderContainerTraits<std::unordered_set<T, Hasher, KeyEq, Alloc>> : OrderContainerTraitsBase<false> {};
	template<typename T, typename Hasher, typename KeyEq, typename Alloc>
	struct OrderContainerTraits<std::unordered_multiset<T, Hasher, KeyEq, Alloc>> : OrderContainerTraitsBase<false> {};

	template<typename T, typename Alloc>
	void OrderContainerTraits_Add(std::forward_list<T, Alloc>& container, T&& value) {
		container.push_front(std::move(value));
	}

	template<typename T, typename Alloc>
	void OrderContainerTraits_PostProcess(std::forward_list<T, Alloc>& container) {
		container.reverse();
	}

	template<typename T, typename Alloc>
	size_t OrderContainerTraits_Size(std::forward_list<T, Alloc>& container) {
		return std::distance(
			OrderContainerTraits_Begin(container),
			OrderContainerTraits_End(container)
		);
	}

	template<typename T, typename Pr, typename Alloc>
	void OrderContainerTraits_Add(std::set<T, Pr, Alloc>& container, T&& value) {
		container.insert(std::move(value));
	}

	template<typename T, typename Pr, typename Alloc>
	void OrderContainerTraits_Add(std::multiset<T, Pr, Alloc>& container, T&& value) {
		container.insert(std::move(value));
	}

	template<typename T, typename Hasher, typename KeyEq, typename Alloc>
	void OrderContainerTraits_Add(std::unordered_set<T, Hasher, KeyEq, Alloc>& container, T&& value) {
		container.insert(std::move(value));
	}

	template<typename T, typename Hasher, typename KeyEq, typename Alloc>
	void OrderContainerTraits_Add(std::unordered_multiset<T, Hasher, KeyEq, Alloc>& container, T&& value) {
		container.insert(std::move(value));
	}

	// =========================================================================================

	template<typename T> struct MapTraits {
		static constexpr bool isMap = false;
	};
	struct MapTraitsBase {
		static constexpr bool isMap = true;
	};
	template<typename Map>
	auto MapTraits_Begin(const Map& m) noexcept {
		return m.begin();
	}
	template<typename Map>
	auto MapTraits_End(const Map& m) noexcept {
		return m.end();
	}
	template<typename Map>
	auto MapTraits_Begin(Map& m) noexcept {
		return m.begin();
	}
	template<typename Map>
	auto MapTraits_End(Map& m) noexcept {
		return m.end();
	}
	template<typename Iter>
	const auto& MapTraits_Iterator_Key(const Iter& iter) {
		return iter->first;
	}
	template<typename Iter>
	const auto& MapTraits_Iterator_Mapped(const Iter& iter) {
		return iter->second;
	}
	template<typename Map>
	using MapTraits_KeyType = std::decay_t<decltype(MapTraits_Iterator_Key(MapTraits_Begin(std::declval<Map>())))>;
	template<typename Map>
	using MapTraits_MappedType = std::decay_t<decltype(MapTraits_Iterator_Mapped(MapTraits_Begin(std::declval<Map>())))>;
	template<typename Map>
	auto MapTraits_Emplace(Map& m, MapTraits_KeyType<Map>&& key, MapTraits_MappedType<Map>&& mapped) {
		return m.emplace(std::move(key), std::move(mapped));
	}
	template<typename Key, typename Mapped, typename Pr, typename Alloc>
	struct MapTraits<std::map<Key, Mapped, Pr, Alloc>> : MapTraitsBase {};
	template<typename Key, typename Mapped, typename Pr, typename Alloc>
	struct MapTraits<std::multimap<Key, Mapped, Pr, Alloc>> : MapTraitsBase {};
	template<typename Key, typename Mapped, typename Hasher, typename KeyEq, typename Alloc>
	struct MapTraits<std::unordered_map<Key, Mapped, Hasher, KeyEq, Alloc>> : MapTraitsBase {};
	template<typename Key, typename Mapped, typename Hasher, typename KeyEq, typename Alloc>
	struct MapTraits<std::unordered_multimap<Key, Mapped, Hasher, KeyEq, Alloc>> : MapTraitsBase {};

	// =========================================================================================

	template<typename T> struct TupleTraits {
		static constexpr bool isTuple = false;
	};
	struct TupleTraitsBase {
		static constexpr bool isTuple = true;
	};
	template<typename... Ts>
	struct TupleTraits<std::tuple<Ts...>> : TupleTraitsBase {};
	template<typename T, typename U>
	struct TupleTraits<std::pair<T, U>> : TupleTraitsBase {};
}
