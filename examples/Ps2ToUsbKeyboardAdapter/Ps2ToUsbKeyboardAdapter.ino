/*
 Name:		Ps2KeyboardController.ino
 Created:	4/9/2017 6:40:00 PM
 Author:	sbenz
*/
#include "ps2_Keyboard.h"
#include "ps2_SimpleDiagnostics.h"
#include "ps2_UsbTranslator.h"
#include "HID-Project.h"

// Create a log of all the data going to and from the keyboard and the host.
class Diagnostics_
    : public ps2::SimpleDiagnostics<254>
{
    typedef ps2::SimpleDiagnostics<254> base;

    enum class UsbTranslatorAppCode : uint8_t {
        // no errors

        sentUsbKeyDown = 0 + base::firstUnusedInfoCode,
        sentUsbKeyUp = 1 + base::firstUnusedInfoCode,
    };

public:
    void sentUsbKeyDown(byte b) { this->push((uint8_t)UsbTranslatorAppCode::sentUsbKeyDown, b); }
    void sentUsbKeyUp(byte b) { this->push((uint8_t)UsbTranslatorAppCode::sentUsbKeyUp, b); }
};

// The USB library supports a begin() statement; this code delays calling it until we're
//  ready to actually do something with the keyboard, but that doesn't seem real valuable.
static bool hasBegun = false;

static Diagnostics_ Diagnostics;
static ps2::UsbTranslator<Diagnostics_> keyMapping(Diagnostics);
static ps2::Keyboard<4,2,1, Diagnostics_> ps2Keyboard(Diagnostics);
static ps2::UsbKeyboardLeds ledValueLastSentToPs2 = ps2::UsbKeyboardLeds::none;

// This example demonstrates how to create keyboard translations (in this case, how the caps lock
//  and other modifier keys are laid out).  The example reads switch1Pin to toggle whether this
//  behavior is on or off.
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

    // On the Pro-Micro, which is what this code was developed on, pin 13 is not connected to anything,
    //  so pin 17 is the easiest one.
    Diagnostics.setLedIndicator<LED_BUILTIN_RX, ps2::DiagnosticsLedBlink::onErrorOnly>();

	ps2::KeyboardOutput scanCode = ps2Keyboard.readScanCode();
	if (scanCode == ps2::KeyboardOutput::garbled) {
        // although it'll auto-retry, we want to ensure we don't end up with stuck keys.
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
                if (hidCode == KeyboardKeycode::KEY_SCROLL_LOCK) {
                    // Real use-cases for using the Scroll Lock key are thin on the ground, so this is okay,
                    //  but perhaps it'd be better to send the report if the key is held down or some other
                    //  more specific feedback.  We send the output to the keyboard itself, since the device
                    //  might not be used in a computer that actually has the code to read the serial port.
                    Diagnostics.sendReport(BootKeyboard);
                    Diagnostics.reset();
                }
                else {
                    Diagnostics.sentUsbKeyDown(hidCode);
                    BootKeyboard.press(hidCode);
                }
				break;
			case ps2::UsbKeyAction::KeyUp:
                if (hidCode == KeyboardKeycode::KEY_SCROLL_LOCK) {
                    // Don't send the keyup, because we hid the keydown.
                }
                else {
                    Diagnostics.sentUsbKeyUp(hidCode);
                    BootKeyboard.release(hidCode);
                }
				break;
		}
	}
}
