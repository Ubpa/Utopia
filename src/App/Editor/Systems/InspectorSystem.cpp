#include "InspectorSystem.h"

#include "../CmptInsepctor.h"

#include "../Components/Inspector.h"
#include "../Components/Hierarchy.h"

#include <DustEngine/Transform/Components/Components.h>

#include <_deps/imgui/imgui.h>

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

			if (inspector->target.Valid()) {
				w->AddCommand([inspector, world = hierarchy->world](UECS::World*) mutable {
					if (!world->entityMngr.Exist(inspector->target)) {
						inspector->target = UECS::Entity::Invalid();
						inspector->lock = false;
						return;
					}

					if (ImGui::Begin("Inspector")) {
						if (ImGui::CollapsingHeader("[*] Attach Component")) {
							ImGui::PushID("[*] Attach Component");
							// Helper class to easy setup a text filter.
							// You may want to implement a more feature-full filtering scheme in your own application.
							static ImGuiTextFilter filter;
							filter.Draw();
							size_t ID = 0;
							size_t N = world->cmptTraits.GetNames().size();
							for (const auto& [type, name] : world->cmptTraits.GetNames()) {
								if (!world->entityMngr.Have(inspector->target, type) && filter.PassFilter(name.c_str())) {
									ImGui::PushID(ID);
									ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(ID / float(N), 0.6f, 0.6f));
									ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(ID / float(N), 0.7f, 0.7f));
									ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(ID / float(N), 0.8f, 0.8f));
									if (ImGui::Button(name.c_str()))
										world->entityMngr.Attach(inspector->target, &type, 1);
									ImGui::PopStyleColor(3);
									ImGui::PopID();
								}
								ID++;
							}
							ImGui::PopID();
						}
						if (ImGui::CollapsingHeader("[*] Detach Component")) {
							ImGui::PushID("[*] Detach Component");
							// Helper class to easy setup a text filter.
							// You may want to implement a more feature-full filtering scheme in your own application.
							static ImGuiTextFilter filter;
							filter.Draw();
							size_t ID = 0;
							auto cmpts = world->entityMngr.Components(inspector->target);
							size_t N = cmpts.size();
							for (const auto& cmpt : cmpts) {
								auto name = world->cmptTraits.Nameof(cmpt.Type());
								if (!name.empty()) {
									if (filter.PassFilter(name.data())) {
										ImGui::PushID(ID);
										ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(ID / float(N), 0.6f, 0.6f));
										ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(ID / float(N), 0.7f, 0.7f));
										ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(ID / float(N), 0.8f, 0.8f));
										if (ImGui::Button(name.data()))
											world->entityMngr.Detach(inspector->target, &cmpt.Type(), 1);
										ImGui::PopStyleColor(3);
										ImGui::PopID();
									}
								}
								else {
									auto name = std::to_string(cmpt.Type().HashCode());
									if (filter.PassFilter(name.c_str())) {
										ImGui::PushID(ID);
										ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(ID / float(N), 0.6f, 0.6f));
										ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(ID / float(N), 0.7f, 0.7f));
										ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(ID / float(N), 0.8f, 0.8f));
										if (ImGui::Button(name.c_str()))
											world->entityMngr.Detach(inspector->target, &cmpt.Type(), 1);
										ImGui::PopStyleColor(3);
										ImGui::PopID();
									}
								}
								ID++;
							}
							ImGui::PopID();
						}

						auto cmpts = world->entityMngr.Components(inspector->target);
						for (size_t i = 0; i < cmpts.size(); i++) {
							auto type = cmpts[i].Type();
							if (CmptInspector::Instance().IsCmptRegistered(type))
								continue;
							auto name = world->cmptTraits.Nameof(type);
							if (name.empty())
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
			}
		},
		"InspectorSystem"
	);
}
