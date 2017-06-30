/*
 Name:		Ps2KeyboardController.ino
 Created:	4/9/2017 6:40:00 PM
 Author:	sbenz
*/
#include "ps2_Keyboard.h"
#include "Diagnostics.h"
#include "ps2_UsbTranslator.h"
#include "HID-Project.h"

static bool hasBegun = false;
static ps2::UsbTranslator keyMapping;
static ps2::Keyboard<4,2,1,Diagnostics_> ps2Keyboard(Diagnostics);
static const int switch1Pin = 6;
static const int switch2Pin = 8;

// the setup function runs once when you press reset or power the board
void setup() {
	pinMode(LED_BUILTIN, OUTPUT);
	ps2Keyboard.begin();
	pinMode(switch1Pin, INPUT_PULLUP);
	pinMode(switch2Pin, INPUT_PULLUP);

	// We're not actually going to pay much mind to this, but it could produce
	// useful diagnostics if things don't go well.
	ps2Keyboard.echo();
}

static ps2::UsbKeyboardLeds ledValueLastSentToPs2 = ps2::UsbNoLeds;


static KeyboardKeycode translateUsbKeystroke(KeyboardKeycode usbKeystroke)
{
	switch (usbKeystroke)
	{
	case KEY_LEFT_ALT:
		return KEY_LEFT_GUI;

	case KEY_LEFT_CTRL:
		return KEY_LEFT_ALT;

	case HID_KEYBOARD_CAPS_LOCK:
		return KEY_LEFT_CTRL;

	case KEY_PAUSE: // pause
		return (KeyboardKeycode)0xe8;

	default:
		return usbKeystroke;
	}
}


void loop() {
	ps2::UsbKeyboardLeds newLedState = (ps2::UsbKeyboardLeds)BootKeyboard.getLeds();
	if (newLedState != ledValueLastSentToPs2)
	{
		ps2Keyboard.sendLedStatus(keyMapping.translateLeds(newLedState));
		ledValueLastSentToPs2 = newLedState;
	}

	bool isRemapMode = digitalRead(switch1Pin) != 0;

	ps2::KeyboardOutput scanCode = ps2Keyboard.readScanCode();
	if (scanCode == ps2::KeyboardOutput::garbled) {
		keyMapping.reset();
	}
	else if (scanCode != ps2::KeyboardOutput::none)
	{
		if (!hasBegun)
		{
			BootKeyboard.begin();
			hasBegun = true;
		}

		ps2::UsbKeyAction action = keyMapping.translatePs2Keycode(scanCode);
		KeyboardKeycode hidCode = (KeyboardKeycode)action.hidCode;

		if (isRemapMode) {
			hidCode = translateUsbKeystroke(hidCode);
		}
		switch (action.gesture) {
			case ps2::UsbKeyAction::KeyDown:
				BootKeyboard.press(hidCode);
				break;
			case ps2::UsbKeyAction::KeyUp:
				BootKeyboard.release(hidCode);
				break;
		}
	}
}
