#include "HierarchySystem.h"

#include "../PlayloadType.h"

#include "../Components/Hierarchy.h"

#include <DustEngine/Transform/Components/Components.h>
#include <DustEngine/Core/Components/Name.h>

#include <_deps/imgui/imgui.h>

using namespace Ubpa::DustEngine;

namespace Ubpa::DustEngine::detail {
	bool HierarchyMovable(const UECS::World* w, UECS::Entity dst, UECS::Entity src) {
		if (dst == src)
			return false;
		else if (auto p = w->entityMngr.Get<Parent>(dst))
			return HierarchyMovable(w, p->value, src);
		else
			return true;
	}

	void HierarchyPrintEntity(Hierarchy* hierarchy, UECS::Entity e) {
		static constexpr ImGuiTreeNodeFlags nodeBaseFlags =
			ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_OpenOnDoubleClick
			| ImGuiTreeNodeFlags_SpanAvailWidth;

		auto children = hierarchy->world->entityMngr.Get<Children>(e);
		auto name = hierarchy->world->entityMngr.Get<Name>(e);
		bool isLeaf = !children || children->value.empty();

		ImGuiTreeNodeFlags nodeFlags = nodeBaseFlags;
		if (hierarchy->select == e)
			nodeFlags |= ImGuiTreeNodeFlags_Selected;
		if (isLeaf)
			nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

		bool nodeOpen = name ?
			ImGui::TreeNodeEx((void*)(intptr_t)e.Idx(), nodeFlags, "%s (%d)", name->value.c_str(), e.Idx())
			: ImGui::TreeNodeEx((void*)(intptr_t)e.Idx(), nodeFlags, "Entity (%d)", e.Idx());

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
			ImGui::SetDragDropPayload(PlayloadType::ENTITY, &e, sizeof(UECS::Entity));
			if (name)
				ImGui::Text("%s (%d)", name->value.c_str(), e.Idx());
			else
				ImGui::Text("Entity (%d)", e.Idx());

			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PlayloadType::ENTITY)) {
				IM_ASSERT(payload->DataSize == sizeof(UECS::Entity));
				auto payload_e = *(const UECS::Entity*)payload->Data;
				if (HierarchyMovable(hierarchy->world, e, payload_e)) {
					auto [payload_e_p] = hierarchy->world->entityMngr.Attach<Parent>(payload_e);
					if (payload_e_p->value.Valid()) {
						auto parentChildren = hierarchy->world->entityMngr.Get<Children>(payload_e_p->value);
						parentChildren->value.erase(payload_e);
					}
					payload_e_p->value = e;
					auto [children] = hierarchy->world->entityMngr.Attach<Children>(e);
					children->value.insert(payload_e);
				}
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::IsItemClicked())
			hierarchy->select = e;
		if (ImGui::IsItemHovered())
			hierarchy->hover = e;

		if (nodeOpen && !isLeaf) {
			for (const auto& child : children->value)
				HierarchyPrintEntity(hierarchy, child);
			ImGui::TreePop();
		}
	}
	
	// delete e and his children
	void HierarchyDeleteEntityRecursively(UECS::World* w, UECS::Entity e) {
		if (auto children = w->entityMngr.Get<Children>(e)) {
			for (const auto& child : children->value)
				HierarchyDeleteEntityRecursively(w, child);
		}
		w->entityMngr.Destroy(e);
	}
	
	// delete e in it's parent
	// then call HierarchyDeleteEntityRecursively
	void HierarchyDeleteEntity(UECS::World* w, UECS::Entity e) {
		if (auto p = w->entityMngr.Get<Parent>(e)) {
			auto e_p = p->value;
			auto children = w->entityMngr.Get<Children>(e_p);
			children->value.erase(e);
		}
		detail::HierarchyDeleteEntityRecursively(w, e);
	}
}

void HierarchySystem::OnUpdate(UECS::Schedule& schedule) {
	schedule.RegisterJob([](UECS::World* w, UECS::Latest<UECS::Singleton<Hierarchy>> hierarchy) {
		w->AddCommand([hierarchy = const_cast<Hierarchy*>(hierarchy.Get())](UECS::World* w) {
			if (ImGui::Begin("Hierarchy")) {
				if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImGui::IsWindowHovered())
					ImGui::OpenPopup("Hierarchy_popup");

				if (ImGui::BeginPopup("Hierarchy_popup")) {
					if (hierarchy->hover.Valid()) {
						if (ImGui::MenuItem("Create Empty")) {
							auto [e, p] = hierarchy->world->entityMngr.Create<Parent>();
							p->value = hierarchy->hover;
							auto [children] = hierarchy->world->entityMngr.Attach<Children>(hierarchy->hover);
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

						if (auto payload_e_p = hierarchy->world->entityMngr.Get<Parent>(payload_e)) {
							auto parentChildren = hierarchy->world->entityMngr.Get<Children>(payload_e_p->value);
							parentChildren->value.erase(payload_e);
							hierarchy->world->entityMngr.Detach(payload_e, &UECS::CmptType::Of<Parent>, 1);
						}
					}
					ImGui::EndDragDropTarget();
				}

				UECS::ArchetypeFilter filter;
				filter.none = { UECS::CmptType::Of<Parent> };
				hierarchy->world->RunEntityJob(
					[=](UECS::Entity e) {
						detail::HierarchyPrintEntity(hierarchy, e);
					},
					false,
					filter
				);
			}
			ImGui::End();
		});
	}, "HierarchySystem");
}
