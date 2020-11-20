#pragma once

#include "../PlayloadType.h"

#include <Utopia/Asset/AssetMngr.h>

#include <_deps/imgui/imgui.h>
#include <_deps/imgui/misc/cpp/imgui_stdlib.h>

#include <Utopia/Core/Traits.h>

#include <UECS/Entity.h>

#include <USTL/tuple.h>
#include <Utopia/Core/Components/Name.h>

#include <variant>

namespace Ubpa::Utopia::detail {
	template<typename T>
	struct ValNTraits {
		static constexpr bool isValN = false;
	};
	template<typename T>
	struct ValNTraitsBase {
		static constexpr bool isValN = std::is_same_v<T, float>;
	};
	
	template<typename T, size_t N> struct ValNTraits<point <T, N>> : ValNTraitsBase<T> {};
	template<typename T, size_t N> struct ValNTraits<scale <T, N>> : ValNTraitsBase<T> {};
	template<typename T, size_t N> struct ValNTraits<val   <T, N>> : ValNTraitsBase<T> {};
	template<typename T, size_t N> struct ValNTraits<vec   <T, N>> : ValNTraitsBase<T> {};
	template<typename T>           struct ValNTraits<euler <T   >> : ValNTraitsBase<T> {};
	template<typename T>           struct ValNTraits<normal<T   >> : ValNTraitsBase<T> {};
	template<typename T>           struct ValNTraits<quat  <T   >> : ValNTraitsBase<T> {};
	template<typename T>           struct ValNTraits<rgb   <T   >> : ValNTraitsBase<T> {};
	template<typename T>           struct ValNTraits<rgba  <T   >> : ValNTraitsBase<T> {};
	template<typename T>           struct ValNTraits<svec  <T   >> : ValNTraitsBase<T> {};

	// =====================================================================================
	template<typename T>
	struct ColorTraits {
		static constexpr bool isColor = false;
	};
	template<typename T>
	struct ColorTraitsBase {
		static constexpr bool isColor = std::is_floating_point_v<T>;
	};

	template<typename T> struct ColorTraits<rgb <T>> : ColorTraitsBase<T> {};
	template<typename T> struct ColorTraits<rgba<T>> : ColorTraitsBase<T> {};

	// ==============================
	inline void HelpMarker(std::string_view desc) {
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(desc.data());
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}

	template<typename Field, typename Value>
	void InspectField(Field field, Value& var, InspectorRegistry::InspectContext ctx);
	template<typename Field, typename Value>
	void InspectVar(Field field, Value& var, InspectorRegistry::InspectContext ctx);
	template<typename Field, typename Value>
	bool InspectVar1(Field field, Value& var, InspectorRegistry::InspectContext ctx);

	constexpr auto GenerateNameField(std::string_view n) {
		return USRefl::Field{
			TSTR(""), nullptr, USRefl::AttrList{
				USRefl::Attr{TSTR(UInspector::name), n}
			}
		};
	}

	template<typename Fld>
	constexpr std::string_view GetFieldName(Fld field) {
		if constexpr (field.attrs.Contains(TSTR(UInspector::name)))
			return field.attrs.Find(TSTR(UInspector::name)).value;
		else
			return field.name;
	}

	template<typename Cmpt>
	void InspectCmpt(Cmpt* cmpt, InspectorRegistry::InspectContext ctx) {
		if constexpr (HasDefinition<USRefl::TypeInfo<Cmpt>>::value) {
			ImGui::PushID((const void *)UECS::CmptType::Of<Cmpt>.HashCode());
			if (ImGui::CollapsingHeader(USRefl::TypeInfo<Cmpt>::name)) {
				USRefl::TypeInfo<Cmpt>::ForEachVarOf(*cmpt, [ctx](auto field, auto& var) {
					InspectField(field, var, ctx);
				});
			}
			ImGui::PopID();
		}
		else {
			if (ctx.inspector.IsRegistered(UECS::CmptType::Of<Cmpt>.HashCode()))
				ctx.inspector.Visit(UECS::CmptType::Of<Cmpt>.HashCode(), cmpt, ctx);
		}
	}

	template<typename Asset>
	void InspectAsset(Asset* asset, InspectorRegistry::InspectContext ctx) {
		if constexpr (HasDefinition<USRefl::TypeInfo<Asset>>::value) {
			USRefl::TypeInfo<Asset>::ForEachVarOf(*asset, [ctx](auto field, auto& var) {
				InspectField(field, var, ctx);
			});
		}
		else {
			if (ctx.inspector.IsRegistered(typeid(Asset).hash_code()))
				ctx.inspector.Visit(typeid(Asset).hash_code(), asset, ctx);
		}
	}

	template<typename Field, typename UserType>
	void InspectUserType(Field field, UserType* obj, InspectorRegistry::InspectContext ctx) {
		if constexpr (HasDefinition<USRefl::TypeInfo<UserType>>::value) {
			if (ImGui::TreeNode(GetFieldName(field).data())) {
				ImGui::PushID((const void*)GetID<UserType>());
				USRefl::TypeInfo<UserType>::ForEachVarOf(*obj, [ctx](auto field, auto& var) {
					InspectVar(field, var, ctx);
				});
				ImGui::TreePop();
				ImGui::PopID();
			}
		}
		else
			ImGui::Text(GetFieldName(field).data());
	}

	template<typename Field, typename Value>
	void InspectField(Field field, Value& var, InspectorRegistry::InspectContext ctx) {
		if constexpr (!field.attrs.Contains(TSTR(UInspector::hide))) {
			if constexpr (field.attrs.Contains(TSTR(UInspector::header))) {
				std::string_view sv{ field.attrs.Find(TSTR(UInspector::header)).value };
				ImGui::Text(sv.data());
			}
			if constexpr (field.attrs.Contains(TSTR(UInspector::tooltip))) {
				std::string_view sv{ field.attrs.Find(TSTR(UInspector::tooltip)).value };
				HelpMarker(sv);
				ImGui::SameLine();
			}
			InspectVar(field, var, ctx);
		}
	}

	template<typename Field, typename Value>
	void InspectVar(Field field, const Value& var, InspectorRegistry::InspectContext ctx) {
		if constexpr (std::is_same_v<Value, bool>) {
			ImGui::Button(var ? "true" : "false", &var);
			ImGui::SameLine();
			ImGui::Text(GetFieldName(field).data());
		}
		else if constexpr (std::is_arithmetic_v<Value>) {
			auto value = std::to_string(var);
			ImGui::Button(value.c_str());
			ImGui::SameLine();
			ImGui::Text(GetFieldName(field).data());
		}
		else if constexpr (std::is_same_v<Value, std::string>) {
			ImGui::Button(var.c_str());
			ImGui::SameLine();
			ImGui::Text(GetFieldName(field).data());
		}
		else if constexpr (std::is_same_v<Value, UECS::Entity>) {
			if (auto name = ctx.world->entityMngr.Get<Name>(var))
				ImGui::BulletText(name->value.c_str());
			else
				ImGui::BulletText("Entity (%d)", var.Idx());
		}
		else if constexpr (std::is_pointer_v<Value> || is_instance_of_v<Value, std::shared_ptr> || is_instance_of_v<Value, USTL::shared_object>) {
			ImGui::Text("(* const)");
			ImGui::SameLine();
			if (var) {
				if constexpr (std::is_pointer_v<Value>)
					InspectVar(field, *var, ctx);
				else {
					using Element = typename Value::element_type;
					if constexpr (std::is_base_of_v<Object, Element>) {
						const auto& path = AssetMngr::Instance().GetAssetPath(*var);
						if (!path.empty()) {
							auto name = path.stem().string();
							ImGui::Text(name.c_str());
						}
						else
							ImGui::Text("UNKNOW");
					}
					else
						InspectVar(field, *var, ctx);
				}
			}
			else
				ImGui::Text("nullptr");
		}
		else if constexpr (ArrayTraits<Value>::isArray) {
			if constexpr (ValNTraits<Value>::isValN) {
				static Value copiedVar;
				copiedVar = var;
				auto data = ArrayTraits_Data(copiedVar);
				constexpr auto N = ArrayTraits<Value>::size;

				if constexpr (ColorTraits<Value>::isColor) {
					constexpr ImGuiColorEditFlags flags =
						ImGuiColorEditFlags_Float
						| ImGuiColorEditFlags_HDR
						| ImGuiColorEditFlags_AlphaPreview;

					if constexpr (N == 3)
						ImGui::ColorEdit3(GetFieldName(field).data(), data, flags);
					else // N == 4
						ImGui::ColorEdit4(GetFieldName(field).data(), data, flags);
				}
				else {
					using ElemType = ArrayTraits_ValueType<Value>;
					if constexpr (std::is_same_v<ElemType, uint8_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_U8, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, uint16_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_U16, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, uint32_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_U32, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, uint64_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_U64, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, int8_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_S8, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, int16_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_S16, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, int32_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_S32, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, int64_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_S64, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, float>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_Float, data, N, 0.001f);
					else if constexpr (std::is_same_v<ElemType, double>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_Double, data, N, 0.001f);
				}
			}
			else {
				if (ImGui::TreeNode(GetFieldName(field).data())) {
					ImGui::PushID(GetFieldName(field).data());
					for (size_t i = 0; i < ArrayTraits<Value>::size; i++) {
						auto str = std::to_string(i);
						InspectVar(GenerateNameField(str), ArrayTraits_Get(var, i), ctx);
					}
					ImGui::PopID();
					ImGui::TreePop();
				}
			}
		}
		else if constexpr (TupleTraits<Value>::isTuple) {
			if (ImGui::TreeNode(GetFieldName(field).data())) {
				ImGui::PushID(GetFieldName(field).data());
				USTL::tuple_for_each(var, [ctx, idx = 0](auto& ele) mutable {
					auto str = std::to_string(idx++);
					InspectVar(GenerateNameField(str), ele, ctx);
				});
				ImGui::PopID();
				ImGui::TreePop();
			}
		}
		else if constexpr (MapTraits<Value>::isMap) {
			if (ImGui::TreeNode(GetFieldName(field).data())) {
				ImGui::PushID(GetFieldName(field).data());
				auto iter_begin = MapTraits_Begin(var);
				auto iter_end = MapTraits_End(var);
				for (auto iter = iter_begin; iter != iter_end; ++iter) {
					auto& [key, mapped] = *iter;
					if constexpr (std::is_same_v<std::decay_t<decltype(key)>, std::string>)
						InspectVar(GenerateNameField(key), mapped, ctx);
					else {
						auto name = std::to_string(key);
						InspectVar(GenerateNameField(name), mapped, ctx);
					}
				}
				ImGui::PopID();
				ImGui::TreePop();
			}
		}
		else if constexpr (OrderContainerTraits<Value>::isOrderContainer) {
			if (ImGui::TreeNode(GetFieldName(field).data())) {
				ImGui::PushID(GetFieldName(field).data());
				auto iter_begin = OrderContainerTraits_Begin(var);
				auto iter_end = OrderContainerTraits_End(var);
				size_t idx = 0;
				for (auto iter = iter_begin; iter != iter_end; ++iter) {
					auto name = std::to_string(idx++);
					InspectVar(GenerateNameField(name), *iter, ctx);
				}
				ImGui::PopID();
				ImGui::TreePop();
			}
		}
		else {
			//assert(false);
			//InspectUserType(field, &var, ctx);
		}
	}

	template<size_t Idx, typename Field, typename Variant>
	bool InspectVariantAt(Field field, Variant& var, size_t idx, InspectorRegistry::InspectContext ctx) {
		if (idx != Idx)
			return false;

		using Value = std::variant_alternative_t<Idx, Variant>;
		InspectVar(field, std::get<Value>(var), ctx);

		return true;
	}

	// TODO : stop
	template<typename Field, typename Variant, size_t... Ns>
	void InspectVariant(Field field, Variant& var, std::index_sequence<Ns...>, InspectorRegistry::InspectContext ctx) {
		(InspectVariantAt<Ns>(field, var, var.index(), ctx), ...);
	}

	template<size_t Idx, typename Field, typename Variant>
	bool InspectVariantAt1(bool& changed, Field field, Variant& var, size_t idx, InspectorRegistry::InspectContext ctx) {
		if (idx != Idx)
			return false;

		using Value = std::variant_alternative_t<Idx, Variant>;
		if (InspectVar1(field, std::get<Value>(var), ctx))
			changed = true;

		return true;
	}

	// TODO : stop
	template<typename Field, typename Variant, size_t... Ns>
	bool InspectVariant1(Field field, Variant& var, std::index_sequence<Ns...>, InspectorRegistry::InspectContext ctx) {
		bool changed = false;
		(InspectVariantAt1<Ns>(changed, field, var, var.index(), ctx), ...);
		return changed;
	}

	template<typename Field, typename Value>
	void InspectVar(Field field, Value& var, InspectorRegistry::InspectContext ctx) {
		//static_assert(!std::is_const_v<Value>);
		if constexpr (std::is_same_v<Value, bool>)
			ImGui::Checkbox(GetFieldName(field).data(), &var);
		else if constexpr (std::is_integral_v<Value> || std::is_floating_point_v<Value>) {
			if constexpr (field.attrs.Contains(TSTR(UInspector::range))) {
				Value minvalue = field.attrs.Find(TSTR(UInspector::range)).value.first;
				Value maxvalue = field.attrs.Find(TSTR(UInspector::range)).value.second;
				if constexpr (std::is_same_v<Value, uint8_t>)
					ImGui::SliderScalar(GetFieldName(field).data(), ImGuiDataType_U8, &var, &minvalue, &maxvalue);
				else if constexpr (std::is_same_v<Value, uint16_t>)
					ImGui::SliderScalar(GetFieldName(field).data(), ImGuiDataType_U16, &var, &minvalue, &maxvalue);
				else if constexpr (std::is_same_v<Value, uint32_t>)
					ImGui::SliderScalar(GetFieldName(field).data(), ImGuiDataType_U32, &var, &minvalue, &maxvalue);
				else if constexpr (std::is_same_v<Value, uint64_t>)
					ImGui::SliderScalar(GetFieldName(field).data(), ImGuiDataType_U64, &var, &minvalue, &maxvalue);
				else if constexpr (std::is_same_v<Value, int8_t>)
					ImGui::SliderScalar(GetFieldName(field).data(), ImGuiDataType_S8, &var, &minvalue, &maxvalue);
				else if constexpr (std::is_same_v<Value, int16_t>)
					ImGui::SliderScalar(GetFieldName(field).data(), ImGuiDataType_S16, &var, &minvalue, &maxvalue);
				else if constexpr (std::is_same_v<Value, int32_t>)
					ImGui::SliderScalar(GetFieldName(field).data(), ImGuiDataType_S32, &var, &minvalue, &maxvalue);
				else if constexpr (std::is_same_v<Value, int64_t>)
					ImGui::SliderScalar(GetFieldName(field).data(), ImGuiDataType_S64, &var, &minvalue, &maxvalue);
				else if constexpr (std::is_same_v<Value, float>)
					ImGui::SliderScalar(GetFieldName(field).data(), ImGuiDataType_Float, &var, &minvalue, &maxvalue);
				else if constexpr (std::is_same_v<Value, double>)
					ImGui::SliderScalar(GetFieldName(field).data(), ImGuiDataType_Double, &var, &minvalue, &maxvalue);
				else
					static_assert(false);
			}
			else {
				Value minvalue;
				float step;

				if constexpr (field.attrs.Contains(TSTR(UInspector::min_value)))
					minvalue = field.attrs.Find(TSTR(UInspector::min_value)).value;
				else if constexpr (std::is_floating_point_v<Value>)
					minvalue = -std::numeric_limits<Value>::max();
				else
					minvalue = std::numeric_limits<Value>::min();

				if constexpr (field.attrs.Contains(TSTR(UInspector::step)))
					step = field.attrs.Find(TSTR(UInspector::step)).value;
				else if constexpr (std::is_floating_point_v<Value>)
					step = 0.01f;
				else
					step = 1.f; // integer

				if constexpr (std::is_same_v<Value, uint8_t>)
					ImGui::DragScalar(GetFieldName(field).data(), ImGuiDataType_U8, &var, step, &minvalue);
				else if constexpr (std::is_same_v<Value, uint16_t>)
					ImGui::DragScalar(GetFieldName(field).data(), ImGuiDataType_U16, &var, step, &minvalue);
				else if constexpr (std::is_same_v<Value, uint32_t>)
					ImGui::DragScalar(GetFieldName(field).data(), ImGuiDataType_U32, &var, step, &minvalue);
				else if constexpr (std::is_same_v<Value, uint64_t>)
					ImGui::DragScalar(GetFieldName(field).data(), ImGuiDataType_U64, &var, step, &minvalue);
				else if constexpr (std::is_same_v<Value, int8_t>)
					ImGui::DragScalar(GetFieldName(field).data(), ImGuiDataType_S8, &var, step, &minvalue);
				else if constexpr (std::is_same_v<Value, int16_t>)
					ImGui::DragScalar(GetFieldName(field).data(), ImGuiDataType_S16, &var, step, &minvalue);
				else if constexpr (std::is_same_v<Value, int32_t>)
					ImGui::DragScalar(GetFieldName(field).data(), ImGuiDataType_S32, &var, step, &minvalue);
				else if constexpr (std::is_same_v<Value, int64_t>)
					ImGui::DragScalar(GetFieldName(field).data(), ImGuiDataType_S64, &var, step, &minvalue);
				else if constexpr (std::is_same_v<Value, float>)
					ImGui::DragScalar(GetFieldName(field).data(), ImGuiDataType_Float, &var, step, &minvalue);
				else if constexpr (std::is_same_v<Value, double>)
					ImGui::DragScalar(GetFieldName(field).data(), ImGuiDataType_Double, &var, step, &minvalue);
				else
					static_assert(false);
			}
		}
		else if constexpr (std::is_same_v<Value, std::string>)
			ImGui::InputText(GetFieldName(field).data(), &var);
		else if constexpr (std::is_same_v<Value, UECS::Entity>) {
			ImGui::Text("(*)", var.Idx());
			ImGui::SameLine();
			if (var.Valid()) {
				if (auto name = ctx.world->entityMngr.Get<Name>(var))
					ImGui::Button(name->value.c_str());
				else {
					std::string str = std::string("Entity (") + std::to_string(var.Idx()) + ")";
					ImGui::Button(str.c_str());
				}
			}
			else
				ImGui::Button("None (Entity)");
			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PlayloadType::ENTITY)) {
					IM_ASSERT(payload->DataSize == sizeof(UECS::Entity));
					const auto& payload_e = *(UECS::Entity*)payload->Data;
					var = payload_e;
				}
				ImGui::EndDragDropTarget();
			}
			ImGui::SameLine();
			ImGui::Text(GetFieldName(field).data());
		}
		else if constexpr (std::is_enum_v<Value>) {
			if constexpr (HasDefinition<USRefl::TypeInfo<Value>>::value) {
				std::string_view cur;
				USRefl::TypeInfo<Value>::fields.FindIf([&](auto field) {
					if (field.value == var) {
						cur = GetFieldName(field);
						return true;
					}
					return false;
				});

				ImGui::Button(cur.data());
				ImGui::SameLine();
				ImGui::Text(GetFieldName(field).data());
			}
			else {
				InspectVar(field, static_cast<std::underlying_type_t<Value>&>(var), ctx);
			}
		}
		else if constexpr (is_instance_of_v<Value, std::shared_ptr> || is_instance_of_v<Value, USTL::shared_object>) {
			using Element = typename Value::element_type;
			if constexpr (std::is_base_of_v<Object, Element>) {
				ImGui::Text("(*)");
				ImGui::SameLine();
				// button
				if (var) {
					const auto& path = AssetMngr::Instance().GetAssetPath(*var);
					if (!path.empty()) {
						auto name = path.stem().string();
						ImGui::Button(name.c_str());
					}
					else
						ImGui::Button("UNKNOW");
				}
				else
					ImGui::Button("nullptr");

				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PlayloadType::GUID)) {
						IM_ASSERT(payload->DataSize == sizeof(xg::Guid));
						const auto& payload_guid = *(const xg::Guid*)payload->Data;
						const auto& path = AssetMngr::Instance().GUIDToAssetPath(payload_guid);
						assert(!path.empty());
						if (auto asset = AssetMngr::Instance().LoadAsset<Element>(path))
							var = asset;
					}
					ImGui::EndDragDropTarget();
				}
				ImGui::SameLine();
				ImGui::Text(GetFieldName(field).data());
			}
			else {
				InspectVar(field, *var, ctx);
			}
		}
		else if constexpr (std::is_pointer_v<Value>) {
			ImGui::Text("(*)");
			ImGui::SameLine();
			// button
			if (var)
				InspectVar(field, *var, ctx);
			else
				ImGui::Text("nullptr");
		}
		else if constexpr (ArrayTraits<Value>::isArray) {
			if constexpr (ValNTraits<Value>::isValN) {
				auto data = ArrayTraits_Data(var);
				constexpr auto N = ArrayTraits<Value>::size;

				if constexpr (ColorTraits<Value>::isColor) {
					constexpr ImGuiColorEditFlags flags =  
						ImGuiColorEditFlags_Float
						| ImGuiColorEditFlags_HDR
						| ImGuiColorEditFlags_AlphaPreview;

					if constexpr (N == 3)
						ImGui::ColorEdit3(GetFieldName(field).data(), data, flags);
					else // N == 4
						ImGui::ColorEdit4(GetFieldName(field).data(), data, flags);
				}
				else {
					using ElemType = ArrayTraits_ValueType<Value>;
					if constexpr (std::is_same_v<ElemType, uint8_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_U8, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, uint16_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_U16, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, uint32_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_U32, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, uint64_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_U64, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, int8_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_S8, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, int16_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_S16, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, int32_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_S32, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, int64_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_S64, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, float>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_Float, data, N, 0.001f);
					else if constexpr (std::is_same_v<ElemType, double>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_Double, data, N, 0.001f);
				}
			}
			else {
				if (ImGui::TreeNode(GetFieldName(field).data())) {
					ImGui::PushID(GetFieldName(field).data());
					for (size_t i = 0; i < ArrayTraits<Value>::size; i++) {
						auto str = std::to_string(i);
						InspectVar(GenerateNameField(str), ArrayTraits_Get(var, i), ctx);
					}
					ImGui::PopID();
					ImGui::TreePop();
				}
			}
		}
		else if constexpr (TupleTraits<Value>::isTuple) {
			if (ImGui::TreeNode(GetFieldName(field).data())) {
				ImGui::PushID(GetFieldName(field).data());
				USTL::tuple_for_each(var, [ctx, idx = 0](auto& ele) mutable {
					auto str = std::to_string(idx++);
					InspectVar(GenerateNameField(str), ele, ctx);
				});
				ImGui::PopID();
				ImGui::TreePop();
			}
		}
		else if constexpr (is_instance_of_v<Value, std::variant>)
			InspectVariant(field, var, std::make_index_sequence<std::variant_size_v<Value>>{}, ctx);
		else if constexpr (MapTraits<Value>::isMap) {
			if (ImGui::TreeNode(GetFieldName(field).data())) {
				ImGui::PushID(GetFieldName(field).data());
				auto iter_begin = MapTraits_Begin(var);
				auto iter_end = MapTraits_End(var);
				for (auto iter = iter_begin; iter != iter_end; ++iter) {
					auto& [key, mapped] = *iter;
					if constexpr (std::is_same_v<std::decay_t<decltype(key)>, std::string>)
						InspectVar(GenerateNameField(key), mapped, ctx);
					else {
						auto name = std::to_string(key);
						InspectVar(GenerateNameField(name), mapped, ctx);
					}
				}
				ImGui::PopID();
				ImGui::TreePop();
			}
		}
		else if constexpr (OrderContainerTraits<Value>::isOrderContainer) {
			if (ImGui::TreeNode(GetFieldName(field).data())) {
				ImGui::PushID(GetFieldName(field).data());

				if constexpr (OrderContainerTraits<Value>::isResizable) {
					size_t origSize = OrderContainerTraits_Size(var);
					size_t size = origSize;
					static constexpr ImU64 u64_one = 1;
					ImGui::InputScalar("size", ImGuiDataType_U64, &size, &u64_one);
					if (size != origSize)
						OrderContainerTraits_Resize(var, size);
				}

				auto iter_begin = OrderContainerTraits_Begin(var);
				auto iter_end = OrderContainerTraits_End(var);
				size_t idx = 0;
				for (auto iter = iter_begin; iter != iter_end; ++iter) {
					auto name = std::to_string(idx++);
					InspectVar(GenerateNameField(name), *iter, ctx);
				}
				ImGui::PopID();
				ImGui::TreePop();
			}
		}
		else {
			InspectUserType(field, &var, ctx);
		}
	}

	// return ture if something change
	template<typename Field, typename Value>
	bool InspectVar1(Field field, Value& var, InspectorRegistry::InspectContext ctx) {
		//static_assert(!std::is_const_v<Value>);
		if constexpr (std::is_scalar_v<Value>
		|| std::is_same_v<Value, std::string>
		|| std::is_same_v<Value, UECS::Entity>
		|| is_instance_of_v<Value, std::shared_ptr>
		|| ArrayTraits<Value>::isArray && ValNTraits<Value>::isValN
		)
		{
			Value orig = var;
			if constexpr (std::is_same_v<Value, bool>)
				ImGui::Checkbox(GetFieldName(field).data(), &var);
			else if constexpr (std::is_same_v<Value, uint8_t>)
				ImGui::DragScalar(GetFieldName(field).data(), ImGuiDataType_U8, &var, 1.f);
			else if constexpr (std::is_same_v<Value, uint16_t>)
				ImGui::DragScalar(GetFieldName(field).data(), ImGuiDataType_U16, &var, 1.f);
			else if constexpr (std::is_same_v<Value, uint32_t>)
				ImGui::DragScalar(GetFieldName(field).data(), ImGuiDataType_U32, &var, 1.f);
			else if constexpr (std::is_same_v<Value, uint64_t>)
				ImGui::DragScalar(GetFieldName(field).data(), ImGuiDataType_U64, &var, 1.f);
			else if constexpr (std::is_same_v<Value, int8_t>)
				ImGui::DragScalar(GetFieldName(field).data(), ImGuiDataType_S8, &var, 1.f);
			else if constexpr (std::is_same_v<Value, int16_t>)
				ImGui::DragScalar(GetFieldName(field).data(), ImGuiDataType_S16, &var, 1.f);
			else if constexpr (std::is_same_v<Value, int32_t>)
				ImGui::DragScalar(GetFieldName(field).data(), ImGuiDataType_S32, &var, 1.f);
			else if constexpr (std::is_same_v<Value, int64_t>)
				ImGui::DragScalar(GetFieldName(field).data(), ImGuiDataType_S64, &var, 1.f);
			else if constexpr (std::is_same_v<Value, float>)
				ImGui::DragFloat(GetFieldName(field).data(), &var, 0.001f);
			else if constexpr (std::is_same_v<Value, double>)
				ImGui::DragScalar(GetFieldName(field).data(), ImGuiDataType_Double, &var, 0.001f);
			else if constexpr (std::is_same_v<Value, std::string>)
				ImGui::InputText(GetFieldName(field).data(), &var);
			else if constexpr (std::is_same_v<Value, UECS::Entity>) {
				ImGui::Text("(*)", var.Idx());
				ImGui::SameLine();
				if (var.Valid()) {
					if (auto name = ctx.world->entityMngr.Get<Name>(var))
						ImGui::Button(name->value.c_str());
					else {
						std::string str = std::string("Entity (") + std::to_string(var.Idx()) + ")";
						ImGui::Button(str.c_str());
					}
				}
				else
					ImGui::Button("None (Entity)");
				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PlayloadType::ENTITY)) {
						IM_ASSERT(payload->DataSize == sizeof(UECS::Entity));
						const auto& payload_e = *(UECS::Entity*)payload->Data;
						var = payload_e;
					}
					ImGui::EndDragDropTarget();
				}
				ImGui::SameLine();
				ImGui::Text(GetFieldName(field).data());
			}
			else if constexpr (std::is_enum_v<Value>) {
				if constexpr (HasDefinition<USRefl::TypeInfo<Value>>::value) {
					std::string_view cur;
					USRefl::TypeInfo<Value>::fields.FindIf([&](auto field) {
						if (field.value == var) {
							cur = GetFieldName(field);
							return true;
						}
						return false;
					});

					if (ImGui::BeginCombo(GetFieldName(field).data(), cur.data())) {
						USRefl::TypeInfo<Value>::fields.ForEach([&](auto field) {
							bool isSelected = field.value == var;
							if (ImGui::Selectable(GetFieldName(field).data(), isSelected))
								var = field.value;

							if (isSelected)
								ImGui::SetItemDefaultFocus();

							});
						ImGui::EndCombo();
					}
				}
				else
					InspectVar(field, static_cast<std::underlying_type_t<Value>&>(var), ctx);
			}
			else if constexpr (is_instance_of_v<Value, std::shared_ptr> || is_instance_of_v<Value, USTL::shared_object>) {
				using Element = typename Value::element_type;
				static_assert(std::is_base_of_v<Object, Element>);
				ImGui::Text("(*)");
				ImGui::SameLine();
				// button
				if (var) {
					const auto& path = AssetMngr::Instance().GetAssetPath(*var);
					if (!path.empty()) {
						auto name = path.stem().string();
						ImGui::Button(name.c_str());
					}
					else
						ImGui::Button("UNKNOW");
				}
				else
					ImGui::Button("nullptr");

				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PlayloadType::GUID)) {
						IM_ASSERT(payload->DataSize == sizeof(xg::Guid));
						const auto& payload_guid = *(const xg::Guid*)payload->Data;
						const auto& path = AssetMngr::Instance().GUIDToAssetPath(payload_guid);
						assert(!path.empty());
						if (auto asset = AssetMngr::Instance().LoadAsset<Element>(path))
							var = asset;
					}
					ImGui::EndDragDropTarget();
				}

				ImGui::SameLine();
				ImGui::Text(GetFieldName(field).data());
			}
			else if constexpr (ArrayTraits<Value>::isArray && ValNTraits<Value>::isValN) {
				auto data = ArrayTraits_Data(var);
				constexpr auto N = ArrayTraits<Value>::size;

				if constexpr (ColorTraits<Value>::isColor) {
					constexpr ImGuiColorEditFlags flags =
						ImGuiColorEditFlags_Float
						| ImGuiColorEditFlags_HDR
						| ImGuiColorEditFlags_AlphaPreview;

					if constexpr (N == 3)
						ImGui::ColorEdit3(GetFieldName(field).data(), data, flags);
					else // N == 4
						ImGui::ColorEdit4(GetFieldName(field).data(), data, flags);
				}
				else {
					using ElemType = ArrayTraits_ValueType<Value>;
					if constexpr (std::is_same_v<ElemType, uint8_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_U8, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, uint16_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_U16, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, uint32_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_U32, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, uint64_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_U64, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, int8_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_S8, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, int16_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_S16, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, int32_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_S32, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, int64_t>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_S64, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, float>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_Float, data, N, 0.001f);
					else if constexpr (std::is_same_v<ElemType, double>)
						ImGui::DragScalarN(GetFieldName(field).data(), ImGuiDataType_Double, data, N, 0.001f);
				}
			}
			else
				static_assert(false);
			return orig != var;
		}
		else if constexpr (ArrayTraits<Value>::isArray
		|| TupleTraits<Value>::isTuple
		|| MapTraits<Value>::isMap
		|| OrderContainerTraits<Value>::isOrderContainer)
		{
			bool changed = false;
			if constexpr (ArrayTraits<Value>::isArray) {
				static_assert(!ValNTraits<Value>::isValN);
				if (ImGui::TreeNode(GetFieldName(field).data())) {
					ImGui::PushID(GetFieldName(field).data());
					for (size_t i = 0; i < ArrayTraits<Value>::size; i++) {
						auto str = std::to_string(i);
						if (InspectVar1(GenerateNameField(str), ArrayTraits_Get(var, i), ctx))
							changed = true;
					}
					ImGui::PopID();
					ImGui::TreePop();
				}
			}
			else if constexpr (TupleTraits<Value>::isTuple) {
				if (ImGui::TreeNode(GetFieldName(field).data())) {
					ImGui::PushID(GetFieldName(field).data());
					USTL::tuple_for_each(var, [ctx, idx = 0](auto& ele) mutable {
						auto str = std::to_string(idx++);
						if (InspectVar1(GenerateNameField(str), ele, ctx))
							changed = true;
					});
					ImGui::PopID();
					ImGui::TreePop();
				}
			}
			else if constexpr (MapTraits<Value>::isMap) {
				if (ImGui::TreeNode(GetFieldName(field).data())) {
					ImGui::PushID(GetFieldName(field).data());
					auto iter_begin = MapTraits_Begin(var);
					auto iter_end = MapTraits_End(var);
					for (auto iter = iter_begin; iter != iter_end; ++iter) {
						auto& [key, mapped] = *iter;
						if constexpr (std::is_same_v<std::decay_t<decltype(key)>, std::string>) {
							if (InspectVar1(GenerateNameField(key), mapped, ctx))
								changed = true;
						}
						else {
							auto name = std::to_string(key);
							if (InspectVar1(GenerateNameField(name), mapped, ctx))
								changed = true;
						}
					}
					ImGui::PopID();
					ImGui::TreePop();
				}
			}
			else if constexpr (OrderContainerTraits<Value>::isOrderContainer) {
				if (ImGui::TreeNode(GetFieldName(field).data())) {
					ImGui::PushID(GetFieldName(field).data());

					if constexpr (OrderContainerTraits<Value>::isResizable) {
						size_t origSize = OrderContainerTraits_Size(var);
						size_t size = origSize;
						static constexpr ImU64 u64_one = 1;
						ImGui::InputScalar("size", ImGuiDataType_U64, &size, &u64_one);
						if (size != origSize) {
							OrderContainerTraits_Resize(var, size);
							changed = true;
						}
					}

					auto iter_begin = OrderContainerTraits_Begin(var);
					auto iter_end = OrderContainerTraits_End(var);
					size_t idx = 0;
					for (auto iter = iter_begin; iter != iter_end; ++iter) {
						auto name = std::to_string(idx++);
						if (InspectVar1(GenerateNameField(name), *iter, ctx))
							changed = true;
					}
					ImGui::PopID();
					ImGui::TreePop();
				}
			}
			else
				static_assert(false);
			return changed;
		}
		else if constexpr (is_instance_of_v<Value, std::variant>)
			return InspectVariant1(field, var, std::make_index_sequence<std::variant_size_v<Value>>{}, ctx);
		else {
			Value orig;
			InspectUserType(field, &var, ctx);
			return orig != var;
		}
	}
}

namespace Ubpa::Utopia {
	template<typename... Cmpts>
	void InspectorRegistry::RegisterCmpts() {
		(InspectorRegistry::Instance().RegisterCmpt(&detail::InspectCmpt<Cmpts>), ...);
	}

	template<typename... Assets>
	void InspectorRegistry::RegisterAssets() {
		(InspectorRegistry::Instance().RegisterAsset(&detail::InspectAsset<Assets>), ...);
	}

	template<typename Func>
	void InspectorRegistry::RegisterCmpt(Func&& func) {
		using ArgList = FuncTraits_ArgList<Func>;
		static_assert(Length_v<ArgList> == 2);
		static_assert(std::is_same_v<At_t<ArgList, 1>, InspectContext>);
		using CmptPtr = At_t<ArgList, 0>;
		static_assert(std::is_pointer_v<CmptPtr>);
		using Cmpt = std::remove_pointer_t<CmptPtr>;
		static_assert(!std::is_const_v<Cmpt>);
		RegisterCmpt(UECS::CmptType::Of<Cmpt>, [f = std::forward<Func>(func)](void* cmpt, InspectContext ctx) {
			f(reinterpret_cast<Cmpt*>(cmpt), ctx);
		});
	}

	template<typename Func>
	void InspectorRegistry::RegisterAsset(Func&& func) {
		using ArgList = FuncTraits_ArgList<Func>;
		static_assert(Length_v<ArgList> == 2);
		static_assert(std::is_same_v<At_t<ArgList, 1>, InspectContext>);
		using AssetPtr = At_t<ArgList, 0>;
		static_assert(std::is_pointer_v<AssetPtr>);
		using Asset = std::remove_pointer_t<AssetPtr>;
		static_assert(!std::is_const_v<Asset>);
		RegisterAsset(typeid(Asset), [f = std::forward<Func>(func)](void* asset, InspectContext ctx) {
			f(reinterpret_cast<Asset*>(asset), ctx);
		});
	}
}
