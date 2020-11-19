#pragma once

#include "../../Core/Traits.h"
#include "../AssetMngr.h"
#include "../../Core/Object.h"

#include <variant>

namespace Ubpa::Utopia::detail {
	template<typename Value>
	void WriteVar(const Value& var, Serializer::SerializeContext& ctx);

	template<typename UserType>
	void WriteUserType(const UserType* obj, Serializer::SerializeContext& ctx) {
		if constexpr (HasTypeInfo<UserType>::value) {
			ctx.writer.StartObject();
			Ubpa::USRefl::TypeInfo<UserType>::ForEachVarOf(
				*obj,
				[&ctx](auto field, const auto& var) {
					ctx.writer.Key(field.name.data());
					detail::WriteVar(var, ctx);
				}
			);
			ctx.writer.EndObject();
		}
		else {
			if (ctx.serializer.IsRegistered(GetID<UserType>()))
				ctx.serializer.Visit(GetID<UserType>(), obj, ctx);
			else {
				assert("not support" && false);
				ctx.writer.String(Serializer::Key::NOT_SUPPORT);
			}
		}
	}

	template<size_t Idx, typename Variant>
	bool WriteVariantAt(const Variant& var, size_t idx, Serializer::SerializeContext& ctx) {
		if (idx != Idx)
			return false;

		ctx.writer.StartObject();
		ctx.writer.Key(Serializer::Key::INDEX);
		ctx.writer.Uint64(var.index());
		ctx.writer.Key(Serializer::Key::CONTENT);
		WriteVar(std::get<std::variant_alternative_t<Idx, Variant>>(var), ctx);
		ctx.writer.EndObject();

		return true;
	}

	// TODO : stop
	template<typename Variant, size_t... Ns>
	void WriteVariant(const Variant& var, std::index_sequence<Ns...>, Serializer::SerializeContext& ctx) {
		(WriteVariantAt<Ns>(var, var.index(), ctx), ...);
	}

	template<typename Value>
	void WriteVar(const Value& var, Serializer::SerializeContext& ctx) {
		if constexpr (std::is_floating_point_v<Value>)
			ctx.writer.Double(static_cast<double>(var));
		else if constexpr (std::is_enum_v<Value>)
			WriteVar(static_cast<std::underlying_type_t<Value>>(var), ctx);
		else if constexpr (std::is_integral_v<Value>) {
			if constexpr (std::is_same_v<Value, bool>)
				ctx.writer.Bool(var);
			else {
				constexpr size_t size = sizeof(Value);
				if constexpr (std::is_unsigned_v<Value>) {
					if constexpr (size <= sizeof(unsigned int))
						ctx.writer.Uint(static_cast<std::uint32_t>(var));
					else
						ctx.writer.Uint64(static_cast<std::uint64_t>(var));
				}
				else {
					if constexpr (size <= sizeof(int))
						ctx.writer.Int(static_cast<std::int32_t>(var));
					else
						ctx.writer.Int64(static_cast<std::int64_t>(var));
				}
			}
		}
		else if constexpr (std::is_same_v<Value, std::string>)
			ctx.writer.String(var);
		else if constexpr (std::is_pointer_v<Value>) {
			if (var == nullptr)
				ctx.writer.Null();
			else {
				assert("not support" && false);
				ctx.writer.Null();
			}
		}
		else if constexpr (is_instance_of_v<Value, std::shared_ptr> || is_instance_of_v<Value, USTL::shared_object>) {
			using Element = typename Value::element_type;
			if (var == nullptr)
				ctx.writer.Null();
			else {
				if constexpr (std::is_base_of_v<Object, Element>) {
					auto& assetMngr = AssetMngr::Instance();
					const auto& path = assetMngr.GetAssetPath(*var);
					if (path.empty()) {
						ctx.writer.Null();
						return;
					}

					ctx.writer.String(assetMngr.AssetPathToGUID(path).str());
				}
				else {
					assert("not support" && false);
					ctx.writer.Null();
				}
			}
		}
		else if constexpr (std::is_same_v<Value, UECS::Entity>)
			ctx.writer.Uint64(var.Idx());
		else if constexpr (ArrayTraits<Value>::isArray) {
			ctx.writer.StartArray();
			for (size_t i = 0; i < ArrayTraits<Value>::size; i++)
				WriteVar(ArrayTraits_Get(var, i), ctx);
			ctx.writer.EndArray();
		}
		else if constexpr (OrderContainerTraits<Value>::isOrderContainer) {
			ctx.writer.StartArray();
			auto iter_end = OrderContainerTraits_End(var);
			for (auto iter = OrderContainerTraits_Begin(var); iter != iter_end; iter++)
				WriteVar(*iter, ctx);
			ctx.writer.EndArray();
		}
		else if constexpr (MapTraits<Value>::isMap) {
			auto iter_end = MapTraits_End(var);
			if constexpr (std::is_same_v<std::string, MapTraits_KeyType<Value>>) {
				ctx.writer.StartObject();
				for (auto iter = MapTraits_Begin(var); iter != iter_end; ++iter) {
					const auto& key = MapTraits_Iterator_Key(iter);
					const auto& mapped = MapTraits_Iterator_Mapped(iter);
					ctx.writer.Key(key);
					WriteVar(mapped, ctx);
				}
				ctx.writer.EndObject();
			}
			else {
				ctx.writer.StartArray();
				for (auto iter = MapTraits_Begin(var); iter != iter_end; ++iter) {
					const auto& key = MapTraits_Iterator_Key(iter);
					const auto& mapped = MapTraits_Iterator_Mapped(iter);
					ctx.writer.StartObject();
					ctx.writer.Key(Serializer::Key::KEY);
					WriteVar(key, ctx);
					ctx.writer.Key(Serializer::Key::MAPPED);
					WriteVar(mapped, ctx);
					ctx.writer.EndObject();
				}
				ctx.writer.EndArray();
			}
		}
		else if constexpr (TupleTraits<Value>::isTuple) {
			ctx.writer.StartArray();
			std::apply([&](const auto& ... elements) {
				(WriteVar(elements, ctx), ...);
			}, var);
			ctx.writer.EndArray();
		}
		else if constexpr (is_instance_of_v<Value, std::variant>) {
			constexpr size_t N = std::variant_size_v<Value>;
			WriteVariant(var, std::make_index_sequence<N>{}, ctx);
		}
		else
			WriteUserType(&var, ctx);
	}

	template<typename Value>
	void ReadVar(Value& var, const rapidjson::Value& jsonValueField, Serializer::DeserializeContext& ctx);

	template<size_t Idx, typename Variant>
	bool ReadVariantAt(Variant& var, size_t idx, const rapidjson::Value& jsonValueContent, Serializer::DeserializeContext& ctx) {
		if (idx != Idx)
			return false;

		std::variant_alternative_t<Idx, Variant> element;
		ReadVar(element, jsonValueContent, ctx);
		var = std::move(element);

		return true;
	}

	// TODO : stop
	template<typename Variant, size_t... Ns>
	void ReadVariant(Variant& var, std::index_sequence<Ns...>, const rapidjson::Value& jsonValueField, Serializer::DeserializeContext& ctx) {
		const auto& jsonObject = jsonValueField.GetObject();
		size_t idx = jsonObject[Serializer::Key::INDEX].GetUint64();
		const auto& jsonValueVariantData = jsonObject[Serializer::Key::CONTENT];
		(ReadVariantAt<Ns>(var, idx, jsonValueVariantData, ctx), ...);
	}

	template<typename UserType>
	void ReadUserType(UserType* obj, const rapidjson::Value& jsonValueField, Serializer::DeserializeContext& ctx) {
		if constexpr (HasTypeInfo<UserType>::value) {
			const auto& jsonObject = jsonValueField.GetObject();
			USRefl::TypeInfo<UserType>::ForEachVarOf(
				*obj,
				[&](auto field, auto& var) {
					auto target = jsonObject.FindMember(field.name.data());
					if (target == jsonObject.MemberEnd())
						return;
					
					ReadVar(var, target->value, ctx);
				}
			);
		}
		else {
			if (ctx.deserializer.IsRegistered(GetID<UserType>()))
				ctx.deserializer.Visit(GetID<UserType>(), obj, jsonValueField, ctx);
			else
				assert("not support" && false);
		}
	}

	template<typename Value>
	void ReadVar(Value& var, const rapidjson::Value& jsonValueField, Serializer::DeserializeContext& ctx) {
		if constexpr (std::is_floating_point_v<Value>)
			var = static_cast<Value>(jsonValueField.GetDouble());
		else if constexpr (std::is_enum_v<Value>) {
			std::underlying_type_t<Value> under;
			ReadVar(under, jsonValueField, ctx);
			var = static_cast<Value>(under);
		}
		else if constexpr (std::is_integral_v<Value>) {
			if constexpr (std::is_same_v<Value, bool>)
				var = jsonValueField.GetBool();
			else {
				constexpr size_t size = sizeof(Value);
				if constexpr (std::is_unsigned_v<Value>) {
					if constexpr (size <= sizeof(unsigned int))
						var = static_cast<Value>(jsonValueField.GetUint());
					else
						var = static_cast<Value>(jsonValueField.GetUint64());
				}
				else {
					if constexpr (size <= sizeof(int))
						var = static_cast<Value>(jsonValueField.GetInt());
					else
						var = static_cast<Value>(jsonValueField.GetInt64());
				}
			}
		}
		else if constexpr (std::is_same_v<Value, std::string>)
			var = jsonValueField.GetString();
		else if constexpr (std::is_pointer_v<Value>) {
			if (jsonValueField.IsNull())
				var = nullptr;
			else {
				assert("not support" && false);
			}
		}
		else if constexpr (is_instance_of_v<Value, std::shared_ptr> || is_instance_of_v<Value, USTL::shared_object>) {
			if (jsonValueField.IsNull())
				var = nullptr;
			else if (jsonValueField.IsString()) {
				using Asset = typename Value::element_type;
				std::string guid_str = jsonValueField.GetString();
				const auto& path = AssetMngr::Instance().GUIDToAssetPath(xg::Guid{ guid_str });
				var = AssetMngr::Instance().LoadAsset<Asset>(path);
			}
			else
				assert("not support" && false);
		}
		else if constexpr (std::is_same_v<Value, UECS::Entity>) {
			auto index = jsonValueField.GetUint64();
			var = ctx.entityIdxMap.at(index);
		}
		else if constexpr (ArrayTraits<Value>::isArray) {
			const auto& arr = jsonValueField.GetArray();
			size_t N = std::min<size_t>(arr.Size(), ArrayTraits<Value>::size);
			for (size_t i = 0; i < N; i++)
				ReadVar(ArrayTraits_Get(var, i), arr[static_cast<rapidjson::SizeType>(i)], ctx);
		}
		else if constexpr (OrderContainerTraits<Value>::isOrderContainer) {
			const auto& arr = jsonValueField.GetArray();
			for (const auto& jsonValueElement : arr) {
				OrderContainerTraits_ValueType<Value> element;
				ReadVar(element, jsonValueElement, ctx);
				OrderContainerTraits_Add(var, std::move(element));
			}
			OrderContainerTraits_PostProcess(var);
		}
		else if constexpr (MapTraits<Value>::isMap) {
			if constexpr (std::is_same_v<MapTraits_KeyType<Value>, std::string>) {
				const auto& m = jsonValueField.GetObject();
				for (const auto& [val_key, val_mapped] : m) {
					MapTraits_MappedType<Value> mapped;
					ReadVar(mapped, val_mapped, ctx);
					MapTraits_Emplace(
						var,
						MapTraits_KeyType<Value>{val_key.GetString()},
						std::move(mapped)
					);
				}
			}
			else {
				const auto& m = jsonValueField.GetArray();
				for (const auto& val_pair : m) {
					const auto& pair = val_pair.GetObject();
					MapTraits_KeyType<Value> key;
					MapTraits_MappedType<Value> mapped;
					ReadVar(key, pair[Serializer::Key::KEY], ctx);
					ReadVar(mapped, pair[Serializer::Key::MAPPED], ctx);
					MapTraits_Emplace(var, std::move(key), std::move(mapped));
				}
			}
		}
		else if constexpr (TupleTraits<Value>::isTuple) {
			std::apply([&](auto& ... elements) {
				const auto& arr = jsonValueField.GetArray();
				rapidjson::SizeType i = 0;
				(ReadVar(elements, arr[i++], ctx), ...);
			}, var);
		}
		else if constexpr (is_instance_of_v<Value, std::variant>) {
			constexpr size_t N = std::variant_size_v<Value>;
			ReadVariant(var, std::make_index_sequence<N>{}, jsonValueField, ctx);
		}
		else
			ReadUserType(&var, jsonValueField, ctx);
	};
}

namespace Ubpa::Utopia {
	template<typename Func>
	void Serializer::RegisterComponentSerializeFunction(Func&& func) {
		using ArgList = FuncTraits_ArgList<Func>;
		static_assert(Length_v<ArgList> == 2);
		static_assert(std::is_same_v<At_t<ArgList, 1>, SerializeContext&>);
		using ConstCmptPtr = At_t<ArgList, 0>;
		static_assert(std::is_pointer_v<ConstCmptPtr>);
		using ConstCmpt = std::remove_pointer_t<ConstCmptPtr>;
		static_assert(std::is_const_v<ConstCmpt>);
		using Cmpt = std::remove_const_t<ConstCmpt>;
		RegisterComponentSerializeFunction(
			UECS::CmptType::Of<Cmpt>,
			[f = std::forward<Func>(func)](const void* p, SerializeContext& ctx) {
				f(reinterpret_cast<const Cmpt*>(p), ctx);
			}
		);
	}

	template<typename Func>
	void Serializer::RegisterComponentDeserializeFunction(Func&& func) {
		using ArgList = FuncTraits_ArgList<Func>;
		static_assert(Length_v<ArgList> == 3);
		static_assert(std::is_same_v<At_t<ArgList, 1>, const rapidjson::Value&>);
		static_assert(std::is_same_v<At_t<ArgList, 2>, DeserializeContext&>);
		using CmptPtr = At_t<ArgList, 0>;
		static_assert(std::is_pointer_v<CmptPtr>);
		using Cmpt = std::remove_pointer_t<CmptPtr>;
		static_assert(!std::is_const_v<Cmpt>);
		RegisterComponentDeserializeFunction(
			UECS::CmptType::Of<Cmpt>,
			[f = std::forward<Func>(func)](void* p, const rapidjson::Value& jsonValueCmpt, DeserializeContext& ctx) {
				f(reinterpret_cast<Cmpt*>(p), jsonValueCmpt, ctx);
			}
		);
	}

	template<typename Func>
	void Serializer::RegisterUserTypeSerializeFunction(Func&& func) {
		using ArgList = FuncTraits_ArgList<Func>;
		static_assert(Length_v<ArgList> == 2);
		static_assert(std::is_same_v<At_t<ArgList, 1>, SerializeContext&>);
		using ConstUserTypePtr = At_t<ArgList, 0>;
		static_assert(std::is_pointer_v<ConstUserTypePtr>);
		using ConstUserType = std::remove_pointer_t<ConstUserTypePtr>;
		static_assert(std::is_const_v<ConstUserType>);
		using UserType = std::remove_const_t<ConstUserType>;
		RegisterUserTypeSerializeFunction(
			GetID<UserType>(),
			[f = std::forward<Func>(func)](const void* p, SerializeContext& ctx) {
				f(reinterpret_cast<const UserType*>(p), ctx);
			}
		);
	}

	template<typename Func>
	void Serializer::RegisterUserTypeDeserializeFunction(Func&& func) {
		using ArgList = FuncTraits_ArgList<Func>;
		static_assert(Length_v<ArgList> == 3);
		static_assert(std::is_same_v<At_t<ArgList, 1>, const rapidjson::Value&>);
		static_assert(std::is_same_v<At_t<ArgList, 2>, DeserializeContext&>);
		using UserTypePtr = At_t<ArgList, 0>;
		static_assert(std::is_pointer_v<UserTypePtr>);
		using UserType = std::remove_pointer_t<UserTypePtr>;
		static_assert(!std::is_const_v<UserType>);
		RegisterUserTypeDeserializeFunction(
			GetID<UserType>(),
			[f = std::forward<Func>(func)](void* p, const rapidjson::Value& jsonValueCmpt, DeserializeContext& ctx) {
				f(reinterpret_cast<UserType*>(p), jsonValueCmpt, ctx);
			}
		);
	}

	template<typename... Cmpts>
	void Serializer::RegisterComponentSerializeFunction() {
		(RegisterComponentSerializeFunction(&detail::WriteUserType<Cmpts>), ...);
	}

	template<typename... Cmpts>
	void Serializer::RegisterComponentDeserializeFunction() {
		(RegisterComponentDeserializeFunction(&detail::ReadUserType<Cmpts>), ...);
	}

	template<typename... UserTypes>
	void Serializer::RegisterUserTypeSerializeFunction() {
		(RegisterUserTypeSerializeFunction(&detail::WriteUserType<UserTypes>), ...);
	}

	template<typename... UserTypes>
	void Serializer::RegisterUserTypeDeserializeFunction() {
		(RegisterUserTypeDeserializeFunction(&detail::ReadUserType<UserTypes>), ...);
	}

	template<typename... Cmpts>
	void Serializer::RegisterComponents() {
		RegisterComponentSerializeFunction<Cmpts...>();
		RegisterComponentDeserializeFunction<Cmpts...>();
	}

	// register UserTypes' serialize and deserialize function
	template<typename... UserTypes>
	void Serializer::RegisterUserTypes() {
		RegisterUserTypeSerializeFunction<UserTypes...>();
		RegisterUserTypeDeserializeFunction<UserTypes...>();
	}

	template<typename UserType>
	std::string Serializer::ToJSON(const UserType* obj) {
		static_assert(!std::is_void_v<UserType>);
		return ToJSON(GetID<UserType>(), obj);
	}

	template<typename UserType>
	bool Serializer::ToUserType(std::string_view json, UserType* obj) {
		static_assert(!std::is_void_v<UserType>);
		return ToUserType(json, GetID<UserType>(), obj);
	}
}
