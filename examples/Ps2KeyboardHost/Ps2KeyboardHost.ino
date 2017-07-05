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
#include "ps2_AnsiTranslator.h"
#include "ps2_SimpleDiagnostics.h"

typedef ps2::SimpleDiagnostics<254> Diagnostics_;
static Diagnostics_ diagnostics;
static ps2::AnsiTranslator<Diagnostics_> keyMapping(diagnostics);
static ps2::Keyboard<4,2,1, Diagnostics_> ps2Keyboard(diagnostics);
static ps2::KeyboardLeds lastLedSent;

void setup() {
    ps2Keyboard.begin();
    ps2Keyboard.reset();

    keyMapping.setNumLock(true);
    lastLedSent = ps2::KeyboardLeds::numLock;
    ps2Keyboard.sendLedStatus(ps2::KeyboardLeds::numLock);
}

void loop() {
    diagnostics.setLedIndicator<LED_BUILTIN_RX>();
    ps2::KeyboardOutput scanCode = ps2Keyboard.readScanCode();
    if (scanCode == ps2::KeyboardOutput::garbled) {
        keyMapping.reset();
    }
    else if (scanCode != ps2::KeyboardOutput::none)
    {
        char buf[2];
        buf[1] = '\0';
        buf[0] = keyMapping.translatePs2Keycode(scanCode);
        if (buf[0] == '\r') {
            Serial.println();
        }
        else if (buf[0] == '\004') { // ctrl+D
            Serial.println();
            diagnostics.sendReport(Serial);
            Serial.println();
            diagnostics.reset();
        }
        else if (buf[0] >= ' ') { // Characters < ' ' are control-characters; this example isn't clever enough to do anything with them.
            Serial.write(buf);
        }

        ps2::KeyboardLeds newLeds =
              (keyMapping.getCapsLock() ? ps2::KeyboardLeds::capsLock : ps2::KeyboardLeds::none)
            | (keyMapping.getNumLock() ? ps2::KeyboardLeds::numLock : ps2::KeyboardLeds::none);
        if (newLeds != lastLedSent) {
            ps2Keyboard.sendLedStatus(newLeds);
            lastLedSent = newLeds;
        }
    }
}
