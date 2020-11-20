#include <Utopia/App/Editor/Systems/SystemControllerSystem.h>

#include <Utopia/App/Editor/Components/SystemController.h>

#include <_deps/imgui/imgui.h>

using namespace Ubpa::UECS;
using namespace Ubpa::Utopia;

void SystemControllerSystem::OnUpdate(Schedule& schedule) {
	schedule.RegisterCommand([](World* w) {
		auto systemController = w->entityMngr.GetSingleton<SystemController>();
		if (!systemController || !systemController->world)
			return;

		if (ImGui::Begin("System Controller")) {
			if (ImGui::Button("Create System"))
				ImGui::OpenPopup("Create System Popup");

			if (ImGui::BeginPopup("Create System Popup")) {
				ImGui::PushID("Create System Popup");
				// Helper class to easy setup a text filter.
				// You may want to implement a more feature-full filtering scheme in your own application.
				static ImGuiTextFilter filter;
				filter.Draw();
				int idx = 0;
				size_t createID = static_cast<size_t>(-1);
				const size_t N = systemController->world->systemMngr.systemTraits.GetNameIDMap().size();
				for (const auto& [name, ID] : systemController->world->systemMngr.systemTraits.GetNameIDMap()) {
					if (!systemController->world->systemMngr.IsAlive(ID) && filter.PassFilter(name.data())) {
						ImGui::PushID(idx);
						ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(idx / float(N), 0.6f, 0.6f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(idx / float(N), 0.7f, 0.7f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(idx / float(N), 0.8f, 0.8f));
						if (ImGui::Button(name.data()))
							createID = ID;
						ImGui::PopStyleColor(3);
						ImGui::PopID();
					}
					idx++;
				}
				if (createID != static_cast<size_t>(-1))
					systemController->world->systemMngr.Create(createID);
				ImGui::PopID();
				ImGui::EndPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Destroy System"))
				ImGui::OpenPopup("Destroy System Popup");
			if (ImGui::BeginPopup("Destroy System Popup")) {
				ImGui::PushID("Destroy System Popup");
				// Helper class to easy setup a text filter.
				// You may want to implement a more feature-full filtering scheme in your own application.
				static ImGuiTextFilter filter;
				filter.Draw();
				int idx = 0;
				size_t destroyID = static_cast<size_t>(-1);
				size_t N = systemController->world->systemMngr.GetAliveSystemIDs().size();
				for (const auto& ID : systemController->world->systemMngr.GetAliveSystemIDs()) {
					auto name = systemController->world->systemMngr.systemTraits.Nameof(ID);
					if (filter.PassFilter(name.data())) {
						ImGui::PushID(idx);
						ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(idx / float(N), 0.6f, 0.6f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(idx / float(N), 0.7f, 0.7f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(idx / float(N), 0.8f, 0.8f));
						if (ImGui::Button(name.data()))
							destroyID = ID;
						ImGui::PopStyleColor(3);
						ImGui::PopID();
					}
					idx++;
				}
				if (destroyID != static_cast<size_t>(-1))
					systemController->world->systemMngr.Destroy(destroyID);
				ImGui::PopID();
				ImGui::EndPopup();
			}
			ImGui::Separator();

			{
				size_t switchID = static_cast<size_t>(-1);
				static ImGuiTextFilter filter;
				filter.Draw();
				for (const auto& ID : systemController->world->systemMngr.GetAliveSystemIDs()) {
					auto name = systemController->world->systemMngr.systemTraits.Nameof(ID);
					if (filter.PassFilter(name.data())) {
						bool isActive = systemController->world->systemMngr.IsActive(ID);
						if (ImGui::Checkbox(name.data(), &isActive))
							switchID = ID;
					}
				}
				if (switchID != static_cast<size_t>(-1)) {
					if (systemController->world->systemMngr.IsActive(switchID))
						systemController->world->systemMngr.Deactivate(switchID);
					else
						systemController->world->systemMngr.Activate(switchID);
				}
			}
		}
		ImGui::End();
	});
}
