#pragma once

#include "../PlayloadType.h"

#include <DustEngine/Asset/AssetMngr.h>

#include <DustEngine/_deps/imgui/imgui.h>
#include <DustEngine/_deps/imgui/misc/cpp/imgui_stdlib.h>

#include <DustEngine/Core/Traits.h>

#include <USTL/tuple.h>

namespace Ubpa::DustEngine::detail {
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

	template<typename Field, typename Value>
	void InspectVar(Field field, Value& var, CmptInspector::InspectContext ctx);

	template<typename Cmpt>
	void InspectCmpt(Cmpt* cmpt, CmptInspector::InspectContext ctx) {
		if constexpr (HasDefinition<USRefl::TypeInfo<Cmpt>>::value) {
			ImGui::PushID(UECS::CmptType::Of<Cmpt>.HashCode());
			if (ImGui::CollapsingHeader(USRefl::TypeInfo<Cmpt>::name.data())) {
				USRefl::TypeInfo<Cmpt>::ForEachVarOf(*cmpt, [ctx](auto field, auto& var) {
					InspectVar(field, var, ctx);
				});
			}
			ImGui::PopID();
		}
		else {
			if (ctx.inspector.IsRegistered(UECS::CmptType::Of<Cmpt>.HashCode()))
				ctx.inspector.Visit(UECS::CmptType::Of<Cmpt>.HashCode(), cmpt, ctx);
		}
	}

	template<typename Field, typename UserType>
	void InspectUserType(Field field, UserType* obj, CmptInspector::InspectContext ctx) {
		if constexpr (HasDefinition<USRefl::TypeInfo<UserType>>::value) {
			if (ImGui::TreeNode(field.name.data())) {
				ImGui::PushID(GetID<UserType>());
				USRefl::TypeInfo<UserType>::ForEachVarOf(*obj, [ctx](auto field, auto& var) {
					InspectVar(field, var, ctx);
				});
				ImGui::TreePop();
				ImGui::PopID();
			}
		}
		else
			ImGui::Text(field.name.data());
	}

	template<typename Field, typename Value>
	void InspectVar(Field field, const Value& var, CmptInspector::InspectContext ctx) {
		if constexpr (std::is_same_v<Value, bool>) {
			ImGui::Button(var ? "true" : "false", &var);
			ImGui::SameLine();
			ImGui::Text(field.name.data());
		}
		else if constexpr (std::is_arithmetic_v<Value>) {
			auto value = std::to_string(var);
			ImGui::Button(value.c_str());
			ImGui::SameLine();
			ImGui::Text(field.name.data());
		}
		else if constexpr (std::is_same_v<Value, std::string>) {
			ImGui::Button(var.c_str());
			ImGui::SameLine();
			ImGui::Text(field.name.data());
		}
		else if constexpr (std::is_pointer_v<Value>) {
			ImGui::Text("(*)");
			ImGui::SameLine();
			// button
			if (var) {
				const auto& path = AssetMngr::Instance().GetAssetPath(var);
				if (!path.empty()) {
					auto name = path.stem().string();
					ImGui::Button(name.c_str());
				}
				else
					ImGui::Button("UNKNOW");
			}
			else
				ImGui::Button("nullptr");

			ImGui::SameLine();
			ImGui::Text(field.name.data());
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
						ImGui::ColorEdit3(field.name.data(), data, flags);
					else // N == 4
						ImGui::ColorEdit4(field.name.data(), data, flags);
				}
				else {
					using ElemType = ArrayTraits_ValueType<Value>;
					if constexpr (std::is_same_v<ElemType, uint8_t>)
						ImGui::DragScalarN(field.name.data(), ImGuiDataType_U8, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, uint16_t>)
						ImGui::DragScalarN(field.name.data(), ImGuiDataType_U16, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, uint32_t>)
						ImGui::DragScalarN(field.name.data(), ImGuiDataType_U32, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, uint64_t>)
						ImGui::DragScalarN(field.name.data(), ImGuiDataType_U64, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, int8_t>)
						ImGui::DragScalarN(field.name.data(), ImGuiDataType_S8, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, int16_t>)
						ImGui::DragScalarN(field.name.data(), ImGuiDataType_S16, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, int32_t>)
						ImGui::DragScalarN(field.name.data(), ImGuiDataType_S32, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, int64_t>)
						ImGui::DragScalarN(field.name.data(), ImGuiDataType_S64, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, float>)
						ImGui::DragScalarN(field.name.data(), ImGuiDataType_Float, data, N, 0.001f);
					else if constexpr (std::is_same_v<ElemType, double>)
						ImGui::DragScalarN(field.name.data(), ImGuiDataType_Double, data, N, 0.001f);
				}
			}
			else {
				if (ImGui::TreeNode(field.name.data())) {
					ImGui::PushID(field.name.data());
					for (size_t i = 0; i < ArrayTraits<Value>::size; i++) {
						auto str = std::to_string(i);
						InspectVar(USRefl::Field{ std::string_view{str}, (void*)0 }, ArrayTraits_Get(var, i), ctx);
					}
					ImGui::PopID();
					ImGui::TreePop();
				}
			}
		}
		else if constexpr (TupleTraits<Value>::isTuple) {
			if (ImGui::TreeNode(field.name.data())) {
				ImGui::PushID(field.name.data());
				USTL::tuple_for_each(var, [ctx, idx = 0](auto& ele) mutable {
					auto str = std::to_string(idx++);
					InspectVar(USRefl::Field{ std::string_view{str}, (void*)0 }, ele, ctx);
				});
				ImGui::PopID();
				ImGui::TreePop();
			}
		}
		else if constexpr (MapTraits<Value>::isMap) {
			if (ImGui::TreeNode(field.name.data())) {
				ImGui::PushID(field.name.data());
				auto iter_begin = MapTraits_Begin(var);
				auto iter_end = MapTraits_End(var);
				for (auto iter = iter_begin; iter != iter_end; ++iter) {
					auto& [key, mapped] = *iter;
					if constexpr (std::is_same_v<std::decay_t<decltype(key)>, std::string>)
						InspectVar(USRefl::Field{ std::string_view{key}, (void*)0 }, mapped, ctx);
					else {
						auto name = std::to_string(key);
						InspectVar(USRefl::Field{ std::string_view{name}, (void*)0 }, mapped, ctx);
					}
				}
				ImGui::PopID();
				ImGui::TreePop();
			}
		}
		else if constexpr (OrderContainerTraits<Value>::isOrderContainer) {
			if (ImGui::TreeNode(field.name.data())) {
				ImGui::PushID(field.name.data());
				auto iter_begin = OrderContainerTraits_Begin(var);
				auto iter_end = OrderContainerTraits_End(var);
				size_t idx = 0;
				for (auto iter = iter_begin; iter != iter_end; ++iter) {
					auto name = std::to_string(idx++);
					InspectVar(USRefl::Field{ std::string_view{name}, (void*)0 }, *iter, ctx);
				}
				ImGui::PopID();
				ImGui::TreePop();
			}
		}
		else {
			assert(false);
			//InspectUserType(field, &var, ctx);
		}
	}

	template<typename Field, typename Value>
	void InspectVar(Field field, Value& var, CmptInspector::InspectContext ctx) {
		//static_assert(!std::is_const_v<Value>);
		if constexpr (std::is_same_v<Value, bool>)
			ImGui::Checkbox(field.name.data(), &var);
		else if constexpr (std::is_same_v<Value, uint8_t>)
			ImGui::DragScalar(field.name.data(), ImGuiDataType_U8, &var, 1.f);
		else if constexpr (std::is_same_v<Value, uint16_t>)
			ImGui::DragScalar(field.name.data(), ImGuiDataType_U16, &var, 1.f);
		else if constexpr (std::is_same_v<Value, uint32_t>)
			ImGui::DragScalar(field.name.data(), ImGuiDataType_U32, &var, 1.f);
		else if constexpr (std::is_same_v<Value, uint64_t>)
			ImGui::DragScalar(field.name.data(), ImGuiDataType_U64, &var, 1.f);
		else if constexpr (std::is_same_v<Value, int8_t>)
			ImGui::DragScalar(field.name.data(), ImGuiDataType_S8, &var, 1.f);
		else if constexpr (std::is_same_v<Value, int16_t>)
			ImGui::DragScalar(field.name.data(), ImGuiDataType_S16, &var, 1.f);
		else if constexpr (std::is_same_v<Value, int32_t>)
			ImGui::DragScalar(field.name.data(), ImGuiDataType_S32, &var, 1.f);
		else if constexpr (std::is_same_v<Value, int64_t>)
			ImGui::DragScalar(field.name.data(), ImGuiDataType_S64, &var, 1.f);
		else if constexpr (std::is_same_v<Value, float>)
			ImGui::DragFloat(field.name.data(), &var, 0.001f);
		else if constexpr (std::is_same_v<Value, double>)
			ImGui::DragScalar(field.name.data(), ImGuiDataType_Double, &var, 0.001f);
		else if constexpr (std::is_same_v<Value, std::string>)
			ImGui::InputText(field.name.data(), &var);
		else if constexpr (std::is_pointer_v<Value>) {
			using Type = std::remove_pointer_t<Value>;
			ImGui::Text("(*)");
			ImGui::SameLine();
			// button
			if (var) {
				const auto& path = AssetMngr::Instance().GetAssetPath(var);
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
					if (auto asset = AssetMngr::Instance().LoadAsset<std::decay_t<Type>>(path))
						var = asset;
				}
				ImGui::EndDragDropTarget();
			}
			
			ImGui::SameLine();
			ImGui::Text(field.name.data());
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
						ImGui::ColorEdit3(field.name.data(), data, flags);
					else // N == 4
						ImGui::ColorEdit4(field.name.data(), data, flags);
				}
				else {
					using ElemType = ArrayTraits_ValueType<Value>;
					if constexpr (std::is_same_v<ElemType, uint8_t>)
						ImGui::DragScalarN(field.name.data(), ImGuiDataType_U8, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, uint16_t>)
						ImGui::DragScalarN(field.name.data(), ImGuiDataType_U16, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, uint32_t>)
						ImGui::DragScalarN(field.name.data(), ImGuiDataType_U32, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, uint64_t>)
						ImGui::DragScalarN(field.name.data(), ImGuiDataType_U64, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, int8_t>)
						ImGui::DragScalarN(field.name.data(), ImGuiDataType_S8, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, int16_t>)
						ImGui::DragScalarN(field.name.data(), ImGuiDataType_S16, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, int32_t>)
						ImGui::DragScalarN(field.name.data(), ImGuiDataType_S32, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, int64_t>)
						ImGui::DragScalarN(field.name.data(), ImGuiDataType_S64, data, N, 1.f);
					else if constexpr (std::is_same_v<ElemType, float>)
						ImGui::DragScalarN(field.name.data(), ImGuiDataType_Float, data, N, 0.001f);
					else if constexpr (std::is_same_v<ElemType, double>)
						ImGui::DragScalarN(field.name.data(), ImGuiDataType_Double, data, N, 0.001f);
				}
			}
			else {
				if (ImGui::TreeNode(field.name.data())) {
					ImGui::PushID(field.name.data());
					for (size_t i = 0; i < ArrayTraits<Value>::size; i++) {
						auto str = std::to_string(i);
						InspectVar(USRefl::Field{ std::string_view{str}, (void*)0 }, ArrayTraits_Get(var, i), ctx);
					}
					ImGui::PopID();
					ImGui::TreePop();
				}
			}
		}
		else if constexpr (TupleTraits<Value>::isTuple) {
			if (ImGui::TreeNode(field.name.data())) {
				ImGui::PushID(field.name.data());
				USTL::tuple_for_each(var, [ctx, idx = 0](auto& ele) mutable {
					auto str = std::to_string(idx++);
					InspectVar(USRefl::Field{ std::string_view{str}, (void*)0 }, ele, ctx);
				});
				ImGui::PopID();
				ImGui::TreePop();
			}
		}
		else if constexpr (MapTraits<Value>::isMap) {
			if (ImGui::TreeNode(field.name.data())) {
				ImGui::PushID(field.name.data());
				auto iter_begin = MapTraits_Begin(var);
				auto iter_end = MapTraits_End(var);
				for (auto iter = iter_begin; iter != iter_end; ++iter) {
					auto& [key, mapped] = *iter;
					if constexpr (std::is_same_v<std::decay_t<decltype(key)>, std::string>)
						InspectVar(USRefl::Field{ std::string_view{key}, (void*)0 }, mapped, ctx);
					else {
						auto name = std::to_string(key);
						InspectVar(USRefl::Field{ std::string_view{name}, (void*)0 }, mapped, ctx);
					}
				}
				ImGui::PopID();
				ImGui::TreePop();
			}
		}
		else if constexpr (OrderContainerTraits<Value>::isOrderContainer) {
			if (ImGui::TreeNode(field.name.data())) {
				ImGui::PushID(field.name.data());
				auto iter_begin = OrderContainerTraits_Begin(var);
				auto iter_end = OrderContainerTraits_End(var);
				size_t idx = 0;
				for (auto iter = iter_begin; iter != iter_end; ++iter) {
					auto name = std::to_string(idx++);
					InspectVar(USRefl::Field{ std::string_view{name}, (void*)0 }, *iter, ctx);
				}
				ImGui::PopID();
				ImGui::TreePop();
			}
		}
		else {
			InspectUserType(field, &var, ctx);
		}
	}
}

namespace Ubpa::DustEngine {
	template<typename... Cmpts>
	void CmptInspector::RegisterCmpts() {
		(CmptInspector::Instance().RegisterCmpt(&detail::InspectCmpt<Cmpts>), ...);
	}

	template<typename Func>
	void CmptInspector::RegisterCmpt(Func&& func) {
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
}
