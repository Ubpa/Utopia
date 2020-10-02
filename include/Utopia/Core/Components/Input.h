#pragma once

#include <UGM/point.h>
#include <UGM/val.h>

namespace Ubpa::Utopia {
	struct Input {
		// ============================
		//           [Basic]           
		// ============================

		valf2 DisplaySize;

		// Mouse position, in pixels.
		// Set to (-FLT_MAX, -FLT_MAX) if mouse is unavailable (on another screen, etc.)
		pointf2 MousePos;

		// Mouse buttons: 0=left, 1=right, 2=middle + extras
		bool MouseDown[5];

		// Mouse wheel Vertical: 1 unit scrolls about 5 lines text.
		float MouseWheel;

		// Mouse wheel Horizontal.
		// Most users don't have a mouse with an horizontal wheel, may not be filled by all back-ends.
		float MouseWheelH;

		// Keyboard modifier pressed: Control
		bool KeyCtrl;

		// Keyboard modifier pressed: Shift
		bool KeyShift;

		// Keyboard modifier pressed: Alt
		bool KeyAlt;
		
		// Keyboard modifier pressed: Cmd/Super/Windows
		bool KeySuper;

		// Keyboard keys that are pressed (ideally left in the "native" order your engine has access to keyboard keys, so you can use your own defines/enums for keys).
		// ASCII : 0-255
		// 256 - 511 ?
		bool KeysDown[512];

		// ============================
		//            [Pro]            
		// ============================
		// Forward compatibility not guaranteed!

		bool MouseInDisplay;
		bool MouseInDisplayPre;

		// Mouse button went from !Down to Down
		bool MouseClicked[5];

		// Previous mouse position (note that MouseDelta is not necessary == MousePos-MousePosPrev, in case either position is invalid)
		pointf2 MousePosPrev;

		// Mouse delta.
		// Note that this is zero if either current or previous position are invalid (-FLT_MAX,-FLT_MAX), so a disappearing/reappearing mouse won't have a huge delta.
		vecf2 MouseDelta;

		// Position at time of clicking
		pointf2 MouseClickedPos[5];

		// Has mouse button been double-clicked?
		bool MouseDoubleClicked[5];

		// Mouse button went from Down to !Down
		bool MouseReleased[5];

		// Duration the mouse button has been down (0.0f == just clicked)
		float MouseDownDuration[5];

		// Duration the keyboard key has been down (0.0f == just pressed)
		float KeysDownDuration[512];
	};
}

#include "details/Input_AutoRefl.inl"
