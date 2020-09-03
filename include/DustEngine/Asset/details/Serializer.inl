#pragma once

#include "../../Core/Traits.h"
#include "../AssetMngr.h"

namespace Ubpa::DustEngine::detail {
	template<typename Value>
	void WriteVar(const Value& var, Serializer::SerializeContext ctx);

	template<typename UserType>
	void WriteUserType(const UserType* obj, Serializer::SerializeContext ctx) {
		if constexpr (HasTypeInfo<UserType>::value) {
			ctx.writer->StartObject();
			Ubpa::USRefl::TypeInfo<UserType>::ForEachVarOf(
				*obj,
				[&ctx](auto field, const auto& var) {
					ctx.writer->Key(field.name.data());
					detail::WriteVar(var, ctx);
				}
			);
			ctx.writer->EndObject();
		}
		else {
			if (ctx.serializer->IsRegistered(GetID<UserType>()))
				ctx.serializer->Visit(GetID<UserType>(), obj, ctx);
			else {
				assert("not support" && false);
				ctx.writer->String(Serializer::Key::NOT_SUPPORT);
			}
		}
	}

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
					if constexpr (size <= sizeof(unsigned int))
						ctx.writer->Uint(static_cast<std::uint32_t>(var));
					else
						ctx.writer->Uint64(static_cast<std::uint64_t>(var));
				}
				else {
					if constexpr (size <= sizeof(int))
						ctx.writer->Int(static_cast<std::int32_t>(var));
					else
						ctx.writer->Int64(static_cast<std::int64_t>(var));
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
		else if constexpr (OrderContainerTraits<Value>::isVector) {
			ctx.writer->StartArray();
			auto iter_end = OrderContainerTraits_End(var);
			for (auto iter = OrderContainerTraits_Begin(var); iter != iter_end; iter++)
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
					ctx.writer->Key(Serializer::Key::KEY);
					WriteVar(key, ctx);
					ctx.writer->Key(Serializer::Key::MAPPED);
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
		else
			WriteUserType(&var, ctx);
	}

	template<typename Value>
	Value ReadVar(const rapidjson::Value& jsonValueField, Serializer::DeserializeContext ctx);

	template<typename UserType>
	void ReadUserType(UserType* obj, const rapidjson::Value& jsonValueField, Serializer::DeserializeContext ctx) {
		if constexpr (HasTypeInfo<UserType>::value) {
			const auto& jsonObject = jsonValueField.GetObject();
			USRefl::TypeInfo<UserType>::ForEachVarOf(
				*obj,
				[&](auto field, auto& var) {
					auto target = jsonObject.FindMember(field.name.data());
					if (target == jsonObject.MemberEnd())
						return;

					var = ReadVar<std::remove_reference_t<decltype(var)>>(target->value, ctx);
				}
			);
		}
		else {
			if (ctx.deserializer->IsRegistered(GetID<UserType>()))
				ctx.deserializer->Visit(GetID<UserType>(), obj, jsonValueField, ctx);
			else
				assert("not support" && false);
		}
	}

	template<typename Value>
	Value ReadVar(const rapidjson::Value& jsonValueField, Serializer::DeserializeContext ctx) {
		if constexpr (std::is_floating_point_v<Value>)
			return static_cast<Value>(jsonValueField.GetDouble());
		else if constexpr (std::is_integral_v<Value>) {
			if constexpr (std::is_same_v<Value, bool>)
				return jsonValueField.GetBool();
			else {
				constexpr size_t size = sizeof(Value);
				if constexpr (std::is_unsigned_v<Value>) {
					if constexpr (size <= sizeof(unsigned int))
						return static_cast<Value>(jsonValueField.GetUint());
					else
						return static_cast<Value>(jsonValueField.GetUint64());
				}
				else {
					if constexpr (size <= sizeof(int))
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
		else {
			// compound type
			static_assert(std::is_default_constructible_v<Value>);
			Value rst;

			if constexpr (ArrayTraits<Value>::isArray) {
				const auto& arr = jsonValueField.GetArray();
				assert(ArrayTraits<Value>::size == arr.Size());
				for (size_t i = 0; i < ArrayTraits<Value>::size; i++) {
					ArrayTraits_Set(
						rst, i,
						ReadVar<ArrayTraits_ValueType<Value>>(arr[static_cast<rapidjson::SizeType>(i)], ctx)
					);
				}
			}
			else if constexpr (OrderContainerTraits<Value>::isVector) {
				const auto& arr = jsonValueField.GetArray();
				for (size_t i = 0; i < arr.Size(); i++) {
					OrderContainerTraits_Add(
						rst,
						ReadVar<OrderContainerTraits_ValueType<Value>>(arr[static_cast<rapidjson::SizeType>(i)], ctx)
					);
				}
				OrderContainerTraits_PostProcess(rst);
			}
			else if constexpr (MapTraits<Value>::isMap) {
				if constexpr (std::is_same_v<MapTraits_KeyType<Value>, std::string>) {
					const auto& m = jsonValueField.GetObject();
					for (const auto& [val_key, val_mapped] : m) {
						MapTraits_Emplace(
							rst,
							MapTraits_KeyType<Value>{val_key.GetString()},
							ReadVar<MapTraits_MappedType<Value>>(val_mapped, ctx)
						);
					}
				}
				else {
					const auto& m = jsonValueField.GetArray();
					for (const auto& val_pair : m) {
						const auto& pair = val_pair.GetObject();
						MapTraits_Emplace(
							rst,
							ReadVar<MapTraits_KeyType<Value>>(pair[Serializer::Key::KEY], ctx),
							ReadVar<MapTraits_MappedType<Value>>(pair[Serializer::Key::MAPPED], ctx)
						);
					}
				}
			}
			else if constexpr (TupleTraits<Value>::isTuple) {
				std::apply([&](auto& ... elements) {
					const auto& arr = jsonValueField.GetArray();
					rapidjson::SizeType i = 0;
					((elements = ReadVar<std::remove_reference_t<decltype(elements)>>(arr[i++], ctx)), ...);
				}, rst);
			}
			else
				ReadUserType(&rst, jsonValueField, ctx);

			return rst;
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
			GetID<UserType>(),
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
			GetID<UserType>(),
			[f = std::forward<Func>(func)](void* p, const rapidjson::Value& jsonValueCmpt, DeserializeContext ctx) {
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

	template<typename... Cmpts>
	void Serializer::Register() {
		RegisterComponentSerializeFunction<Cmpts...>();
		RegisterComponentDeserializeFunction<Cmpts...>();
	}
}
