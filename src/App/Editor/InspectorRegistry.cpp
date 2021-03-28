#include <Utopia/App/Editor/InspectorRegistry.h>

#include <UTemplate/Type.hpp>
#include <UDRefl/UDRefl.hpp>

#include <_deps/imgui/imgui.h>
#include <_deps/imgui/misc/cpp/imgui_stdlib.h>

#include <Utopia/Core/Components/Name.h>

#include <unordered_map>
#include <functional>

using namespace Ubpa::UDRefl;

using namespace Ubpa::Utopia;

struct InspectorRegistry::Impl {
	Visitor<void(void*, InspectContext)> inspector;
};

InspectorRegistry::InspectorRegistry() : pImpl{ new Impl } {}
InspectorRegistry::~InspectorRegistry() { delete pImpl; }

void InspectorRegistry::Register(TypeID type, std::function<void(void*, InspectContext)> cmptInspectFunc) {
	pImpl->inspector.Register(type.GetValue(), std::move(cmptInspectFunc));
}

bool InspectorRegistry::IsRegistered(TypeID type) const {
	return pImpl->inspector.IsRegistered(type.GetValue())
		|| UDRefl::Mngr.typeinfos.contains(UDRefl::Mngr.tregistry.Typeof(type));
}

bool InspectorRegistry::IsRegistered(Type type) const {
	return pImpl->inspector.IsRegistered(type.GetID().GetValue())
		|| UDRefl::Mngr.typeinfos.contains(type);
}

void InspectorRegistry::InspectComponent(const UECS::World* world, UECS::CmptPtr cmpt) {
	if (pImpl->inspector.IsRegistered(cmpt.Type().GetValue())) {
		pImpl->inspector.Visit(cmpt.Type().GetValue(), cmpt.Ptr(), InspectContext{ world, pImpl->inspector });
		return;
	}

	ImGui::PushID((const void*)cmpt.Type().GetValue());

	auto type = UDRefl::Mngr.tregistry.Typeof(cmpt.Type());
	if (ImGui::CollapsingHeader(type.GetName().data())) {
		InspectorRegistry::InspectContext ctx{ world, pImpl->inspector };
		for (const auto& [n, var] : VarRange{ ObjectView{type, cmpt.Ptr()} })
			InspectRecursively(n, var.GetType().GetID(), var.GetPtr(), ctx);
	}

	ImGui::PopID();
}

void InspectorRegistry::Inspect(const UECS::World* w, TypeID typeID, void* asset) {
	if (pImpl->inspector.IsRegistered(typeID.GetValue())) {
		pImpl->inspector.Visit(typeID.GetValue(), asset, InspectContext{ nullptr, pImpl->inspector });
		return;
	}

	auto type = UDRefl::Mngr.tregistry.Typeof(typeID);
	InspectorRegistry::InspectContext ctx{ w, pImpl->inspector };
	for (const auto& [n, var] : VarRange{ ObjectView{type, asset} })
		InspectRecursively(n, var.GetType().GetID(), var.GetPtr(), ctx);
}

void InspectorRegistry::InspectRecursively(std::string_view name, TypeID typeID, void* obj, InspectContext ctx) {
	if (ctx.inspector.IsRegistered(typeID.GetValue())) {
		ctx.inspector.Visit(typeID.GetValue(), obj, ctx);
		return;
	}

	auto type = UDRefl::Mngr.tregistry.Typeof(typeID);
	UDRefl::ObjectView objv{ type, obj };
	if (!type.Valid())
		return;

	if (type.IsConst()) {
		// TODO
		ImGui::Text(name.data());
	}
	else {
		if (type.IsArithmetic()) {
			switch (type.GetID().GetValue())
			{
			case TypeID_of<bool>.GetValue():
				ImGui::Checkbox(name.data(), &objv.As<bool>());
				break;
			case TypeID_of<std::int8_t>.GetValue():
				ImGui::DragScalarN(name.data(), ImGuiDataType_S8, obj, 1, 1.f);
				break;
			case TypeID_of<std::int16_t>.GetValue():
				ImGui::DragScalarN(name.data(), ImGuiDataType_S16, obj, 1, 1.f);
				break;
			case TypeID_of<std::int32_t>.GetValue():
				ImGui::DragScalarN(name.data(), ImGuiDataType_S32, obj, 1, 1.f);
				break;
			case TypeID_of<std::int64_t>.GetValue():
				ImGui::DragScalarN(name.data(), ImGuiDataType_S64, obj, 1, 1.f);
				break;
			case TypeID_of<std::uint8_t>.GetValue():
				ImGui::DragScalarN(name.data(), ImGuiDataType_U8, obj, 1, 1.f);
				break;
			case TypeID_of<std::uint16_t>.GetValue():
				ImGui::DragScalarN(name.data(), ImGuiDataType_U16, obj, 1, 1.f);
				break;
			case TypeID_of<std::uint32_t>.GetValue():
				ImGui::DragScalarN(name.data(), ImGuiDataType_U32, obj, 1, 1.f);
				break;
			case TypeID_of<std::uint64_t>.GetValue():
				ImGui::DragScalarN(name.data(), ImGuiDataType_U64, obj, 1, 1.f);
				break;
			case TypeID_of<float>.GetValue():
				ImGui::DragScalarN(name.data(), ImGuiDataType_Float, obj, 1, 0.1f);
				break;
			case TypeID_of<double>.GetValue():
				ImGui::DragScalarN(name.data(), ImGuiDataType_Double, obj, 1, 0.1f);
				break;
			default:
				assert(false);
				break;
			}
		}
		else if (auto attr = Mngr.GetTypeAttr(type, Type_of<ContainerType>); attr.GetType().Valid()) {
			ContainerType ct = attr.As<ContainerType>();
			switch (ct)
			{
			case Ubpa::UDRefl::ContainerType::Stack:
			case Ubpa::UDRefl::ContainerType::Queue:
			case Ubpa::UDRefl::ContainerType::PriorityQueue:
			case Ubpa::UDRefl::ContainerType::None:
				break;
			case Ubpa::UDRefl::ContainerType::Span:
			case Ubpa::UDRefl::ContainerType::Array: //TODO: valf4, ...
			case Ubpa::UDRefl::ContainerType::Deque:
			case Ubpa::UDRefl::ContainerType::ForwardList: // TODO: append
			case Ubpa::UDRefl::ContainerType::List:
			case Ubpa::UDRefl::ContainerType::MultiSet:
			case Ubpa::UDRefl::ContainerType::Map:
			case Ubpa::UDRefl::ContainerType::MultiMap:
			case Ubpa::UDRefl::ContainerType::RawArray:
			case Ubpa::UDRefl::ContainerType::Set:
			case Ubpa::UDRefl::ContainerType::UnorderedMap:
			case Ubpa::UDRefl::ContainerType::UnorderedMultiSet:
			case Ubpa::UDRefl::ContainerType::UnorderedMultiMap:
			case Ubpa::UDRefl::ContainerType::UnorderedSet:
			case Ubpa::UDRefl::ContainerType::Vector:
			{
				ImGui::PushID(objv.GetPtr());
				std::size_t i = 0;
				auto e = objv.end();
				for (auto iter = objv.begin(); iter != e; ++iter) {
					auto n = std::to_string(i++);
					auto v = (*iter).RemoveReference();
					InspectRecursively(n, v.GetType().GetID(), v.GetPtr(), ctx);
				}
				ImGui::PopID();
			}
				break;
			case Ubpa::UDRefl::ContainerType::Pair:
			case Ubpa::UDRefl::ContainerType::Tuple:
			{
				std::size_t size = objv.tuple_size();
				std::size_t i = 0;
				for (std::size_t i = 0; i < size; i++) {
					auto n = std::to_string(i++);
					auto v = objv.get(i).RemoveReference();
					InspectRecursively(n, v.GetType().GetID(), v.GetPtr(), ctx);
				}
			}
				break;
			case Ubpa::UDRefl::ContainerType::Variant:
			{
				auto v = objv.variant_visit_get().RemoveReference();
				InspectRecursively(name, v.GetType().GetID(), v.GetPtr(), ctx);
			}
				break;
			case Ubpa::UDRefl::ContainerType::Optional:
			{
				if (objv.has_value()) {
					auto v = objv.value().RemoveReference();
					InspectRecursively(name, v.GetType().GetID(), v.GetPtr(), ctx);
				}
				else
					ImGui::Text("null");
			}
				break;
			default:
				assert(false);
				break;
			}
		}
	}
}
