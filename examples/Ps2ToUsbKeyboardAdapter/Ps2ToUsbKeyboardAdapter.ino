/*
Copyright (C) 2017 Steve Benz <s8878992@hotmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
USA
*/
#include "ps2_Keyboard.h"
#include "ps2_SimpleDiagnostics.h"
#include "ps2_UsbTranslator.h"
#include "HID-Project.h"

// Create a log of all the data going to and from the keyboard and the host.
class Diagnostics
    : public ps2::SimpleDiagnostics<254>
{
    typedef ps2::SimpleDiagnostics<254> base;

    enum class UsbTranslatorAppCode : uint8_t {
        // no errors

        sentUsbKeyDown = 0 + base::firstUnusedInfoCode,
        sentUsbKeyUp = 1 + base::firstUnusedInfoCode,
    };

public:
    void sentUsbKeyDown(byte b) { this->push(UsbTranslatorAppCode::sentUsbKeyDown, b); }
    void sentUsbKeyUp(byte b) { this->push(UsbTranslatorAppCode::sentUsbKeyUp, b); }
};

// The USB library supports a begin() statement; this code delays calling it until we're
//  ready to actually do something with the keyboard, but that doesn't seem real valuable.
static bool hasBegun = false;

static Diagnostics diagnostics;
static ps2::UsbTranslator<Diagnostics> keyMapping(diagnostics);
static ps2::Keyboard<4,2,1, Diagnostics> ps2Keyboard(diagnostics);
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
    diagnostics.setLedIndicator<LED_BUILTIN_RX, ps2::DiagnosticsLedBlink::blinkOnError>();

    ps2::KeyboardOutput scanCode = ps2Keyboard.readScanCode();
    if (scanCode == ps2::KeyboardOutput::garbled) {
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
                    diagnostics.sendReport(BootKeyboard);
                    diagnostics.reset();
                }
                else {
                    diagnostics.sentUsbKeyDown(hidCode);
                    BootKeyboard.press(hidCode);
                }
                break;
            case ps2::UsbKeyAction::KeyUp:
                if (hidCode == KeyboardKeycode::KEY_SCROLL_LOCK) {
                    // Don't send the keyup, because we hid the keydown.
                }
                else {
                    diagnostics.sentUsbKeyUp(hidCode);
                    BootKeyboard.release(hidCode);
                }
                break;
        }
    }
}
