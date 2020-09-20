#include <DustEngine/Core/Systems/InputSystem.h>

#include <DustEngine/Core/Components/Input.h>

#include <_deps/imgui/imgui.h>

using namespace Ubpa::DustEngine;
using namespace Ubpa::UECS;

void InputSystem::OnUpdate(Schedule& schedule) {
	if (!ImGui::GetCurrentContext())
		return;

	schedule.RegisterJob([](Singleton<Input> input) {
		const auto& io = ImGui::GetIO();

		// ============================
		//           [Basic]           
		// ============================
		input->MousePos = io.MousePos;
		memcpy(input->MouseDown, io.MouseDown, 5 * sizeof(bool));
		input->MouseWheel = io.MouseWheel;
		input->MouseWheelH = io.MouseWheelH;
		input->KeyCtrl = io.KeyCtrl;
		input->KeyShift = io.KeyShift;
		input->KeyAlt = io.KeyAlt;
		input->KeySuper = io.KeySuper;
		memcpy(input->KeysDown, io.KeysDown, 512 * sizeof(bool));
		memcpy(input->MouseClicked, io.MouseClicked, 5 * sizeof(bool));

		// ============================
		//            [Pro]            
		// ============================
		// Forward compatibility not guaranteed!

		input->MousePosPrev = io.MousePosPrev;
		input->MouseDelta = io.MouseDelta;
		memcpy(input->MouseClickedPos, io.MouseClickedPos, 5 * sizeof(pointf2));
		memcpy(input->MouseDoubleClicked, io.MouseDoubleClicked, 5 * sizeof(bool));
		memcpy(input->MouseReleased, io.MouseReleased, 5 * sizeof(bool));
		memcpy(input->MouseDownDuration, io.MouseDownDuration, 5 * sizeof(float));
		memcpy(input->KeysDownDuration, io.KeysDownDuration, 512 * sizeof(float));
	}, SystemFuncName);
}
