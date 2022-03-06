#include <Utopia/Editor/Core/Systems/FrameGraphVisualizerSystem.h>

#include <Utopia/Editor/Core/Components/FrameGraphVisualizer.h>

#include <Utopia/Render/DX12/FrameGraphVisualize.h>

namespace ed = ax::NodeEditor;

static constexpr size_t offset_write_pin = 1;
static constexpr size_t offset_read_pin = 2;
static constexpr size_t offset_move_in_pin = 3;
static constexpr size_t offset_move_out_pin = 4;

void Ubpa::Utopia::FrameGraphVisualizerSystem::OnUpdate(UECS::Schedule& schedule) {
	UECS::World* world = schedule.GetWorld();
	world->AddCommand([world]{
		world->RunEntityJob([](UECS::Write<FrameGraphVisualizer> FrameGraphVisualizer) {
            if (!FrameGraphVisualizer->frameGraphDataMap.contains(FrameGraphVisualizer->currFrameGraphName))
                FrameGraphVisualizer->currFrameGraphName = "";

            if (ImGui::Begin("FrameGraphVisualizer Config")) {
                ImGuiComboFlags flag = 0;
                if (ImGui::BeginCombo("FrameGraphName", FrameGraphVisualizer->currFrameGraphName.data(), flag))
                {
                    for (const auto& iter : FrameGraphVisualizer->frameGraphDataMap) {
                        const std::string& name = iter.first;
                        bool isSelected = name == FrameGraphVisualizer->currFrameGraphName;
                        if (ImGui::Selectable(name.data(), isSelected))
                            FrameGraphVisualizer->currFrameGraphName = name;

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }
            ImGui::End();

            { // node editor
                ed::SetCurrentEditor(FrameGraphVisualizer->ctx);

                ed::Begin("FrameGraphVisualizer");

                if (FrameGraphVisualizer->ctx && !FrameGraphVisualizer->currFrameGraphName.empty()) {
                    const auto& fgData = FrameGraphVisualizer->frameGraphDataMap.at(FrameGraphVisualizer->currFrameGraphName);
                    const UFG::FrameGraph& fg = fgData.fg;
                    size_t fgId = string_hash(FrameGraphVisualizer->currFrameGraphName.data());
                    std::map<size_t, size_t> imagedIDs; // nodeID -> idx
                    if (fgData.stage) {
                        for (size_t i = 0; i < fgData.stage->GetInputNodeIDs().size(); i++)
                            imagedIDs.emplace(fgData.stage->GetInputNodeIDs()[i], i);
                    }

                    // resource nodes
                    for (const auto& node : fg.GetResourceNodes()) {
                        if (node.Name().starts_with(FrameGraphVisualize::NamePrefix))
                            continue;

                        size_t nodeId = fgId + string_hash(node.Name().data());
                        ed::BeginNode(nodeId);
                            ImGui::Text(node.Name().data());

                            ed::BeginPin(nodeId + offset_write_pin, ed::PinKind::Input);
                            ImGui::Text("-> Write");
                            ed::EndPin();
                            ImGui::SameLine();
                            ed::BeginPin(nodeId + offset_read_pin, ed::PinKind::Output);
                            ImGui::Text("Read ->");
                            ed::EndPin();

                            ed::BeginPin(nodeId + offset_move_in_pin, ed::PinKind::Input);
                            ImGui::Text("-> Move In");
                            ed::EndPin();
                            ImGui::SameLine();
                            ed::BeginPin(nodeId + offset_move_out_pin, ed::PinKind::Output);
                            ImGui::Text("Move Out ->");
                            ed::EndPin();

                            if (auto target = imagedIDs.find(fg.GetResourceNodeIndex(node.Name())); target != imagedIDs.end()) {
                                assert(fgData.stage);
                                ImGui::Image((ImTextureID)fgData.stage->GetOutputSrvGpuHandles()[target->second].ptr, {256,256});
                            }
                        ed::EndNode();
                    }

                    // pass nodes
                    for (const auto& pass : fg.GetPassNodes()) {
                        if (pass.GetType() == UFG::PassNode::Type::Copy)
                            continue;

                        if (pass.Name().starts_with(FrameGraphVisualize::NamePrefix))
                            continue;

                        size_t passId = fgId + string_hash(pass.Name().data());
                        ed::BeginNode(passId);
                            ImGui::Text(pass.Name().data());

                            size_t N = std::max(pass.Inputs().size(), pass.Outputs().size());
                            for (size_t i = 0; i < N; i++) {
                                if (i < pass.Inputs().size()) {
                                    ed::BeginPin(passId + 1 + i, ed::PinKind::Input);
                                    const std::string name = "-> " + std::string(fg.GetResourceNodes()[pass.Inputs()[i]].Name());
                                    ImGui::Text(name.data());
                                    ed::EndPin();
                                }
                                if (i < pass.Inputs().size() && i < pass.Outputs().size())
                                    ImGui::SameLine();
                                if (i < pass.Outputs().size()) {
                                    ed::BeginPin(passId + 1 + pass.Inputs().size() + i, ed::PinKind::Output);
                                    const std::string name = std::string(fg.GetResourceNodes()[pass.Outputs()[i]].Name()) + " ->";
                                    ImGui::Text(name.data());
                                    ed::EndPin();
                                }
                            }
                        ed::EndNode();
                    }

                    // links
                    for (const auto& pass : fg.GetPassNodes()) {
                        if (pass.GetType() == UFG::PassNode::Type::Copy)
                            continue;

                        size_t passId = fgId + string_hash(pass.Name().data());

                        size_t linkId = passId + 1 + pass.Inputs().size() + pass.Outputs().size();

                        for (size_t i = 0; i < pass.Inputs().size(); i++) {
                            size_t nodeId = fgId + string_hash(fg.GetResourceNodes()[pass.Inputs()[i]].Name().data());
                            size_t nodePinId = nodeId + offset_read_pin;

                            ed::Link(linkId + i, nodePinId, passId + 1 + i, { 0.608f,0.733f,0.349f,1.f });
                        }

                        for (size_t i = 0; i < pass.Outputs().size(); i++) {
                            size_t nodeId = fgId + string_hash(fg.GetResourceNodes()[pass.Outputs()[i]].Name().data());
                            size_t nodePinId = nodeId + offset_write_pin;

                            ed::Link(linkId + pass.Inputs().size() + i, nodePinId, passId + 1 + pass.Inputs().size() + i, { 0.929f,0.110f,0.141f,1.f });
                        }
                    }

                    for (size_t i = 0; i < fg.GetMoveNodes().size(); i++) {
                        size_t moveId = fgId + string_hash("MoveNode") + i;
                        const auto& move = fg.GetMoveNodes()[i];
                        size_t nodeInputId = fgId + string_hash(fg.GetResourceNodes()[move.GetSourceNodeIndex()].Name().data());
                        size_t nodeOutputId = fgId + string_hash(fg.GetResourceNodes()[move.GetDestinationNodeIndex()].Name().data());
                        size_t nodeMoveOutPinId = nodeInputId + offset_move_out_pin;
                        size_t nodeMoveInPinId = nodeOutputId + offset_move_in_pin;

                        ed::Link(moveId, nodeMoveOutPinId, nodeMoveInPinId, { 0.969f,0.588f,0.275f,1.f });
                    }
                }

                ed::End();

                ed::SetCurrentEditor(nullptr);
            }
		}, false);
	}, 0);
}
