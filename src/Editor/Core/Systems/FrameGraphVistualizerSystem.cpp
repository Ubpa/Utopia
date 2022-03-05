#include <Utopia/Editor/Core/Systems/FrameGraphVistualizerSystem.h>

#include <Utopia/Editor/Core/Components/FrameGraphVistualizer.h>

namespace ed = ax::NodeEditor;

static constexpr size_t offset_write_pin = 1;
static constexpr size_t offset_read_pin = 2;
static constexpr size_t offset_move_in_pin = 3;
static constexpr size_t offset_move_out_pin = 4;
static constexpr size_t offset_copy_in_pin = 5;
static constexpr size_t offset_copy_out_pin = 6;

void Ubpa::Utopia::FrameGraphVistualizerSystem::OnUpdate(UECS::Schedule& schedule) {
	UECS::World* world = schedule.GetWorld();
	world->AddCommand([world]{
		world->RunEntityJob([](UECS::Write<FrameGraphVistualizer> frameGraphVistualizer) {
            if (!frameGraphVistualizer->frameGraphMap.contains(frameGraphVistualizer->currFrameGraphName))
                frameGraphVistualizer->currFrameGraphName = "";

            if (ImGui::Begin("FrameGraphVistualizer Config")) {
                ImGuiComboFlags flag = 0;
                if (ImGui::BeginCombo("FrameGraphName", frameGraphVistualizer->currFrameGraphName.data(), flag))
                {
                    for (const auto& [name, fg] : frameGraphVistualizer->frameGraphMap) {
                        bool isSelected = name == frameGraphVistualizer->currFrameGraphName;
                        if (ImGui::Selectable(name.data(), isSelected))
                            frameGraphVistualizer->currFrameGraphName = name;

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }
            ImGui::End();

            { // node editor
                ed::SetCurrentEditor(frameGraphVistualizer->ctx);

                ed::Begin("FrameGraphVistualizer");


                if (frameGraphVistualizer->ctx && !frameGraphVistualizer->currFrameGraphName.empty()) {
                    const UFG::FrameGraph& fg = frameGraphVistualizer->frameGraphMap.at(frameGraphVistualizer->currFrameGraphName);
                    size_t fgId = string_hash(frameGraphVistualizer->currFrameGraphName.data());

                    // resource nodes
                    for (const auto& node : fg.GetResourceNodes()) {
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

                            ed::BeginPin(nodeId + offset_copy_in_pin, ed::PinKind::Input);
                            ImGui::Text("-> Copy In");
                            ed::EndPin();
                            ImGui::SameLine();
                            ed::BeginPin(nodeId + offset_copy_out_pin, ed::PinKind::Output);
                            ImGui::Text("Copy Out ->");
                            ed::EndPin();
                        ed::EndNode();
                    }

                    // pass nodes
                    for (const auto& pass : fg.GetPassNodes()) {
                        if (pass.GetType() == UFG::PassNode::Type::Copy)
                            continue;

                        size_t passId = fgId + string_hash(pass.Name().data());
                        ed::BeginNode(passId);
                            ImGui::Text(pass.Name().data());

                            size_t N = std::max(pass.Inputs().size(), pass.Outputs().size());
                            for (size_t i = 0; i < N; i++) {
                                if (i < pass.Inputs().size()) {
                                    ed::BeginPin(passId + 1 + i, ed::PinKind::Input);
                                    ImGui::Text(("-> " + std::string(fg.GetResourceNodes()[pass.Inputs()[i]].Name())).data());
                                    ed::EndPin();
                                }
                                if (i < pass.Inputs().size() && i < pass.Outputs().size())
                                    ImGui::SameLine();
                                if (i < pass.Outputs().size()) {
                                    ed::BeginPin(passId + 1 + pass.Inputs().size() + i, ed::PinKind::Output);
                                    ImGui::Text((std::string(fg.GetResourceNodes()[pass.Outputs()[i]].Name()) + " ->").data());
                                    ed::EndPin();
                                }
                            }
                        ed::EndNode();
                    }

                    // links
                    for (const auto& pass : fg.GetPassNodes()) {
                        size_t passId = fgId + string_hash(pass.Name().data());
                        if (pass.GetType() == UFG::PassNode::Type::Copy) {
                            for (size_t i = 0; i < pass.Inputs().size(); i++) {
                                size_t nodeInputId = fgId + string_hash(fg.GetResourceNodes()[pass.Inputs()[i]].Name().data());
                                size_t nodeOutputId = fgId + string_hash(fg.GetResourceNodes()[pass.Outputs()[i]].Name().data());
                                size_t nodeCopyOutPinId = nodeInputId + offset_copy_out_pin;
                                size_t nodeCopyInPinId = nodeOutputId + offset_copy_in_pin;

                                ed::Link(passId, nodeCopyOutPinId, nodeCopyInPinId);
                            }
                        }
                        else {
                            size_t linkId = passId + 1 + pass.Inputs().size() + pass.Outputs().size();

                            for (size_t i = 0; i < pass.Inputs().size(); i++) {
                                size_t nodeId = fgId + string_hash(fg.GetResourceNodes()[pass.Inputs()[i]].Name().data());
                                size_t nodePinId = pass.GetType() == UFG::PassNode::Type::General ? nodeId + offset_read_pin
                                    : nodeId + offset_copy_out_pin;

                                ed::Link(linkId + i, nodePinId, passId + 1 + i);
                            }

                            for (size_t i = 0; i < pass.Outputs().size(); i++) {
                                size_t nodeId = fgId + string_hash(fg.GetResourceNodes()[pass.Outputs()[i]].Name().data());
                                size_t nodePinId = pass.GetType() == UFG::PassNode::Type::General ? nodeId + offset_write_pin
                                    : nodeId + offset_copy_in_pin;

                                ed::Link(linkId + pass.Inputs().size() + i, nodePinId, passId + 1 + pass.Inputs().size() + i);
                            }
                        }
                    }

                    for (size_t i = 0; i < fg.GetMoveNodes().size(); i++) {
                        size_t moveId = fgId + string_hash("MoveNode") + i;
                        const auto& move = fg.GetMoveNodes()[i];
                        size_t nodeInputId = fgId + string_hash(fg.GetResourceNodes()[move.GetSourceNodeIndex()].Name().data());
                        size_t nodeOutputId = fgId + string_hash(fg.GetResourceNodes()[move.GetDestinationNodeIndex()].Name().data());
                        size_t nodeMoveOutPinId = nodeInputId + offset_move_out_pin;
                        size_t nodeMoveInPinId = nodeOutputId + offset_move_in_pin;

                        ed::Link(moveId, nodeMoveOutPinId, nodeMoveInPinId);
                    }
                }

                ed::End();

                ed::SetCurrentEditor(nullptr);
            }
		}, false);
	}, 0);
}
