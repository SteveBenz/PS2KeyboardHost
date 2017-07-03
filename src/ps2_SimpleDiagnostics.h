#pragma once
#include <stdint.h>
#include "ps2_Keyboard.h"
#include "ps2_KeyboardOutput.h"
#include <util/atomic.h>

namespace ps2 {
    enum class DiagnosticsLedBlink {
        none,
        keepAliveBlink,
        onErrorOnly,
    };
    enum class DiagnosticsOutputTarget {
        none,
        keyboard,
        serial,
        both,
    };

    template <uint16_t Size>
    class SimpleDiagnostics
    {
        byte data[Size];
        int index = -1;
        uint16_t failureCodes = 0;

        enum class Ps2Code : uint8_t {
            packetDidNotStartWithZero = 0,
            parityError = 1,
            packetDidNotEndWithOne = 2,
            packetIncomplete = 3,
            sendFrameError = 4,
            bufferOverflow = 5,
            incorrectResponse = 6,
            noResponse = 7,
            noTranslationForKey = 8,

            sentByte = 16,
            receivedByte = 17,
        };

        void recordFailure(uint8_t code) {
            ATOMIC_BLOCK(ATOMIC_FORCEON) {
                this->failureCodes |= 1 << code;
            }
        }

        void pushByte(byte b)
        {
            ATOMIC_BLOCK(ATOMIC_FORCEON) {
                if (index < 0) {
                    data[-1 - index] = b;
                    index = (index == -Size) ? 0 : index - 1;
                }
                else {
                    data[index] = b;
                    index = (index == Size - 1) ? 0 : index + 1;
                }
            }
        }

    protected:
        static const int firstUnusedFailureCode = 9;
        static const int firstUnusedInfoCode = 18;

        template <typename E>
        void push(E code) {
            push((byte)code);
        }
        template <typename E1,typename E2>
        void push(E1 code, E2 extraData1) {
            push((byte)code, (uint8_t)extraData1);
        }
        template <typename E1, typename E2, typename E3>
        void push(E1 code, E2 extraData1, E3 extraData2) {
            push((byte)code, (byte)extraData1, (byte)extraData2);
        }

        void push(byte code) {
            this->recordFailure(code);
            pushByte(code << 2);
        }
        void push(byte code, byte extraData1) {
            this->recordFailure(code);
            pushByte((code << 2) | 1);
            pushByte(extraData1);
        }
        void push(byte code, byte extraData1, byte extraData2) {
            this->recordFailure(code);
            pushByte((code << 2) | 2);
            pushByte(extraData1);
            pushByte(extraData2);
        }

    public:
        template <typename Target>
        void sendReport(Target &printTo) {
            // This report isn't the least bit human-readable.  While developing this software, it became
            //  clear to me that if you have an opportunity to write code in either the Arduino or on a PC,
            //  you choose the PC every time because the development experience is so much better and you
            //  can have richer output.  I developed something to read this sequence, but it's not something
            //  I'm comfortable sharing, because it's a Windows-only app, and the quality isn't really ready
            //  for sharing.  Rather than invest in that any more, I feel that it should be rewritten as a
            //  JavaScript web page on the wiki or someplace like that.
            printTo.print("{");
            printTo.print(this->failureCodes, 16);
            printTo.print(":");
            int startInclusive = index < 0 ? 0 : index;
            int end = index < 0 ? -1 - index : index;
            if (index < 0)
            {
                for (int i = 0; i < -1 - index; ++i) {
                    // Can't just do print(data,16), as it'll get truncated if it's < 16.
                    printTo.print(data[i] >> 4, 16);
                    printTo.print(data[i] & 0xf, 16);
                }
            }
            else {
                for (int i = index; i < Size + index; ++i) {
                    printTo.print(data[i % Size] >> 4, 16);
                    printTo.print(data[i % Size] & 0xf, 16);
                }
            }
            printTo.print("}");
        }

        bool anyErrors() const { return this->failureCodes != 0; }

        void reset() {
            this->failureCodes = 0;
            this->index = -1;
        }

        template <uint8_t DiagnosticLedPin = LED_BUILTIN, DiagnosticsLedBlink ledBehavior = DiagnosticsLedBlink::onErrorOnly>
        void setLedIndicator() {
            switch (ledBehavior) {
            case DiagnosticsLedBlink::keepAliveBlink:
                digitalWrite(DiagnosticLedPin, (millis() & (failureCodes != 0 ? 128 : 1024)) ? HIGH : LOW);
                break;
            case DiagnosticsLedBlink::onErrorOnly:
                if (failureCodes != 0) {
                    digitalWrite(DiagnosticLedPin, (millis() & 128) ? HIGH : LOW);
                }
                break;
            default:
                // Do nothing;
                break;
            }
        }

        void packetDidNotStartWithZero() { this->push(Ps2Code::packetDidNotStartWithZero); }
        void parityError() { this->push(Ps2Code::parityError); }
        void packetDidNotEndWithOne() { this->push(Ps2Code::packetDidNotEndWithOne); }
        void packetIncomplete() { this->push(Ps2Code::packetIncomplete); }
        void sendFrameError() { this->push(Ps2Code::sendFrameError); }
        void bufferOverflow() { this->push(Ps2Code::bufferOverflow); }
        void incorrectResponse(KeyboardOutput scanCode, KeyboardOutput expectedScanCode) {
            this->push(Ps2Code::incorrectResponse, scanCode, expectedScanCode);
        }
        void noResponse(KeyboardOutput expectedScanCode) {
            this->push(Ps2Code::noResponse, expectedScanCode);
        }
        void noTranslationForKey(bool isExtended, KeyboardOutput code) {
            this->push(Ps2Code::noTranslationForKey, isExtended, code);
        }
        void sentByte(byte b) { this->push(Ps2Code::sentByte, b); }
        void receivedByte(byte b) { this->push(Ps2Code::receivedByte, b); }
    };
}