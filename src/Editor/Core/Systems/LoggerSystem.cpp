#include <Utopia/Editor/Core/Systems/LoggerSystem.h>

#include <Utopia/Core/StringsSink.h>

#include <imgui/imgui.h>
#include <spdlog/spdlog.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UECS;

void LoggerSystem::OnUpdate(UECS::Schedule& schedule) {
	schedule.GetWorld()->AddCommand([]() {
		if (ImGui::Begin("Log")) {
			auto logger = spdlog::default_logger();
			if (!logger || logger->sinks().size() != 1) {
				ImGui::Text("not support");
				ImGui::End();
				return;
			}
			auto sink = std::dynamic_pointer_cast<StringsSink>(logger->sinks().front());
			if (!sink) {
				ImGui::Text("not support");
				ImGui::End();
				return;
			}

#ifndef NDEBUG
			if (ImGui::Button("Test")) {
				spdlog::info("Welcome to spdlog version {}.{}.{}  !", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);
				spdlog::warn("Easy padding in numbers like {:08d}", 12);
				spdlog::critical("Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
				spdlog::info("Support for floats {:03.2f}", 1.23456);
				spdlog::info("Positional args are {1} {0}..", "too", "supported");
				spdlog::info("{:>8} aligned, {:<8} aligned", "right", "left");
			}
			ImGui::SameLine();
#endif // !NDEBUG

			if (ImGui::Button("Clear"))
				sink->Clear();
			ImGui::SameLine();

			const auto& logs = sink->GetLogs();

			static ImGuiTextFilter filter;
			filter.Draw();

			ImGui::Separator();
			ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			if (filter.IsActive()) {
				for (const auto& log : logs) {
					if (filter.PassFilter(log.c_str()))
						ImGui::TextUnformatted(log.c_str());
				}
			}
			else {
				for (const auto& log : logs)
					ImGui::TextUnformatted(log.c_str());
			}
			ImGui::PopStyleVar();

			if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
				ImGui::SetScrollHereY(1.0f);

			ImGui::EndChild();
		}
		ImGui::End();
	}, 0);
}
