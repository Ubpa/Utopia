#include "InspectorSystem.h"

#include "../CmptInsepctor.h"

#include "../Components/Inspector.h"
#include "../Components/Hierarchy.h"

#include <DustEngine/Transform/Components/Components.h>

#include <DustEngine/_deps/imgui/imgui.h>

using namespace Ubpa::DustEngine;

void InspectorSystem::OnUpdate(UECS::Schedule& schedule) {
	schedule.RegisterJob(
		[]
		(
			UECS::World* w,
			UECS::Latest<UECS::Singleton<Hierarchy>> hierarchy,
			UECS::Singleton<Inspector> inspector
		)
		{
			if (!inspector->lock)
				inspector->target = hierarchy->select;

			w->AddCommand([inspector, world = hierarchy->world](UECS::World*) {
				if (ImGui::Begin("Inspector") && inspector->target.Valid()) {
					auto cmpts = world->entityMngr.Components(inspector->target);
					for (size_t i = 0; i < cmpts.size(); i++) {
						auto type = cmpts[i].Type();
						if (CmptInspector::Instance().IsCmptRegistered(type))
							continue;
						auto name = world->cmptTraits.Nameof(type);
						if(name.empty())
							ImGui::Text(std::to_string(type.HashCode()).c_str());
						else
							ImGui::Text(name.data());
					}

					for (const auto& cmpt : cmpts) {
						if (CmptInspector::Instance().IsCmptRegistered(cmpt.Type()))
							CmptInspector::Instance().Inspect(cmpt);
					}
				}
				ImGui::End();
			});
		},
		"InspectorSystem"
	);
}
