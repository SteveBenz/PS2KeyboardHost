#pragma once

#include "ps2_KeyboardLeds.h"
#include "ps2_KeyboardOutput.h"

namespace ps2 {

	enum UsbKeyboardLeds {
		UsbNoLeds = 0x0,
		UsbNumLockLed = 0x1,
		UsbCapsLockLed = 0x2,
		UsbScrollLockLed = 0x4,
		UsbLedMask = 0x07,
	};

	struct UsbKeyAction {
		uint8_t hidCode;
		enum {
			KeyUp,
			KeyDown,
			None
		} gesture;
	};

	// Translates from PS2's default scancode set to USB/HID
	class UsbTranslator
	{
	public:
		UsbTranslator();
		void reset();
		UsbKeyAction translatePs2Keycode(ps2::KeyboardOutput ps2Scan);
		KeyboardLeds translateLeds(UsbKeyboardLeds usbLeds);

	private:
		bool isSpecial;
		bool isUnmake;
		int pauseKeySequenceIndex;
	};
}

#include "ps2_UsbTranslator.hpp"