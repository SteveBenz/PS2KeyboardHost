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
#include "ps2_NeutralTranslator.h"
#include "ps2_SimpleDiagnostics.h"

typedef ps2::SimpleDiagnostics<32> Diagnostics;
static Diagnostics diagnostics;
static ps2::Keyboard<3,2,1,Diagnostics> ps2Keyboard(diagnostics);
static const int switch1Pin = 6;
static const int switch2Pin = 8;

// the setup function runs once when you press reset or power the board
void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    ps2Keyboard.begin();
    pinMode(switch1Pin, INPUT_PULLUP);
    pinMode(switch2Pin, INPUT_PULLUP);
}

int oldSwitch1PinValue = HIGH;
int oldSwitch2PinValue = HIGH;
static ps2::NeutralTranslator translator;

void waitForUnmake(ps2::KeyboardOutput key)
{
    long stopAtMs = millis() + 1000;

    bool gotUnmake = false;
    bool stop = false;
    do {
        ps2::KeyboardOutput scanCode = ps2Keyboard.readScanCode();
        if (scanCode != ps2::KeyboardOutput::none) {
            Serial.println((byte)scanCode, HEX);
        }

        if (scanCode == ps2::KeyboardOutput::unmake) {
            gotUnmake = true;
        }
        else if (key == scanCode && gotUnmake) {
            stop = true;
        }
    } while (!stop && stopAtMs > millis());
}

void printResult(const char *msg, bool result)
{
    Serial.print(msg);
    Serial.println(result ? "" : "!");
}

class TestQueueDiagnostics
{
public:
    void bufferOverflow()
    {
        if (!overflowExpected) {
            Serial.println("testQueue: Unexpected buffer overflow");
        }
        overflowExpected = false;
    }

    bool overflowExpected = false;
};

static void testQueue()
{
    TestQueueDiagnostics diagnosticsStub;
    ps2::KeyboardOutputBuffer<3, TestQueueDiagnostics> buf(diagnosticsStub);

    ps2::KeyboardOutput k = buf.pop();
    if (k != ps2::KeyboardOutput::none) {
        Serial.println("testQueue: buffer failed pop on empty");
    }

    buf.push(ps2::KeyboardOutput::sc2_0);
    if (buf.pop() != ps2::KeyboardOutput::sc2_0) {
        Serial.println("testQueue: failed single push");
    }

    buf.push(ps2::KeyboardOutput::sc2_0);
    buf.push(ps2::KeyboardOutput::sc2_1);
    buf.push(ps2::KeyboardOutput::sc2_2);
    if (buf.pop() != ps2::KeyboardOutput::sc2_0) {
        Serial.println("testQueue: full buffer assert 1");
    }
    if (buf.pop() != ps2::KeyboardOutput::sc2_1) {
        Serial.println("testQueue: full buffer assert 2");
    }
    if (buf.pop() != ps2::KeyboardOutput::sc2_2) {
        Serial.println("testQueue: full buffer assert 3");
    }
    if (buf.pop() != ps2::KeyboardOutput::none) {
        Serial.println("testQueue: buffer failed pop on empty 2");
    }

    buf.push(ps2::KeyboardOutput::sc2_3);
    buf.push(ps2::KeyboardOutput::sc2_4);
    buf.push(ps2::KeyboardOutput::sc2_5);
    diagnosticsStub.overflowExpected = true;
    buf.push(ps2::KeyboardOutput::sc2_6);
    if (buf.pop() != ps2::KeyboardOutput::sc2_4) {
        Serial.println("testQueue: full buffer assert 4");
    }
    if (buf.pop() != ps2::KeyboardOutput::sc2_5) {
        Serial.println("testQueue: full buffer assert 5");
    }
    if (buf.pop() != ps2::KeyboardOutput::sc2_6) {
        Serial.println("testQueue: full buffer assert 6");
    }
    if (buf.pop() != ps2::KeyboardOutput::none) {
        Serial.println("testQueue: buffer failed pop on empty 3");
    }
}

static byte f1_f4[4] = { 0x07, 0x0f, 0x17, 0x1f };
static byte f7_f8[4] = { 0x37, 0x3f };

// the loop function runs over and over again until power down or reset
void loop() {
    diagnostics.setLedIndicator<LED_BUILTIN_RX, ps2::DiagnosticsLedBlink::heartbeat>();

    int pin1Value = digitalRead(switch1Pin);
    if (!pin1Value && oldSwitch1PinValue) {
        Serial.print("Reset...");

        bool resetOk = ps2Keyboard.reset();
        Serial.println(resetOk ? "ok" : "error");

        uint16_t id = ps2Keyboard.readId();
        Serial.print("id: ");
        Serial.println(id, HEX);

        ps2::ScanCodeSet scanCodeSet = ps2Keyboard.getScanCodeSet();
        Serial.print("scancodeset: ");
        Serial.println((byte)scanCodeSet, HEX);

        printResult("echo", ps2Keyboard.echo());

        Serial.println("self-test complete");
    }
    oldSwitch1PinValue = pin1Value;

    int pin2Value = digitalRead(switch2Pin);
    if (pin2Value != oldSwitch2PinValue)
    {
        if (pin2Value) {
            printResult("enable", ps2Keyboard.enable());
        }
        else {
            printResult("disable", ps2Keyboard.disable());
        }
        oldSwitch2PinValue = pin2Value;
    }

    ps2::KeyboardOutput scanCode = ps2Keyboard.readScanCode();
    if (scanCode != ps2::KeyboardOutput::none) {
        Serial.println((byte)scanCode, HEX);

        switch (scanCode) {
            case ps2::KeyboardOutput::sc2_1: {
                waitForUnmake(scanCode);
                ps2::ScanCodeSet scanCodeSet = ps2Keyboard.getScanCodeSet();
                Serial.print("scancodeset: ");
                Serial.println((byte)scanCodeSet, HEX);
                break;
            }
            case ps2::KeyboardOutput::sc2_2: {
                waitForUnmake(scanCode);
                printResult("set pcat scan code set (2)", ps2Keyboard.setScanCodeSet(ps2::ScanCodeSet::pcat));
                break;
            }
            case ps2::KeyboardOutput::sc2_3: {
                waitForUnmake(scanCode);
                printResult("set ps2 scan code set (3)", ps2Keyboard.setScanCodeSet(ps2::ScanCodeSet::ps2));
                break;
            }
            case ps2::KeyboardOutput::sc2_4: {
                waitForUnmake(scanCode);
                printResult("disable breaks", ps2Keyboard.disableBreakCodes());
                break;
            }
            case ps2::KeyboardOutput::sc2_5: {
                waitForUnmake(scanCode);
                printResult("enable break & typematic", ps2Keyboard.enableBreakAndTypematic());
                break;
            }
            case ps2::KeyboardOutput::sc2_6: {
                waitForUnmake(scanCode);
                printResult("slow typematic", ps2Keyboard.setTypematicRateAndDelay(ps2::TypematicRate::slowestRate, ps2::TypematicStartDelay::longestDelay));
                break;
            }
            case ps2::KeyboardOutput::sc2_7: {
                waitForUnmake(scanCode);
                printResult("disable typematic", ps2Keyboard.disableTypematic());
                break;
            }
            case ps2::KeyboardOutput::sc2_8: {
                waitForUnmake(scanCode);
                printResult("disable break & typematic", ps2Keyboard.disableBreakAndTypematic());
                break;
            }
            case ps2::KeyboardOutput::sc2_9: {
                waitForUnmake(scanCode);
                printResult("reset to default", ps2Keyboard.resetToDefaults());
                break;
            }
            case ps2::KeyboardOutput::sc2_q: {
                waitForUnmake(scanCode);
                printResult("LED:Num", ps2Keyboard.sendLedStatus(ps2::KeyboardLeds::numLock));
                break;
            }
            case ps2::KeyboardOutput::sc2_w: {
                waitForUnmake(scanCode);
                printResult("LED:Caps", ps2Keyboard.sendLedStatus(ps2::KeyboardLeds::capsLock));
                break;
            }
            case ps2::KeyboardOutput::sc2_e: {
                waitForUnmake(scanCode);
                printResult("LED:Scroll", ps2Keyboard.sendLedStatus(ps2::KeyboardLeds::scrollLock));
                break;
            }
            case ps2::KeyboardOutput::sc2_r: {
                waitForUnmake(scanCode);
                printResult("LED:none", ps2Keyboard.sendLedStatus(ps2::KeyboardLeds::none));
                break;
            }
            case ps2::KeyboardOutput::sc2_u: {
                waitForUnmake(scanCode);
                printResult("disable breaks F1-F4", ps2Keyboard.disableBreakCodes(f1_f4, 4));
                printResult("disable breaks F7-F8", ps2Keyboard.disableBreakCodes(f7_f8, 2));
                printResult("enable", ps2Keyboard.enable());
                break;
            }
            case ps2::KeyboardOutput::sc2_i: {
                waitForUnmake(scanCode);
                printResult("disable typematic F1-F4", ps2Keyboard.disableTypematic(f1_f4, 4));
                printResult("disable typematic F7-F8", ps2Keyboard.disableTypematic(f7_f8, 2));
                printResult("enable", ps2Keyboard.enable());
                break;
            }
            case ps2::KeyboardOutput::sc2_o: {
                waitForUnmake(scanCode);
                printResult("disable break & typematic F1-F4", ps2Keyboard.disableBreakAndTypematic(f1_f4, 4));
                printResult("disable break & typematic F7-F8", ps2Keyboard.disableBreakAndTypematic(f7_f8, 2));
                printResult("enable", ps2Keyboard.enable());
                break;
            }
            case ps2::KeyboardOutput::sc2_t: {
                waitForUnmake(scanCode);
                testQueue();
                Serial.println("testQueue done");
                break;
            }
            case ps2::KeyboardOutput::sc2_tab: {
                waitForUnmake(scanCode);
                diagnostics.sendReport(Serial);
                Serial.println();
                break;
            }
            case ps2::KeyboardOutput::sc2_h: {
                waitForUnmake(scanCode);
                // Simulating what happens if you wait for startup but the keyboard doesn't
                //  generate one - either because it's a strange keyboard or because the
                //  arduino rebooted but the keyboard didn't.
                // First, clear any errors, since this is sure to generate a new one and the
                //  slate would be clean in a reboot scenario anyway.
                diagnostics.reset();
                Serial.println("Starting awaitStartup");
                // Warn the tester - it's also a thing to validate that any keystroke stops
                //  the wait and leaves that keystroke on the queue.
                bool result = ps2Keyboard.awaitStartup();
                Serial.print("awaitStartup returned ");
                Serial.print(result ? "true" : "false");
                Serial.print(" Diagnostics:");
                diagnostics.sendReport(Serial);
                diagnostics.reset();
                Serial.println();
            }
        }

        ps2::KeyCode translated = translator.translatePs2Keycode(scanCode);
        if (translated != ps2::KeyCode::PS2_NONE) {
            Serial.print("<");
            Serial.print((uint16_t)translated, HEX);
            Serial.println(">");
        }
    }
}
