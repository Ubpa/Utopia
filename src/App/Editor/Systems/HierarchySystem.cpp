#include "HierarchySystem.h"

#include "../Components/Hierarchy.h"

#include <DustEngine/Transform/Components/Components.h>

#include <DustEngine/_deps/imgui/imgui.h>

using namespace Ubpa::DustEngine;

namespace Ubpa::DustEngine::detail {
	void HierarchyPrintEntity(Hierarchy* hierarchy, UECS::Entity e) {
		static constexpr ImGuiTreeNodeFlags nodeBaseFlags =
			ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_OpenOnDoubleClick
			| ImGuiTreeNodeFlags_SpanAvailWidth;

		ImGuiTreeNodeFlags nodeFlags = nodeBaseFlags;
		if (hierarchy->select == e)
			nodeFlags |= ImGuiTreeNodeFlags_Selected;

		auto children = hierarchy->world->entityMngr.Get<Children>(e);

		if (children) {
			bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)e.Idx(), nodeFlags, "Entity %d", e.Idx());
			if (ImGui::IsItemClicked())
				hierarchy->select = e;
			if (nodeOpen) {
				for (const auto& child : children->value)
					HierarchyPrintEntity(hierarchy, child);
				ImGui::TreePop();
			}
		}
		else {
			nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
			ImGui::TreeNodeEx((void*)(intptr_t)e.Idx(), nodeFlags, "Entity %d", e.Idx());
			if (ImGui::IsItemClicked())
				hierarchy->select = e;
		}
	}
}

void HierarchySystem::OnUpdate(UECS::Schedule& schedule) {
	GetWorld()->AddCommand([](UECS::World* w) {
		auto hierarchy = w->entityMngr.GetSingleton<Hierarchy>();
		if (!hierarchy)
			return;
		if (ImGui::Begin("Hierarchy")) {

			UECS::ArchetypeFilter filter;
			filter.none = { UECS::CmptType::Of<Parent> };
			const_cast<UECS::World*>(hierarchy->world)->RunEntityJob(
				[=](UECS::Entity e) { detail::HierarchyPrintEntity(hierarchy, e); },
				false,
				filter
			);
		}
		ImGui::End();
	});
}
