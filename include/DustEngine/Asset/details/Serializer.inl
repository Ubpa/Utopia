#pragma once

#include <UTemplate/Func.h>
#include <USRefl/USRefl.h>
#include "../AssetMngr.h"

namespace Ubpa::DustEngine {
	template<typename Func>
	void Serializer::RegisterComponentSerializeFunction(Func&& func) {
		using ArgList = FuncTraits_ArgList<Func>;
		static_assert(Length_v<ArgList> == 2);
		static_assert(std::is_same_v<At_t<ArgList, 1>, JSONWriter&>);
		using ConstCmptPtr = At_t<ArgList, 0>;
		static_assert(std::is_pointer_v<ConstCmptPtr>);
		using ConstCmpt = std::remove_pointer_t<ConstCmptPtr>;
		static_assert(std::is_const_v<ConstCmpt>);
		using Cmpt = std::remove_const_t<ConstCmpt>;
		RegisterComponentSerializeFunction(
			UECS::CmptType::Of<Cmpt>,
			[func = std::forward<Func>(func)](const void* p, JSONWriter& writer) {
				func(reinterpret_cast<const Cmpt*>(p), writer);
			}
		);
	}

	template<typename Cmpt>
	void Serializer::RegisterComponentSerializeFunction() {
		RegisterComponentSerializeFunction([](const Cmpt* cmpt, JSONWriter& writer) {
			USRefl::TypeInfo<Cmpt>::ForEachVarOf(*cmpt, [&writer](auto field, const auto& var) {
				writer.Key(field.name.data());
				using Value = std::decay_t<decltype(var)>;
				if constexpr (std::is_floating_point_v<Value>)
					writer.Double(static_cast<double>(var));
				else if constexpr (std::is_integral_v<Value>) {
					if constexpr (std::is_same_v<Value, bool>)
						writer.Bool(var);
					else {
						constexpr size_t size = sizeof(Value);
						if constexpr (std::is_unsigned_v<Value>) {
							if constexpr (size <= 4)
								writer.Uint(static_cast<std::uint32_t>(var));
							else
								writer.Uint64(static_cast<std::uint64_t>(var));
						}
						else {
							if constexpr (size <= 4)
								writer.Int(static_cast<std::uint32_t>(var));
							else
								writer.Int64(static_cast<std::uint64_t>(var));
						}
					}
				}
				else if constexpr (std::is_same_v<Value, std::string>)
					writer.String(var.c_str());
				else if constexpr (std::is_pointer_v<Value>) {
					if (var == nullptr)
						writer.Null();
					else {
						auto& assetMngr = AssetMngr::Instance();
						if (assetMngr.Contains(var))
							writer.String(assetMngr.AssetPathToGUID(assetMngr.GetAssetPath(var)).str());
						else {
							assert(false);
							writer.Null();
						}
					}
				}
				else if constexpr (std::is_same_v<Value, UECS::Entity>) {
					writer.Uint64(var.Idx());
				}
				else
					writer.String("__NOT_SUPPORT");
			});
		});
	}

	template<typename Cmpt>
	void Serializer::RegisterComponentDeserializeFunction() {
		RegisterComponentDeserializeFunction(
			UECS::CmptType::Of<Cmpt>,
			[](UECS::World* world, UECS::Entity e, const JSONCmpt& cmptJSON) {
				auto [cmpt] = world->entityMngr.Attach<Cmpt>(e);
				USRefl::TypeInfo<Cmpt>::ForEachVarOf(*cmpt, [&cmptJSON](auto field, auto& var) {
					const auto& val_var = cmptJSON[field.name.data()];
					using Value = std::decay_t<decltype(var)>;
					if constexpr (std::is_floating_point_v<Value>)
						var = static_cast<Value>(val_var.GetDouble());
					else if constexpr (std::is_integral_v<Value>) {
						if constexpr (std::is_same_v<Value, bool>)
							var = val_var.GetBool();
						else {
							constexpr size_t size = sizeof(Value);
							if constexpr (std::is_unsigned_v<Value>) {
								if constexpr (size <= 4)
									var = static_cast<Value>(val_var.GetUint());
								else
									var = static_cast<Value>(val_var.GetUint64());
							}
							else {
								if constexpr (size <= 4)
									var = static_cast<Value>(val_var.GetInt());
								else
									var = static_cast<Value>(val_var.GetInt64());
							}
						}
					}
					else if constexpr (std::is_same_v<Value, std::string>)
						var = val_var.GetString();
					else if constexpr (std::is_pointer_v<Value>) {
						if (val_var.IsNull())
							var = nullptr;
						else {
							using Asset = std::remove_const_t<std::remove_pointer_t<Value>>;
							std::string guid = val_var.GetString();
							const auto& path = AssetMngr::Instance().GUIDToAssetPath(xg::Guid{ guid });
							var = AssetMngr::Instance().LoadAsset<Asset>(path);
						}
					}
					else if constexpr (std::is_same_v<Value, UECS::Entity>)
						var = UECS::Entity::Invalid();
					else
						; // not support
				}
			);
		});
	}
}
