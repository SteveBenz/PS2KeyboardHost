#pragma once
#include <stdint.h>
#include "ps2_Keyboard.h"
#include "ps2_KeyboardOutput.h"
#include <util/atomic.h>

namespace ps2 {
    /** \brief Used with \ref Simplediagnostics to control the behavior of a pin that
     *         will signal the device's user that an error has been recorded.
     */
    enum class DiagnosticsLedBlink {
        /** The LED blinks slowly when there's no error and fast when an error has happened.
         *   (If you see a slow blink, then you at least know that the device is cycling
         *   through the loop and not stuck in a loop somewhere.)
         */
        heartbeat,

        /** The pin is left HIGH unless an error has been recorded, in which case it will
            be toggled on and off rapidly. */
        blinkOnError,

        /** The pin is left LOW until an error is registered, at which point it switches to HIGH */
        toggleHigh,

        /** The pin is left HIGH until an error is registered, at which point it switches to LOW */
        toggleLow
    };

    /** \brief A basic recorder for events coming from the PS2 keyboard class library.
     *
     *  \details
     *
     *   It addition to recording events, it can also blink an LED when an error has been
     *   recorded.  It can dump its output to any print-capable class, such as the Serial port
     *   or a USB keyboard.
     *
     *   \section Usage Basic Usage
     *
     *   \code
     *    typedef ps2::SimpleDiagnostics<32> Diagnostics;
     *    static Diagnostics diagnostics;
     *    static ps2::Keyboard<4,2,1,Diagnostics> ps2Keyboard(diagnostics);
     *
     *    void loop() {
     *      diagnostics.setLedIndicator<LED_BUILTIN_RX, ps2::DiagnosticsLedBlink::heartbeat>();
     *      if ( <magic-user-gesture> ) {
     *        diagnostics.sendReport(Serial);
     *        diagnostics.reset();
     *      }
     *   \endcode
     *
     *   Each event is recorded as a series of one or more bytes in a circular queue.  The queue
     *   is meant to be read right-to-left, with the newest events at the right.  Thus if an
     *   event has multiple bytes in it, the extra bytes will be pushed onto the queue first.
     *   The last byte pushed contains the event ID and a count of the number of extra bytes.
     *   The number of extra bytes is in the lower two bits and the event ID in the upper 6.
     *   If the event ID is less than 16, it is taken to be an error.
     *
     *   There are two bytes reserved in the structure for storing all the errors that have
     *   happened since the recorder was last reset (as a bit-field).
     *
     *   \section Subclassing Subclassing
     *
     *    If you want to record events from other parts of your application, you can create a
     *    subclass that defines new events.  The Ps2ToUsbKeyboardAdapter example demonstrates
     *    how to do that:
     *
     *    \code
     *     class Diagnostics
     *         : public ps2::SimpleDiagnostics<254>
     *     {
     *       typedef ps2::SimpleDiagnostics<254> base;
     *       enum class UsbTranslatorAppCode : uint8_t {
     *         sentUsbKeyDown = 0 + base::firstUnusedInfoCode,
     *         sentUsbKeyUp = 1 + base::firstUnusedInfoCode,
     *       };
     *     public:
     *       void sentUsbKeyDown(byte b) { this->push((uint8_t)UsbTranslatorAppCode::sentUsbKeyDown, b); }
     *       void sentUsbKeyUp(byte b) { this->push((uint8_t)UsbTranslatorAppCode::sentUsbKeyUp, b); }
     *     };
     *    \endcode
     *
     *    Note that there are a maximum of 16 error identifiers and a maximum of 48 non-error identifiers.
     *
     *    \section AuthorsNote Author's Note
     *     Debugging and logging are often areas where we wish we could do more, but they can be
     *     infinite pits of labor if you let them.  Further, when neglected, the rest of the project
     *     becomes an infinite pit of labor...  There are two things I would wish for in the future:
     *     I wish it would store more data and, in particular, record the 10 keystrokes or
     *     other events before and after any error.  I would also like to see a website-based
     *     diagnostic data interpreter.
     *
     *  \tparam Size  The number of bytes to use for recording events.
     */
    template <uint16_t Size = 60>
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

        void push(byte code) {
            this->recordFailure(code);
            pushByte(code << 2);
        }
        void push(byte code, byte extraData1) {
            pushByte(extraData1);
            this->recordFailure(code);
            pushByte((code << 2) | 1);
        }
        void push(byte code, byte extraData1, byte extraData2) {
            pushByte(extraData2);
            pushByte(extraData1);
            this->recordFailure(code);
            pushByte((code << 2) | 2);
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

    public:
        /** \brief Dumps all event data to a print-based class
         *  \details It's a good idea to call \ref reset after calling this.
         */
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

        /** \brief Returns true if any errors have been recorded since the last call to \ref reset. */
        bool anyErrors() const { return this->failureCodes != 0; }

        /** \brief Clears all recorded data. */
        void reset() {
            this->failureCodes = 0;
            this->index = -1;
        }

        /** \brief Enables you to have a blinking indicator when an error happens.
         *  \tparam DiagnosticLedPin The led's pin - usually one of the built-in pins.
         *  \tparam LedBehavior Controls what the LED does.  See \ref DiagnosticsLedBlink
         *                      for the available behaviors.
         */
        template <uint8_t DiagnosticLedPin = LED_BUILTIN, DiagnosticsLedBlink LedBehavior = DiagnosticsLedBlink::blinkOnError>
        void setLedIndicator() {
            bool value;
            switch (LedBehavior) {
            case DiagnosticsLedBlink::heartbeat:
                value = (millis() & (failureCodes != 0 ? 128 : 1024));
                break;
            case DiagnosticsLedBlink::blinkOnError:
                value = (failureCodes == 0 || (millis() & 128));
                break;
            case DiagnosticsLedBlink::toggleHigh:
                value = (failureCodes != 0);
                break;
            case DiagnosticsLedBlink::toggleLow:
                value = (failureCodes == 0);
                break;
            }
            digitalWrite(DiagnosticLedPin, value ? HIGH : LOW);
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