#include <Utopia/App/Editor/Systems/InspectorSystem.h>

#include <Utopia/App/Editor/InspectorRegistry.h>

#include <Utopia/App/Editor/Components/Inspector.h>
#include <Utopia/App/Editor/Components/Hierarchy.h>
#include <Utopia/Core/AssetMngr.h>

#include <_deps/imgui/imgui.h>

using namespace Ubpa::Utopia;

void InspectorSystem::OnUpdate(UECS::Schedule& schedule) {
	schedule.GetWorld()->AddCommand([world = schedule.GetWorld()]() {
		auto inspector = world->entityMngr.WriteSingleton<Inspector>();
		if (!inspector)
			return;

		auto hierarchy = world->entityMngr.WriteSingleton<Hierarchy>();
		switch (inspector->mode) {
		case Inspector::Mode::Asset:
		{
			auto path = AssetMngr::Instance().GUIDToAssetPath(inspector->asset);
			if (path.empty()) {
				inspector->asset = xg::Guid{};
				inspector->lock = false;
				return;
			}
			auto asset = AssetMngr::Instance().LoadMainAsset(path);
			const auto& type = AssetMngr::Instance().GetAssetType(path);

			if (ImGui::Begin("Inspector") && inspector->asset.isValid()) {
				ImGui::Checkbox("lock", &inspector->lock);
				ImGui::Separator();
				if (InspectorRegistry::Instance().IsRegisteredAsset(type))
					InspectorRegistry::Instance().Inspect(type, asset.obj.GetPtr());
			}
			ImGui::End();
			break;
		}
		case Inspector::Mode::Entity:
		{
			if (!hierarchy->world->entityMngr.Exist(inspector->entity)) {
				inspector->entity = UECS::Entity::Invalid();
				inspector->lock = false;
			}

			if (ImGui::Begin("Inspector") && inspector->entity.Valid()) {
				if (ImGui::Button("Attach Component"))
					ImGui::OpenPopup("Attach Component Popup");

				if (ImGui::BeginPopup("Attach Component Popup")) {
					ImGui::PushID("Attach Component Popup");
					// Helper class to easy setup a text filter.
					// You may want to implement a more feature-full filtering scheme in your own application.
					static ImGuiTextFilter filter;
					filter.Draw();
					int ID = 0;
					size_t N = hierarchy->world->entityMngr.cmptTraits.GetNames().size();
					for (const auto& [type, name] : hierarchy->world->entityMngr.cmptTraits.GetNames()) {
						if (!hierarchy->world->entityMngr.Have(inspector->entity, type) && filter.PassFilter(name.data())) {
							ImGui::PushID(ID);
							ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(ID / float(N), 0.6f, 0.6f));
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(ID / float(N), 0.7f, 0.7f));
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(ID / float(N), 0.8f, 0.8f));
							if (ImGui::Button(name.data()))
								hierarchy->world->entityMngr.Attach(inspector->entity, TempTypeIDs{ type });
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
					int ID = 0;
					auto cmpts = hierarchy->world->entityMngr.Components(inspector->entity, UECS::AccessMode::WRITE);
					size_t N = cmpts.size();
					for (const auto& cmpt : cmpts) {
						auto name = hierarchy->world->entityMngr.cmptTraits.Nameof(cmpt.AccessType());
						if (!name.empty()) {
							if (filter.PassFilter(name.data())) {
								ImGui::PushID(ID);
								ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(ID / float(N), 0.6f, 0.6f));
								ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(ID / float(N), 0.7f, 0.7f));
								ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(ID / float(N), 0.8f, 0.8f));
								if (ImGui::Button(name.data())) {
									const auto cmptType = cmpt.AccessType();
									hierarchy->world->entityMngr.Detach(inspector->entity, TempTypeIDs{ cmptType });
								}
								ImGui::PopStyleColor(3);
								ImGui::PopID();
							}
						}
						else {
							auto name = std::to_string(cmpt.AccessType().GetValue());
							if (filter.PassFilter(name.c_str())) {
								ImGui::PushID(ID);
								ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(ID / float(N), 0.6f, 0.6f));
								ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(ID / float(N), 0.7f, 0.7f));
								ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(ID / float(N), 0.8f, 0.8f));
								if (ImGui::Button(name.c_str())) {
									const auto cmptType = cmpt.AccessType();
									hierarchy->world->entityMngr.Detach(inspector->entity, TempTypeIDs{ cmptType });
								}
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

				auto cmpts = hierarchy->world->entityMngr.Components(inspector->entity, Ubpa::UECS::AccessMode::WRITE);
				for (size_t i = 0; i < cmpts.size(); i++) {
					auto type = cmpts[i].AccessType();
					if (InspectorRegistry::Instance().IsRegisteredCmpt(type))
						continue;
					auto name = hierarchy->world->entityMngr.cmptTraits.Nameof(type);
					if (name.empty())
						ImGui::Text(std::to_string(type.GetValue()).c_str());
					else
						ImGui::Text(name.data());
				}

				for (const auto& cmpt : cmpts) {
					if (InspectorRegistry::Instance().IsRegisteredCmpt(cmpt.AccessType()))
						InspectorRegistry::Instance().Inspect(hierarchy->world, cmpt.Ptr());
				}
			}
			ImGui::End();
			break;
		}
		default:
			assert(false);
			break;
		}
	}, 0);
}
