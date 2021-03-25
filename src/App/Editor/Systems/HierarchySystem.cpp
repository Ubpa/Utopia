#include <Utopia/App/Editor/Systems/HierarchySystem.h>

#include <Utopia/App/Editor/PlayloadType.h>

#include <Utopia/App/Editor/Components/Hierarchy.h>
#include <Utopia/App/Editor/Components/Inspector.h>

#include <Utopia/Core/Components/Children.h>
#include <Utopia/Core/Components/Parent.h>
#include <Utopia/Core/Components/Name.h>

#include <_deps/imgui/imgui.h>

using namespace Ubpa::Utopia;

namespace Ubpa::Utopia::detail {
	bool HierarchyMovable(const UECS::World* w, UECS::Entity dst, UECS::Entity src) {
		if (dst == src)
			return false;
		else if (auto p = w->entityMngr.ReadComponent<Parent>(dst))
			return HierarchyMovable(w, p->value, src);
		else
			return true;
	}

	void HierarchyPrintEntity(Hierarchy* hierarchy, UECS::Entity e, Inspector* inspector) {
		static constexpr ImGuiTreeNodeFlags nodeBaseFlags =
			ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_OpenOnDoubleClick
			| ImGuiTreeNodeFlags_SpanAvailWidth;

		auto children = hierarchy->world->entityMngr.ReadComponent<Children>(e);
		auto name = hierarchy->world->entityMngr.ReadComponent<Name>(e);
		bool isLeaf = !children || children->value.empty();

		ImGuiTreeNodeFlags nodeFlags = nodeBaseFlags;
		if (hierarchy->select == e)
			nodeFlags |= ImGuiTreeNodeFlags_Selected;
		if (isLeaf)
			nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

		bool nodeOpen = name ?
			ImGui::TreeNodeEx((void*)(intptr_t)e.index, nodeFlags, "%s (%d)", name->value.c_str(), e.index)
			: ImGui::TreeNodeEx((void*)(intptr_t)e.index, nodeFlags, "Entity (%d)", e.index);

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
			ImGui::SetDragDropPayload(PlayloadType::ENTITY, &e, sizeof(UECS::Entity));
			if (name)
				ImGui::Text("%s (%d)", name->value.c_str(), e.index);
			else
				ImGui::Text("Entity (%d)", e.index);

			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PlayloadType::ENTITY)) {
				IM_ASSERT(payload->DataSize == sizeof(UECS::Entity));
				auto payload_e = *(const UECS::Entity*)payload->Data;
				if (HierarchyMovable(hierarchy->world, e, payload_e)) {
					hierarchy->world->entityMngr.Attach(payload_e, TypeIDs_of<Parent>);
					auto payload_e_p = hierarchy->world->entityMngr.WriteComponent<Parent>(payload_e);
					if (payload_e_p->value.Valid()) {
						auto parentChildren = hierarchy->world->entityMngr.WriteComponent<Children>(payload_e_p->value);
						parentChildren->value.erase(payload_e);
					}
					payload_e_p->value = e;
					hierarchy->world->entityMngr.Attach(e, TypeIDs_of<Children>);
					auto children = hierarchy->world->entityMngr.WriteComponent<Children>(e);
					children->value.insert(payload_e);
				}
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::IsItemHovered() && ImGui::IsItemDeactivated()) {
			hierarchy->select = e;
			if (inspector && !inspector->lock) {
				inspector->mode = Inspector::Mode::Entity;
				inspector->entity = e;
			}
		}
		if (ImGui::IsItemHovered())
			hierarchy->hover = e;

		if (nodeOpen && !isLeaf) {
			for (const auto& child : children->value)
				HierarchyPrintEntity(hierarchy, child, inspector);
			ImGui::TreePop();
		}
	}
	
	// delete e and his children
	void HierarchyDeleteEntityRecursively(UECS::World* w, UECS::Entity e) {
		if (auto children = w->entityMngr.ReadComponent<Children>(e)) {
			for (const auto& child : children->value)
				HierarchyDeleteEntityRecursively(w, child);
		}
		w->entityMngr.Destroy(e);
	}
	
	// delete e in it's parent
	// then call HierarchyDeleteEntityRecursively
	void HierarchyDeleteEntity(UECS::World* w, UECS::Entity e) {
		if (auto p = w->entityMngr.ReadComponent<Parent>(e)) {
			auto e_p = p->value;
			auto children = w->entityMngr.WriteComponent<Children>(e_p);
			children->value.erase(e);
		}
		detail::HierarchyDeleteEntityRecursively(w, e);
	}
}

void HierarchySystem::OnUpdate(UECS::Schedule& schedule) {
	schedule.GetWorld()->AddCommand([w = schedule.GetWorld()]() {
		auto hierarchy = w->entityMngr.WriteSingleton<Hierarchy>();
		if (!hierarchy)
			return;

		if (ImGui::Begin("Hierarchy")) {
			if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
				ImGui::OpenPopup("Hierarchy_popup");

			if (ImGui::BeginPopup("Hierarchy_popup")) {
				if (hierarchy->hover.Valid()) {
					if (ImGui::MenuItem("Create Empty")) {
						auto e = hierarchy->world->entityMngr.Create(TypeIDs_of<Parent>);
						auto p = hierarchy->world->entityMngr.WriteComponent<Parent>(e);
						p->value = hierarchy->hover;
						hierarchy->world->entityMngr.Attach(hierarchy->hover, TypeIDs_of<Children>);
						auto children = hierarchy->world->entityMngr.WriteComponent<Children>(hierarchy->hover);
						children->value.insert(e);
					}

					if (ImGui::MenuItem("Delete")) {
						detail::HierarchyDeleteEntity(hierarchy->world, hierarchy->hover);

						hierarchy->hover = UECS::Entity::Invalid();
						if (!hierarchy->world->entityMngr.Exist(hierarchy->select))
							hierarchy->select = UECS::Entity::Invalid();
					}
				}
				else {
					if (ImGui::MenuItem("Create Empty Entity"))
						hierarchy->world->entityMngr.Create();
				}

				ImGui::EndPopup();
			}
			else
				hierarchy->hover = UECS::Entity::Invalid();

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PlayloadType::ENTITY)) {
					IM_ASSERT(payload->DataSize == sizeof(UECS::Entity));
					auto payload_e = *(const UECS::Entity*)payload->Data;

					if (auto payload_e_p = hierarchy->world->entityMngr.ReadComponent<Parent>(payload_e)) {
						auto parentChildren = hierarchy->world->entityMngr.WriteComponent<Children>(payload_e_p->value);
						parentChildren->value.erase(payload_e);
						hierarchy->world->entityMngr.Detach(payload_e, TypeIDs_of<Parent>);
					}
				}
				ImGui::EndDragDropTarget();
			}

			auto inspector = w->entityMngr.WriteSingleton<Inspector>();
			UECS::ArchetypeFilter filter;
			filter.none = { TypeID_of<Parent> };
			hierarchy->world->RunEntityJob(
				[=](UECS::Entity e) {
					detail::HierarchyPrintEntity(hierarchy, e, inspector);
				},
				false,
				filter
			);
		}
		ImGui::End();
	}, 0);
}
