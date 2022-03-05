#include <Utopia/Editor/Core/Systems/FrameGraphVistualizerSystem.h>

#include <Utopia/Editor/Core/Components/FrameGraphVistualizer.h>

namespace ed = ax::NodeEditor;

void Ubpa::Utopia::FrameGraphVistualizerSystem::OnUpdate(UECS::Schedule& schedule) {
	UECS::World* world = schedule.GetWorld();
	world->AddCommand([world]{
		world->RunEntityJob([](UECS::Latest<FrameGraphVistualizer> frameGraphVistualizer) {
			if (!frameGraphVistualizer->ctx)
				return;

            { // node editor
                ed::SetCurrentEditor(frameGraphVistualizer->ctx);

                ed::Begin("My Editor");

                int uniqueId = 1;

                // Start drawing nodes.
                ed::BeginNode(uniqueId++);
                ImGui::Text("Node A");
                ed::BeginPin(uniqueId++, ed::PinKind::Input);
                ImGui::Text("-> In");
                ed::EndPin();
                ImGui::SameLine();
                ed::BeginPin(uniqueId++, ed::PinKind::Output);
                ImGui::Text("Out ->");
                ed::EndPin();
                ed::EndNode();

                ed::End();

                ed::SetCurrentEditor(nullptr);
            }
		}, false);
	}, 0);
}
