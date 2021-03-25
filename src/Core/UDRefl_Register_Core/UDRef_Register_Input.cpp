#include "UDRefl_Register_Core_impl.h"

#include <Utopia/Core/Components/Input.h>

#include <UDRefl/UDRefl.hpp>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

void Ubpa::Utopia::details::UDRefl_Register_Input() {
	Mngr.RegisterType<Input>();
	Mngr.AddField<&Input::DisplaySize>("DisplaySize");
	Mngr.AddField<&Input::MousePos>("MousePos");
	Mngr.AddField<&Input::MouseDown>("MouseDown");
	Mngr.AddField<&Input::MouseWheel>("MouseWheel");
	Mngr.AddField<&Input::MouseWheelH>("MouseWheelH");
	Mngr.AddField<&Input::KeyCtrl>("KeyCtrl");
	Mngr.AddField<&Input::KeyShift>("KeyShift");
	Mngr.AddField<&Input::KeyAlt>("KeyAlt");
	Mngr.AddField<&Input::KeySuper>("KeySuper");
	Mngr.AddField<&Input::KeysDown>("KeysDown");
	Mngr.AddField<&Input::MouseInDisplay>("MouseInDisplay");
	Mngr.AddField<&Input::MouseInDisplayPre>("MouseInDisplayPre");
	Mngr.AddField<&Input::MouseClicked>("MouseClicked");
	Mngr.AddField<&Input::MousePosPrev>("MousePosPrev");
	Mngr.AddField<&Input::MouseDelta>("MouseDelta");
	Mngr.AddField<&Input::MouseClickedPos>("MouseClickedPos");
	Mngr.AddField<&Input::MouseDoubleClicked>("MouseDoubleClicked");
	Mngr.AddField<&Input::MouseReleased>("MouseReleased");
	Mngr.AddField<&Input::MouseDownDuration>("MouseDownDuration");
	Mngr.AddField<&Input::KeysDownDuration>("KeysDownDuration");
}
