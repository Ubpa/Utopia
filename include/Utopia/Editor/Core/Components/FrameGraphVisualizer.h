#pragma once

#include <imgui_node_editor/imgui_node_editor.h>
#include <Utopia/Render/DX12/IPipeline.h>
#include <map>
#include <string>

namespace Ubpa::Utopia {
	struct FrameGraphVisualizer {
		~FrameGraphVisualizer() {
			if (ctx)
				ax::NodeEditor::DestroyEditor(ctx);
		}

		std::map<std::string, IPipeline::FrameGraphData> frameGraphDataMap;

		// internal data
		ax::NodeEditor::EditorContext* ctx{ nullptr };
		std::string currFrameGraphName;
	};
}
