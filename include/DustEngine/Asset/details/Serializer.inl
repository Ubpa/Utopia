#pragma once

#include <UTemplate/Func.h>
#include <USRefl/USRefl.h>
#include "../AssetMngr.h"

#include <UGM/UGM.h>

namespace Ubpa::DustEngine::detail {
	template<typename Void, typename T>
	struct HasDefinitionHelper : std::false_type {};
	template<typename T>
	struct HasDefinitionHelper<std::void_t<decltype(sizeof(T))>, T> : std::true_type {};
	template<typename T>
	struct HasDefinition : HasDefinitionHelper<void, T> {};

	template<typename T>
	struct HasTypeInfo : HasDefinition<Ubpa::USRefl::TypeInfo<T>> {};

	// =========================================================================================

	template<typename T> struct ArrayTraits {
		static constexpr bool isArray = false;
	};
	template<size_t N> struct ArrayTraitsBase {
		static constexpr bool isArray = true;
		static constexpr size_t size = N;
	};
	template<typename Arr>
	const auto& ArrayTraits_Get(const Arr& arr, size_t idx) noexcept {
		assert(idx < ArrayTraits<Arr>::size);
		return arr[idx];
	}
	template<typename Arr>
	using ArrayTraits_ValueType = std::decay_t<decltype(ArrayTraits_Get(std::declval<Arr>(), 0))>;
	template<typename Arr>
	void ArrayTraits_Set(Arr& arr, size_t idx, ArrayTraits_ValueType<Arr>&& value) {
		assert(idx < ArrayTraits<Arr>::size);
		arr[idx] = std::move(value);
	}

	template<typename T, size_t N> struct ArrayTraits<std::array<T, N>> : ArrayTraitsBase<N> {};
	template<typename T, size_t N> struct ArrayTraits<bbox<T, N>>       : ArrayTraitsBase<2> {};
	template<typename T, size_t N> struct ArrayTraits<hvec<T, N>>       : ArrayTraitsBase<N> {};
	template<typename T, size_t N> struct ArrayTraits<mat<T, N>>        : ArrayTraitsBase<N> {};
	template<typename T, size_t N> struct ArrayTraits<point<T, N>>      : ArrayTraitsBase<N> {};
	template<typename T, size_t N> struct ArrayTraits<scale<T, N>>      : ArrayTraitsBase<N> {};
	template<typename T, size_t N> struct ArrayTraits<triangle<T, N>>   : ArrayTraitsBase<3> {};
	template<typename T, size_t N> struct ArrayTraits<val<T, N>>        : ArrayTraitsBase<N> {};
	template<typename T, size_t N> struct ArrayTraits<vec<T, N>>        : ArrayTraitsBase<N> {};
	template<typename T>           struct ArrayTraits<euler<T>>         : ArrayTraitsBase<3> {};
	template<typename T>           struct ArrayTraits<normal<T>>        : ArrayTraitsBase<3> {};
	template<typename T>           struct ArrayTraits<quat<T>>          : ArrayTraitsBase<4> {};
	template<typename T>           struct ArrayTraits<rgb<T>>           : ArrayTraitsBase<3> {};
	template<typename T>           struct ArrayTraits<rgba<T>>          : ArrayTraitsBase<4> {};
	template<typename T>           struct ArrayTraits<svec<T>>          : ArrayTraitsBase<3> {};
	template<typename T>           struct ArrayTraits<transform<T>>     : ArrayTraitsBase<4> {};

	// =========================================================================================

	template<typename T> struct VectorTraits {
		static constexpr bool isVector = false;
	};
	struct VectorTraitsBase {
		static constexpr bool isVector = true;
	};
	template<typename Vector>
	auto VectorTraits_Begin(const Vector& vec) noexcept {
		return vec.begin();
	}
	template<typename Vector>
	auto VectorTraits_End(const Vector& vec) noexcept {
		return vec.end();
	}
	template<typename Vector>
	using VectorTraits_ValueType = std::decay_t<decltype(*VectorTraits_Begin(std::declval<Vector>()))>;
	template<typename Vector>
	void VectorTraits_Add(Vector& vec, VectorTraits_ValueType<Vector>&& value) {
		vec.push_back(std::move(value));
	}
	template<typename Vector>
	void VectorTraits_PostProcess(const Vector& vec) noexcept {}
	template<typename T, typename Alloc>
	struct VectorTraits<std::vector<T, Alloc>> : VectorTraitsBase {};
	template<typename T, typename Alloc>
	struct VectorTraits<std::deque<T, Alloc>> : VectorTraitsBase {};
	template<typename T, typename Alloc>
	struct VectorTraits<std::list<T, Alloc>> : VectorTraitsBase {};
	template<typename T, typename Alloc>
	struct VectorTraits<std::forward_list<T, Alloc>> : VectorTraitsBase {};
	template<typename T, typename Pr, typename Alloc>
	struct VectorTraits<std::set<T, Pr, Alloc>> : VectorTraitsBase {};
	template<typename T, typename Pr, typename Alloc>
	struct VectorTraits<std::multiset<T, Pr, Alloc>> : VectorTraitsBase {};
	template<typename T, typename Hasher, typename KeyEq, typename Alloc>
	struct VectorTraits<std::unordered_set<T, Hasher, KeyEq, Alloc>> : VectorTraitsBase {};
	template<typename T, typename Hasher, typename KeyEq, typename Alloc>
	struct VectorTraits<std::unordered_multiset<T, Hasher, KeyEq, Alloc>> : VectorTraitsBase {};

	template<typename T, typename Alloc>
	void VectorTraits_Add(std::forward_list<T, Alloc>& container, T&& value) {
		container.push_front(std::move(value));
	}

	template<typename T, typename Alloc>
	void VectorTraits_PostProcess(std::forward_list<T, Alloc>& container) {
		container.reverse();
	}

	template<typename T, typename Pr, typename Alloc>
	void VectorTraits_Add(std::set<T, Pr, Alloc>& container, T&& value) {
		container.insert(std::move(value));
	}

	template<typename T, typename Pr, typename Alloc>
	void VectorTraits_Add(std::multiset<T, Pr, Alloc>& container, T&& value) {
		container.insert(std::move(value));
	}

	template<typename T, typename Hasher, typename KeyEq, typename Alloc>
	void VectorTraits_Add(std::unordered_set<T, Hasher, KeyEq, Alloc>& container, T&& value) {
		container.insert(std::move(value));
	}

	template<typename T, typename Hasher, typename KeyEq, typename Alloc>
	void VectorTraits_Add(std::unordered_multiset<T, Hasher, KeyEq, Alloc>& container, T&& value) {
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

	// =========================================================================================

	template<typename Value>
	void WriteVar(const Value& var, Serializer::SerializeContext ctx) {
		if constexpr (std::is_floating_point_v<Value>)
			ctx.writer->Double(static_cast<double>(var));
		else if constexpr (std::is_integral_v<Value>) {
			if constexpr (std::is_same_v<Value, bool>)
				ctx.writer->Bool(var);
			else {
				constexpr size_t size = sizeof(Value);
				if constexpr (std::is_unsigned_v<Value>) {
					if constexpr (size <= 4)
						ctx.writer->Uint(static_cast<std::uint32_t>(var));
					else
						ctx.writer->Uint64(static_cast<std::uint64_t>(var));
				}
				else {
					if constexpr (size <= 4)
						ctx.writer->Int(static_cast<std::uint32_t>(var));
					else
						ctx.writer->Int64(static_cast<std::uint64_t>(var));
				}
			}
		}
		else if constexpr (std::is_same_v<Value, std::string>)
			ctx.writer->String(var);
		else if constexpr (std::is_pointer_v<Value>) {
			if (var == nullptr)
				ctx.writer->Null();
			else {
				auto& assetMngr = AssetMngr::Instance();
				if (assetMngr.Contains(var))
					ctx.writer->String(assetMngr.AssetPathToGUID(assetMngr.GetAssetPath(var)).str());
				else {
					assert("not support" && false);
					ctx.writer->Null();
				}
			}
		}
		else if constexpr (std::is_same_v<Value, UECS::Entity>)
			ctx.writer->Uint64(var.Idx());
		else if constexpr (ArrayTraits<Value>::isArray) {
			ctx.writer->StartArray();
			for (size_t i = 0; i < ArrayTraits<Value>::size; i++)
				WriteVar(ArrayTraits_Get(var, i), ctx);
			ctx.writer->EndArray();
		}
		else if constexpr (VectorTraits<Value>::isVector) {
			ctx.writer->StartArray();
			auto iter_end = VectorTraits_End(var);
			for(auto iter = VectorTraits_Begin(var); iter != iter_end; iter++)
				WriteVar(*iter, ctx);
			ctx.writer->EndArray();
		}
		else if constexpr (MapTraits<Value>::isMap) {
			auto iter_end = MapTraits_End(var);
			if constexpr (std::is_same_v<std::string, MapTraits_KeyType<Value>>) {
				ctx.writer->StartObject();
				for (auto iter = MapTraits_Begin(var); iter != iter_end; ++iter) {
					const auto& key = MapTraits_Iterator_Key(iter);
					const auto& mapped = MapTraits_Iterator_Mapped(iter);
					ctx.writer->Key(key);
					WriteVar(mapped, ctx);
				}
				ctx.writer->EndObject();
			}
			else {
				ctx.writer->StartArray();
				for (auto iter = MapTraits_Begin(var); iter != iter_end; ++iter) {
					const auto& key = MapTraits_Iterator_Key(iter);
					const auto& mapped = MapTraits_Iterator_Mapped(iter);
					ctx.writer->StartObject();
					ctx.writer->Key("key");
					WriteVar(key, ctx);
					ctx.writer->Key("mapped");
					WriteVar(mapped, ctx);
					ctx.writer->EndObject();
				}
				ctx.writer->EndArray();
			}
		}
		else if constexpr (TupleTraits<Value>::isTuple) {
			ctx.writer->StartArray();
			std::apply([&](const auto& ... elements) {
				(WriteVar(elements, ctx), ...);
			}, var);
			ctx.writer->EndArray();
		}
		else if constexpr (HasTypeInfo<Value>::value) {
			ctx.writer->StartObject();
			Ubpa::USRefl::TypeInfo<Value>::ForEachVarOf(
				var,
				[&ctx](auto field, const auto& var) {
					ctx.writer->Key(field.name.data());
					detail::WriteVar(var, ctx);
				}
			);
			ctx.writer->EndObject();
		}
		else {
			if (ctx.fieldSerializer->IsRegistered(TypeID<Value>))
				ctx.fieldSerializer->Visit(TypeID<Value>, &var, ctx);
			else {
				assert("not support" && false);
				ctx.writer->String("__NOT_SUPPORT");
			}
		}
	}

	template<typename Value, typename JSONValue>
	Value ReadVar(const JSONValue& jsonValueField, Serializer::DeserializeContext ctx) {
		if constexpr (std::is_floating_point_v<Value>)
			return static_cast<Value>(jsonValueField.GetDouble());
		else if constexpr (std::is_integral_v<Value>) {
			if constexpr (std::is_same_v<Value, bool>)
				return jsonValueField.GetBool();
			else {
				constexpr size_t size = sizeof(Value);
				if constexpr (std::is_unsigned_v<Value>) {
					if constexpr (size <= 4)
						return static_cast<Value>(jsonValueField.GetUint());
					else
						return static_cast<Value>(jsonValueField.GetUint64());
				}
				else {
					if constexpr (size <= 4)
						return static_cast<Value>(jsonValueField.GetInt());
					else
						return static_cast<Value>(jsonValueField.GetInt64());
				}
			}
		}
		else if constexpr (std::is_same_v<Value, std::string>)
			return jsonValueField.GetString();
		else if constexpr (std::is_pointer_v<Value>) {
			if (jsonValueField.IsNull())
				return nullptr;
			else {
				using Asset = std::remove_const_t<std::remove_pointer_t<Value>>;
				std::string guid = jsonValueField.GetString();
				const auto& path = AssetMngr::Instance().GUIDToAssetPath(xg::Guid{ guid });
				return AssetMngr::Instance().LoadAsset<Asset>(path);
			}
		}
		else if constexpr (std::is_same_v<Value, UECS::Entity>) {
			auto index = jsonValueField.GetUint64();
			return ctx.entityIdxMap->find(index)->second;
		}
		else if constexpr (ArrayTraits<Value>::isArray) {
			const auto& arr = jsonValueField.GetArray();
			assert(ArrayTraits<Value>::size == arr.Size());
			Value rst;
			for (size_t i = 0; i < ArrayTraits<Value>::size; i++) {
				ArrayTraits_Set(
					rst, i,
					ReadVar<ArrayTraits_ValueType<Value>>(arr[static_cast<rapidjson::SizeType>(i)], ctx)
				);
			}
			return rst;
		}
		else if constexpr (VectorTraits<Value>::isVector) {
			const auto& arr = jsonValueField.GetArray();
			Value rst;
			for (size_t i = 0; i < arr.Size(); i++) {
				VectorTraits_Add(
					rst,
					ReadVar<VectorTraits_ValueType<Value>>(arr[static_cast<rapidjson::SizeType>(i)], ctx)
				);
			}
			VectorTraits_PostProcess(rst);
			return rst;
		}
		else if constexpr (MapTraits<Value>::isMap) {
			if constexpr (std::is_same_v<MapTraits_KeyType<Value>, std::string>) {
				const auto& m = jsonValueField.GetObject();
				Value rst;
				for (const auto& [val_key, val_mapped] : m) {
					MapTraits_Emplace(
						rst,
						MapTraits_KeyType<Value>{val_key.GetString()},
						ReadVar<MapTraits_MappedType<Value>>(val_mapped, ctx)
					);
				}
				return rst;
			}
			else {
				const auto& m = jsonValueField.GetArray();
				Value rst;
				for (const auto& val_pair : m) {
					const auto& pair = val_pair.GetObject();
					MapTraits_Emplace(
						rst,
						ReadVar<MapTraits_KeyType<Value>>(pair["key"], ctx),
						ReadVar<MapTraits_MappedType<Value>>(pair["mapped"], ctx)
					);
				}
				return rst;
			}
		}
		else if constexpr (TupleTraits<Value>::isTuple) {
			Value rst;
			std::apply([&](auto& ... elements) {
				const auto& arr = jsonValueField.GetArray();
				rapidjson::SizeType i = 0;
				((elements = ReadVar<std::remove_reference_t<decltype(elements)>>(arr[i++], ctx)),...);
			}, rst);
			return rst;
		}
		else if constexpr (HasTypeInfo<Value>::value) {
			const auto& jsonObject = jsonValueField.GetObject();
			Value rst;
			USRefl::TypeInfo<Value>::ForEachVarOf(
				rst,
				[&](auto field, auto& var) {
					const auto& jsonValueField = jsonObject[field.name.data()];
					var = detail::ReadVar<std::remove_reference_t<decltype(var)>>(
						jsonValueField,
						ctx
					);
				}
			);
			return rst;
		}
		else {
			if (ctx.fieldDeserializer->IsRegistered(TypeID<Value>)) {
				Value rst;
				ctx.fieldDeserializer->Visit(TypeID<Value>, &rst, jsonValueField, ctx);
				return rst;
			}
			else {
				assert("not support" && false);
				return {}; // not support
			}
		}
	};
}

namespace Ubpa::DustEngine {
	template<typename Func>
	void Serializer::RegisterComponentSerializeFunction(Func&& func) {
		using ArgList = FuncTraits_ArgList<Func>;
		static_assert(Length_v<ArgList> == 2);
		static_assert(std::is_same_v<At_t<ArgList, 1>, SerializeContext>);
		using ConstCmptPtr = At_t<ArgList, 0>;
		static_assert(std::is_pointer_v<ConstCmptPtr>);
		using ConstCmpt = std::remove_pointer_t<ConstCmptPtr>;
		static_assert(std::is_const_v<ConstCmpt>);
		using Cmpt = std::remove_const_t<ConstCmpt>;
		RegisterComponentSerializeFunction(
			UECS::CmptType::Of<Cmpt>,
			[f = std::forward<Func>(func)](const void* p, SerializeContext ctx) {
				f(reinterpret_cast<const Cmpt*>(p), ctx);
			}
		);
	}

	template<typename Func>
	void Serializer::RegisterComponentDeserializeFunction(Func&& func) {
		using ArgList = FuncTraits_ArgList<Func>;
		static_assert(Length_v<ArgList> == 3);
		static_assert(std::is_same_v<At_t<ArgList, 1>, const rapidjson::Value&>);
		static_assert(std::is_same_v<At_t<ArgList, 2>, DeserializeContext>);
		using CmptPtr = At_t<ArgList, 0>;
		static_assert(std::is_pointer_v<CmptPtr>);
		using Cmpt = std::remove_pointer_t<CmptPtr>;
		static_assert(!std::is_const_v<Cmpt>);
		RegisterComponentDeserializeFunction(
			UECS::CmptType::Of<Cmpt>,
			[f = std::forward<Func>(func)](void* p, const rapidjson::Value& jsonValueCmpt, DeserializeContext ctx) {
				f(reinterpret_cast<Cmpt*>(p), jsonValueCmpt, ctx);
			}
		);
	}

	template<typename Func>
	void Serializer::RegisterUserTypeSerializeFunction(Func&& func) {
		using ArgList = FuncTraits_ArgList<Func>;
		static_assert(Length_v<ArgList> == 2);
		static_assert(std::is_same_v<At_t<ArgList, 1>, SerializeContext>);
		using ConstUserTypePtr = At_t<ArgList, 0>;
		static_assert(std::is_pointer_v<ConstUserTypePtr>);
		using ConstUserType = std::remove_pointer_t<ConstUserTypePtr>;
		static_assert(std::is_const_v<ConstUserType>);
		using UserType = std::remove_const_t<ConstUserType>;
		RegisterUserTypeSerializeFunction(
			TypeID<UserType>,
			[f = std::forward<Func>(func)](const void* p, SerializeContext ctx) {
				f(reinterpret_cast<const UserType*>(p), ctx);
			}
		);
	}

	template<typename Func>
	void Serializer::RegisterUserTypeDeserializeFunction(Func&& func) {
		using ArgList = FuncTraits_ArgList<Func>;
		static_assert(Length_v<ArgList> == 3);
		static_assert(std::is_same_v<At_t<ArgList, 1>, const rapidjson::Value&>);
		static_assert(std::is_same_v<At_t<ArgList, 2>, DeserializeContext>);
		using UserTypePtr = At_t<ArgList, 0>;
		static_assert(std::is_pointer_v<UserTypePtr>);
		using UserType = std::remove_pointer_t<UserTypePtr>;
		static_assert(!std::is_const_v<UserType>);
		RegisterUserTypeDeserializeFunction(
			TypeID<UserType>,
			[f = std::forward<Func>(func)](void* p, const rapidjson::Value& jsonValueCmpt, DeserializeContext ctx) {
				f(reinterpret_cast<UserType*>(p), jsonValueCmpt, ctx);
			}
		);
	}

	template<typename Cmpt>
	void Serializer::RegisterComponentSerializeFunction() {
		RegisterComponentSerializeFunction(
			[](const Cmpt* cmpt, SerializeContext ctx) {
			USRefl::TypeInfo<Cmpt>::ForEachVarOf(
				*cmpt,
				[&ctx](auto field, const auto& var) {
					ctx.writer->Key(field.name.data());
					detail::WriteVar(var, ctx);
				}
			);
		});
	}

	template<typename Cmpt>
	void Serializer::RegisterComponentDeserializeFunction() {
		RegisterComponentDeserializeFunction(
			[](Cmpt* cmpt, const rapidjson::Value& jsonValueCmpt, DeserializeContext ctx) {
				const auto& jsonObjectCmpt = jsonValueCmpt.GetObject();
				USRefl::TypeInfo<Cmpt>::ForEachVarOf(
					*cmpt,
					[&](auto field, auto& var) {
						const auto& jsonValueCmptField = jsonObjectCmpt[field.name.data()];
						var = detail::ReadVar<std::remove_reference_t<decltype(var)>>(
							jsonValueCmptField,
							ctx
						);
					}
				);
			}
		);
	}
}
