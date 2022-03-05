#pragma once

#include <imgui_node_editor/imgui_node_editor.h>
#include <UFG/FrameGraph.hpp>
#include <map>
#include <string>

namespace Ubpa::Utopia {
	struct FrameGraphVistualizer {
		~FrameGraphVistualizer() {
			if (ctx)
				ax::NodeEditor::DestroyEditor(ctx);
		}

		std::map<std::string, UFG::FrameGraph> frameGraphMap;

		// internal data
		ax::NodeEditor::EditorContext* ctx{ nullptr };
		std::string currFrameGraphName;
	};
}
