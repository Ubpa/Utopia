#include "HierarchySystem.h"

#include "../Components/Hierarchy.h"

#include <DustEngine/Transform/Components/Components.h>
#include <DustEngine/Core/Components/Name.h>

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

		auto name = hierarchy->world->entityMngr.Get<Name>(e);
		auto children = hierarchy->world->entityMngr.Get<Children>(e);

		if (children) {
			bool nodeOpen = name ? ImGui::TreeNodeEx((void*)(intptr_t)e.Idx(), nodeFlags, "%s (%d)", name->value.c_str(), e.Idx())
				: ImGui::TreeNodeEx((void*)(intptr_t)e.Idx(), nodeFlags, "Entity (%d)", e.Idx());

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
			if (name)
				ImGui::TreeNodeEx((void*)(intptr_t)e.Idx(), nodeFlags, "%s (%d)", name->value.c_str(), e.Idx());
			else
				ImGui::TreeNodeEx((void*)(intptr_t)e.Idx(), nodeFlags, "Entity (%d)", e.Idx());
			if (ImGui::IsItemClicked())
				hierarchy->select = e;
		}
	}
}

void HierarchySystem::OnUpdate(UECS::Schedule& schedule) {
	schedule.RegisterJob([](UECS::World* w, UECS::Latest<UECS::Singleton<Hierarchy>> hierarchy) {
		w->AddCommand([hierarchy](UECS::World* w) {
			if (ImGui::Begin("Hierarchy")) {

				UECS::ArchetypeFilter filter;
				filter.none = { UECS::CmptType::Of<Parent> };
				const_cast<UECS::World*>(hierarchy->world)->RunEntityJob(
					[=](UECS::Entity e) { detail::HierarchyPrintEntity(const_cast<Hierarchy*>(hierarchy.Get()), e); },
					false,
					filter
				);
			}
			ImGui::End();
		});
	}, "HierarchySystem");
}
