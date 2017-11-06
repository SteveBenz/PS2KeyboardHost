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
#pragma once
#include <stdint.h>
#include "ps2_Keyboard.h"
#include "ps2_KeyboardOutput.h"
#include <util/atomic.h>

namespace ps2 {
    /** \brief Used with \ref SimpleDiagnostics to control the behavior of a pin that
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
    template <uint16_t Size = 60, uint16_t LastErrorSize = 30>
    class SimpleDiagnostics
    {
        byte data[Size];
        byte lastError[LastErrorSize];
        uint16_t bytesInLastError = 0;
        int index = -1;
        uint16_t failureCodes = 0;
        uint32_t millisAtLastRecording = 0;

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
            startupFailure = 9,
            _firstUnusedError = 10,

            sentByte = 16,
            receivedByte = 17,
            pause = 18, // Data is one byte, milliseconds+4/8 (0 to 2.043sec)
            clockLineGlitch = 19, // data is # of bits received
            reserved2 = 20,
            reserved3 = 21,
            // Reserve a few so that more info-level events can come in without jacking up
            // any existing readers.
            _firstUnusedInfo = 22,
        };

        void recordFailure(uint8_t code) {
            if (code < 16) {
                ATOMIC_BLOCK(ATOMIC_FORCEON) {
                    this->failureCodes |= 1 << code;
                }

                uint16_t numBytesCopied = 0;
                uint16_t i = index < 0 ? -1 - index : index;
                i = (i == 0 ? Size : i) - 1;
                while (index >= 0 || i != Size - 1)
                {
                    byte bytesInWord = 1 + (this->data[i] & 0x3);
                    if (numBytesCopied + bytesInWord > LastErrorSize) {
                        break;
                    }

                    while (bytesInWord > 0) {
                        this->lastError[numBytesCopied] = this->data[i];
                        i = (i == 0 ? Size : i) - 1;
                        ++numBytesCopied;
                        --bytesInWord;
                    }
                }
                bytesInLastError = numBytesCopied;
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

        void pushRaw(byte code) {
            pushByte(code << 2);
            this->recordFailure(code);
        }
        void pushRaw(byte code, byte extraData1) {
            pushByte(extraData1);
            pushByte((code << 2) | 1);
            this->recordFailure(code);
        }
        void pushRaw(byte code, byte extraData1, byte extraData2) {
            pushByte(extraData2);
            pushByte(extraData1);
            pushByte((code << 2) | 2);
            this->recordFailure(code);
        }

        void recordPause()
        {
            unsigned long millisNow = millis();
            unsigned long timeDelta = millisNow - millisAtLastRecording;
            if (timeDelta >= 4 && timeDelta < 2044) {
                pushRaw((byte)Ps2Code::pause, (byte)((timeDelta + 4) >> 3));
                millisAtLastRecording = millisNow;
            }
            else if (timeDelta >= 2044) {
                unsigned long lowResDelay = (timeDelta + 32) >> 6;
                if (lowResDelay > 0xffff) {
                    lowResDelay = 0xffff;
                }
                pushRaw((byte)Ps2Code::pause, (byte)(lowResDelay >>8), (byte)(lowResDelay & 0xff));
                millisAtLastRecording = millisNow;
            }
            // Else it's too short to make note of
        }

    protected:
        static const uint8_t firstUnusedFailureCode = (uint8_t)Ps2Code::_firstUnusedError;
        static const uint8_t firstUnusedInfoCode = (uint8_t)Ps2Code::_firstUnusedInfo;

        template <typename E>
        void push(E code) {
            recordPause();
            pushRaw((byte)code);
        }
        template <typename E1,typename E2>
        void push(E1 code, E2 extraData1) {
            recordPause();
            pushRaw((byte)code, (uint8_t)extraData1);
        }
        template <typename E1, typename E2, typename E3>
        void push(E1 code, E2 extraData1, E3 extraData2) {
            recordPause();
            pushRaw((byte)code, (byte)extraData1, (byte)extraData2);
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

            // This content ends up getting reversed
            for (int i = bytesInLastError-1; i >= 0; --i) {
                printTo.print(this->lastError[i] >> 4, 16);
                printTo.print(this->lastError[i] & 0xf, 16);
            }
            printTo.print("|");

            if (this->index < 0)
            {
                for (int i = 0; i < -1 - this->index; ++i) {
                    // Can't just do print(data,16), as it'll get truncated if it's < 16.
                    printTo.print(this->data[i] >> 4, 16);
                    printTo.print(this->data[i] & 0xf, 16);
                }
            }
            else {
                for (int i = this->index; i < Size + this->index; ++i) {
                    printTo.print(this->data[i % Size] >> 4, 16);
                    printTo.print(this->data[i % Size] & 0xf, 16);
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
        void startupFailure() { this->push(Ps2Code::startupFailure); }
        void clockLineGlitch(uint8_t numBitsSent) {
            this->push(Ps2Code::clockLineGlitch, numBitsSent);
        }

        void sentByte(byte b) { this->push(Ps2Code::sentByte, b); }
        void receivedByte(byte b) { this->push(Ps2Code::receivedByte, b); }
    };
}