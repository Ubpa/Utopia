#include <Utopia/App/Editor/Systems/HierarchySystem.h>

#include <Utopia/App/Editor/InspectorRegistry.h>

#include <Utopia/App/Editor/Components/Hierarchy.h>
#include <Utopia/App/Editor/Components/Inspector.h>

#include <Utopia/Core/Components/Children.h>
#include <Utopia/Core/Components/Input.h>
#include <Utopia/Core/Components/Parent.h>
#include <Utopia/Core/Components/Name.h>
#include <Utopia/Core/AssetMngr.h>
#include <Utopia/Core/WorldAssetImporter.h>

#include <_deps/imgui/imgui.h>
#include <_deps/imgui/misc/cpp/imgui_stdlib.h>

using namespace Ubpa::Utopia;

namespace Ubpa::Utopia::details {
	bool HierarchyMovable(const UECS::World* w, UECS::Entity dst, UECS::Entity src) {
		if (dst == src)
			return false;
		else if (auto p = w->entityMngr.ReadComponent<Parent>(dst))
			return HierarchyMovable(w, p->value, src);
		else
			return true;
	}

	void HierarchyPrintEntity(Hierarchy* hierarchy, UECS::Entity e, Inspector* inspector, const Input* input) {
		static constexpr ImGuiTreeNodeFlags nodeBaseFlags =
			ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_OpenOnDoubleClick
			| ImGuiTreeNodeFlags_SpanAvailWidth;

		auto children = hierarchy->world->entityMngr.ReadComponent<Children>(e);
		auto name = hierarchy->world->entityMngr.ReadComponent<Name>(e);
		bool isLeaf = !children || children->value.empty();

		ImGuiTreeNodeFlags nodeFlags = nodeBaseFlags;
		if (hierarchy->selecties.contains(e))
			nodeFlags |= ImGuiTreeNodeFlags_Selected;
		if (isLeaf)
			nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

		bool nodeOpen = name ?
			ImGui::TreeNodeEx((void*)(intptr_t)e.index, nodeFlags, "%s (%d)", name->value.c_str(), e.index)
			: ImGui::TreeNodeEx((void*)(intptr_t)e.index, nodeFlags, "Entity (%d)", e.index);

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
			ImGui::SetDragDropPayload(InspectorRegistry::Playload::Entity, &e, sizeof(UECS::Entity));
			if (name)
				ImGui::Text("%s (%d)", name->value.c_str(), e.index);
			else
				ImGui::Text("Entity (%d)", e.index);

			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(InspectorRegistry::Playload::Entity)) {
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

		if (ImGui::IsItemClicked()) {
			if (input && input->KeyCtrl) {
				auto target = hierarchy->selecties.find(e);
				if (target == hierarchy->selecties.end())
					hierarchy->selecties.insert(target, e);
				else
					hierarchy->selecties.erase(target);
			}
			else
				hierarchy->selecties = { e };

			if (inspector && !inspector->lock) {
				inspector->mode = Inspector::Mode::Entity;
				inspector->entity = e;
			}
		}
		if (ImGui::IsItemHovered())
			hierarchy->hover = e;

		if (nodeOpen && !isLeaf) {
			for (const auto& child : children->value)
				HierarchyPrintEntity(hierarchy, child, inspector, input);
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
		details::HierarchyDeleteEntityRecursively(w, e);
	}
}

void HierarchySystem::OnUpdate(UECS::Schedule& schedule) {
	schedule.GetWorld()->AddCommand([w = schedule.GetWorld()]() {
		auto hierarchy = w->entityMngr.WriteSingleton<Hierarchy>();
		auto input = w->entityMngr.ReadSingleton<Input>();
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
						hierarchy->selecties.insert(hierarchy->hover);

						for (const auto& e : hierarchy->selecties) {
							if (hierarchy->world->entityMngr.Exist(e)) {
								if (hierarchy->world->entityMngr.Have(e, TypeID_of<Parent>)) {
									UECS::Entity p = hierarchy->world->entityMngr.ReadComponent<Parent>(e)->value;
									hierarchy->world->entityMngr.WriteComponent<Children>(p)->value.erase(e);
								}
								details::HierarchyDeleteEntityRecursively(hierarchy->world, e);
							}
						}
					}

					if (ImGui::MenuItem("Save Entities"))
						hierarchy->is_saving_entities = true;

					if (ImGui::MenuItem("Duplicate")) {
						std::vector<UECS::Entity> entities{ hierarchy->selecties.begin(),hierarchy->selecties.end() };
						WorldAsset worldasset(hierarchy->world, entities);
						worldasset.ToWorld(hierarchy->world);
					}
				}
				else {
					if (ImGui::MenuItem("Create Empty Entity"))
						hierarchy->world->entityMngr.Create();

					if (ImGui::MenuItem("Save World"))
						hierarchy->is_saving_world = true;

					if (ImGui::MenuItem("Delete All Entities"))
						hierarchy->world->entityMngr.Clear();
				}

				ImGui::EndPopup();
			}
			else
				hierarchy->hover = UECS::Entity::Invalid();

			if (hierarchy->is_saving_world || hierarchy->is_saving_entities)
				ImGui::OpenPopup("Input Saved Path");

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(InspectorRegistry::Playload::Entity)) {
					IM_ASSERT(payload->DataSize == sizeof(UECS::Entity));
					auto payload_e = *(const UECS::Entity*)payload->Data;

					if (auto payload_e_p = hierarchy->world->entityMngr.ReadComponent<Parent>(payload_e)) {
						auto parentChildren = hierarchy->world->entityMngr.WriteComponent<Children>(payload_e_p->value);
						parentChildren->value.erase(payload_e);
						hierarchy->world->entityMngr.Detach(payload_e, TypeIDs_of<Parent>);
					}
				}
				else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(InspectorRegistry::Playload::Asset)) {
					IM_ASSERT(payload->DataSize == sizeof(InspectorRegistry::Playload::AssetHandle));
					auto asset_handle = *(const InspectorRegistry::Playload::AssetHandle*)payload->Data;
					UDRefl::SharedObject asset = asset_handle.name.empty() ?
						AssetMngr::Instance().GUIDToMainAsset(asset_handle.guid)
						: AssetMngr::Instance().GUIDToAsset(asset_handle.guid, asset_handle.name);
					if (asset.GetType().Is<WorldAsset>()) {
						if(hierarchy->world)
							asset.As<WorldAsset>().ToWorld(hierarchy->world);
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (ImGui::BeginPopupModal("Input Saved Path", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::InputText("path", &hierarchy->saved_path);
				if (ImGui::Button("OK", ImVec2(120, 0))) {
					std::filesystem::path path{ hierarchy->saved_path };
					if (path.extension() != LR"(.world)")
						path += LR"(.world)";
					if (hierarchy->is_saving_world) {
						AssetMngr::Instance().CreateAsset(std::make_shared<WorldAsset>(hierarchy->world), path);
						hierarchy->is_saving_world = false;
					}
					else if (hierarchy->is_saving_entities) {
						std::vector<UECS::Entity> entities{hierarchy->selecties.begin(),hierarchy->selecties.end() };
						AssetMngr::Instance().CreateAsset(std::make_shared<WorldAsset>(hierarchy->world, entities), path);
						hierarchy->is_saving_entities = false;
					}
					else
						assert(false);
					hierarchy->saved_path.clear();
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::Button("Cancel", ImVec2(120, 0))) {
					hierarchy->is_saving_world = false;
					hierarchy->is_saving_entities = false;
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			auto inspector = w->entityMngr.WriteSingleton<Inspector>();
			UECS::ArchetypeFilter filter;
			filter.none = { TypeID_of<Parent> };
			hierarchy->world->RunEntityJob(
				[=](UECS::Entity e) {
					details::HierarchyPrintEntity(hierarchy, e, inspector, input);
				},
				false,
				filter
			);

		}
		ImGui::End();
	}, 0);
}
