#include "UDRefl_Register_Core_impl.h"

#include <Utopia/Core/Components/Input.h>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_Input() {
	Mngr.RegisterType<Input>();
	Mngr.SimpleAddField<&Input::DisplaySize>("DisplaySize");
	Mngr.SimpleAddField<&Input::MousePos>("MousePos");
	Mngr.AddField<&Input::MouseDown>("MouseDown");
	Mngr.SimpleAddField<&Input::MouseWheel>("MouseWheel");
	Mngr.SimpleAddField<&Input::MouseWheelH>("MouseWheelH");
	Mngr.SimpleAddField<&Input::KeyCtrl>("KeyCtrl");
	Mngr.SimpleAddField<&Input::KeyShift>("KeyShift");
	Mngr.SimpleAddField<&Input::KeyAlt>("KeyAlt");
	Mngr.SimpleAddField<&Input::KeySuper>("KeySuper");
	Mngr.AddField<&Input::KeysDown>("KeysDown");
	Mngr.SimpleAddField<&Input::MouseInDisplay>("MouseInDisplay");
	Mngr.SimpleAddField<&Input::MouseInDisplayPre>("MouseInDisplayPre");
	Mngr.AddField<&Input::MouseClicked>("MouseClicked");
	Mngr.AddField<&Input::MousePosPrev>("MousePosPrev");
	Mngr.SimpleAddField<&Input::MouseDelta>("MouseDelta");
	Mngr.SimpleAddField<&Input::MouseClickedPos>("MouseClickedPos");
	Mngr.AddField<&Input::MouseDoubleClicked>("MouseDoubleClicked");
	Mngr.AddField<&Input::MouseReleased>("MouseReleased");
	Mngr.AddField<&Input::MouseDownDuration>("MouseDownDuration");
	Mngr.AddField<&Input::KeysDownDuration>("KeysDownDuration");
}
