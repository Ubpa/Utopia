#include "InspectorSystem.h"

#include "../InspectorRegistry.h"

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
			switch (inspector->mode)
			{
			case Inspector::Mode::Asset: {
				if (!inspector->asset.isValid())
					break;
				w->AddCommand([inspector](UECS::World*) mutable {
					auto path = AssetMngr::Instance().GUIDToAssetPath(inspector->asset);
					if (path.empty()) {
						inspector->asset = xg::Guid{};
						inspector->lock = false;
						return;
					}
					auto asset = AssetMngr::Instance().LoadAsset(path);
					const auto& typeinfo = AssetMngr::Instance().GetAssetType(path);

					if (ImGui::Begin("Inspector")) {
						ImGui::Checkbox("lock", &inspector->lock);
						ImGui::Separator();
						if (InspectorRegistry::Instance().IsRegisteredAsset(typeinfo))
							InspectorRegistry::Instance().Inspect(typeinfo, asset);
					}
					ImGui::End();
				});
				break;
			}
			case Inspector::Mode::Entity: {
				if (!inspector->entity.Valid())
					break;
				w->AddCommand([inspector, world = hierarchy->world](UECS::World*) mutable {
					if (!world->entityMngr.Exist(inspector->entity)) {
						inspector->entity = UECS::Entity::Invalid();
						inspector->lock = false;
						return;
					}

					if (ImGui::Begin("Inspector")) {
						if (ImGui::Button("Attach Component"))
							ImGui::OpenPopup("Attach Component Popup");

						if (ImGui::BeginPopup("Attach Component Popup")) {
							ImGui::PushID("Attach Component Popup");
							// Helper class to easy setup a text filter.
							// You may want to implement a more feature-full filtering scheme in your own application.
							static ImGuiTextFilter filter;
							filter.Draw();
							size_t ID = 0;
							size_t N = world->cmptTraits.GetNames().size();
							for (const auto& [type, name] : world->cmptTraits.GetNames()) {
								if (!world->entityMngr.Have(inspector->entity, type) && filter.PassFilter(name.c_str())) {
									ImGui::PushID(ID);
									ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(ID / float(N), 0.6f, 0.6f));
									ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(ID / float(N), 0.7f, 0.7f));
									ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(ID / float(N), 0.8f, 0.8f));
									if (ImGui::Button(name.c_str()))
										world->entityMngr.Attach(inspector->entity, &type, 1);
									ImGui::PopStyleColor(3);
									ImGui::PopID();
								}
								ID++;
							}
							ImGui::PopID();
							ImGui::EndPopup();
						}
						ImGui::SameLine();
						if (ImGui::Button("Detach Component"))
							ImGui::OpenPopup("Detach Component Popup");
						if (ImGui::BeginPopup("Detach Component Popup")) {
							ImGui::PushID("Detach Component Popup");
							// Helper class to easy setup a text filter.
							// You may want to implement a more feature-full filtering scheme in your own application.
							static ImGuiTextFilter filter;
							filter.Draw();
							size_t ID = 0;
							auto cmpts = world->entityMngr.Components(inspector->entity);
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
											world->entityMngr.Detach(inspector->entity, &cmpt.Type(), 1);
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
											world->entityMngr.Detach(inspector->entity, &cmpt.Type(), 1);
										ImGui::PopStyleColor(3);
										ImGui::PopID();
									}
								}
								ID++;
							}
							ImGui::PopID();
							ImGui::EndPopup();
						}
						ImGui::SameLine();
						ImGui::Checkbox("lock", &inspector->lock);
						ImGui::Separator();

						auto cmpts = world->entityMngr.Components(inspector->entity);
						for (size_t i = 0; i < cmpts.size(); i++) {
							auto type = cmpts[i].Type();
							if (InspectorRegistry::Instance().IsRegisteredCmpt(type))
								continue;
							auto name = world->cmptTraits.Nameof(type);
							if (name.empty())
								ImGui::Text(std::to_string(type.HashCode()).c_str());
							else
								ImGui::Text(name.data());
						}

						for (const auto& cmpt : cmpts) {
							if (InspectorRegistry::Instance().IsRegisteredCmpt(cmpt.Type()))
								InspectorRegistry::Instance().Inspect(world, cmpt);
						}
					}
					ImGui::End();
				});
				break;
				break;
			}
			default:
				break;
			}
			
		},
		"InspectorSystem"
	);
}
