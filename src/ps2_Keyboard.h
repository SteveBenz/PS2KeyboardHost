/*
Keyboard.h - PS2 Keyboard library
Copyright (c) 2007 Free Software Foundation.  All right reserved.

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
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h" // for attachInterrupt, FALLING, HIGH & LOW
#else
#include "WProgram.h"
#endif
#include <stdint.h>
#include <util/atomic.h>
#include "ps2_NullDiagnostics.h"
#include "ps2_KeyboardLeds.h"
#include "ps2_KeyboardOutput.h"
#include "ps2_TypematicRate.h"
#include "ps2_TypematicStartDelay.h"
#include "ps2_ScanCodeSet.h"
#include "ps2_Parity.h"
#include "ps2_KeyboardOutputBuffer.h"

namespace ps2 {

    /*
     * Excellent references:
     *  http://www.computer-engineering.org/ps2keyboard/
     */

    class NullReceiveFunctor {
    public:
        void operator()(KeyboardOutput o) {}
    };

    /**
     * \brief
     *  Instances of this class can be used to interface with a PS2 keyboard.  This class does not
     *  decode the keystroke protocol, but rather provides the data from the keyboard in a raw form.
     *  You either need to write code that directly understands what the PS2 is providing, or use
     *  one of the provided translator classes (e.g. \ref AnsiTranslator).
     *
     * \tparam DataPin The pin number of the pin that's connected to the PS2 data wire.
     * \tparam ClockPin The pin number of the pin that's connected to the PS2 clock wire.  This
     *                  pin must be one that supports interrupts on your board.
     * \tparam BufferSize The size of the internal buffer that stores the bytes coming from the
     *                    keyboard between the time they're received from the ClockPin-based
     *                    interrupts and the time they're consumed by calls to \ref ReadScanCode.
     *                    If you are calling that method very frequently (many times per millisecond)
     *                    then 1 is enough.  Expect each keystroke to eat about 4 bytes, so 16
     *                    can hold up to 4 keystrokes.  There's nothing wrong with larger numbers,
     *                    but you probably want some amount of responsiveness to user commands.
     * \tparam Diagnostics A class that will record any unpleasantness that befalls your program
     *                     such as the aforementioned buffer overflows.  This is purely a debugging
     *                     aid.  If you don't need to be doing any debugging, you can stick wit the
     *                     default, \ref NullDiagnostics, which has no-op implementations for all the
     *                     event handlers.  The C++ compiler will optimize empty implementations
     *                     completely away, so you pay no penalty for having this debug code in your
     *                     project if you don't use it.  The \ref SimpleDiagnostics implementation
     *                     provides an implementation that records all the events.
     *
     * \details
     *  A great source of information about the PS2 keyboard can be found here: http://www.computer-engineering.org/ps2keyboard/.
     *  This documentation assumes you have a basic grasp of that.
     *
     *  Most programs will use this class like this:
     *
     * \code
     * static ps2::Keyboard<4,2> ps2Keyboard(diagnostics);
     *
     * void setup() {
     *   ps2Keyboard.begin();
     *   ps2Keyboard.reset();
     *   // more keyboard setup if needed (e.g. set the scan code set)
     * }
     *
     * void loop() {
     *   ps2::KeyboardOutput scanCode = ps2Keyboard.readScanCode();
     *   if (scanCode != ps2::KeyboardOutput::none) {
     *     respondTo(scanCode);
     *   };
     * }
     * \endcode
     *
     *  In this example, the keyboard is wired up with the data pin connected to pin 4 and
     *  the clock pin connected to pin 2.  The setup function should start up the keyboard.
     *  In many cases, you really don't have to do anything other than call 'begin' to get
     *  going.  Here, it calls 'reset' to undo whatever mode the keyboard might already be in.
     *  In the real world, this is unlikely to ever be necessary, but there you are.  A more
     *  likely move would be to set the keyboard up in the PS/2 scan code set and perhaps
     *  disabling typematic or enabling unmake codes to make it easier to interpret the
     *  keyboard's protocol.
     *
     *  You should poll the keyboard from your loop implementation, as illustrated here.
     *
     *  With all things Arduino, you should understand the performance.  \readScanCode takes
     *  only a few machine instructions to complete.  The methods that push data to the keyboard
     *  take longer (on the order of milliseconds) because of the handshake between the two
     *  devices and the throttling of sending one bit at a time at 10KHz.
     *
     *  All of the keyboard setup methods return a bool that reports success or not; you can
     *  test them if you feel it necessary, but few applications will really need to.
     *
     *  The biggest source of legitimate error is long-running interrupts which cause
     *  the PS2 clock interrupt to be skipped.  The response to the clock pin must be swift
     *  and consistent, else you'll get garbled messages.  The protocol is just robust
     *  enough to know that a failure has happened, but not robust enough to fully recover.
     *  If you're going to have another interrupt source in your project, see to it that
     *  its interrupt handler is as quick as possible.  For its part, the PS2 handler is
     *  just a few instructions in all cases.
     *
     *  If you supply an OnReceivedFunctor, then the operator() method of the object will be called
     *  each time a code is seen on the output line that is not been requested by this class itself.
     *  (That is, if a key is pressed, it'll get called for the bytes associated with that, but if
     *  you call the method to set the LED, the Ack code that the keyboard sends back does not generate
     *  a call.
     */
    template<int DataPin, int ClockPin, int BufferSize = 16, typename Diagnostics = NullDiagnostics, typename OnReceivedFunctor = NullReceiveFunctor>
    class Keyboard {
        static const uint8_t immediateResponseTimeInMilliseconds = 10;

        Diagnostics *diagnostics;
        OnReceivedFunctor *onRecievedFunctor;

        // These are not marked as volatile because they are only modified in the interrupt
        // handler and at startup (before the interrupt handler is enabled).
        uint8_t ioByte = 0;
        uint8_t bitCounter = 0;
        uint32_t failureTimeMicroseconds = 0;
        uint32_t lastReadInterruptMicroseconds = 0;
        Parity parity = Parity::even;
        bool expectingResult = false;
        bool receivedHasFramingError = false;

        // This guy is marked volatile because it's exchanged between the interrupt handler and normal code.
        KeyboardOutputBuffer<BufferSize, Diagnostics> inputBuffer;

        // Commands sent from the host to the ps2 keyboard.  Private to this class because
        //  the point of the class is to encapsulate the protocol.
        enum class ps2CommandCode : uint8_t {
            reset = 0xff,
            resend = 0xfe,
            disableBreakAndTypematicForSpecificKeys = 0xfd,
            disableTypematicForSpecificKeys = 0xfc,
            disableBreaksForSpecificKeys = 0xfb,
            enableBreakAndTypeMaticForAllKeys = 0xfa,
            disableBreakAndTypematicForAllKeys = 0xf9,
            disableTypematicForAllKeys = 0xf8,
            disableBreaksForAllKeys = 0xf7,
            useDefaultSettings = 0xf6,
            disable = 0xf5,
            enable = 0xf4,
            setTypematicRate = 0xf3,
            readId = 0xf2,
            setScanCodeSet = 0xf0,
            echo = 0xee,
            setLeds = 0xed,
        };

        void readInterruptHandler() {
            // The timing of the PS2 keyboard is such that you really need to read the data
            // line just as fast as possible.  If you do it with digitalRead, it'll work right
            // almost all the time (like one character in 100 will suffer a failure - with the
            // keyboards I have to-hand.)
            //
            // Although this looks fantastically more complicated than plain digitalRead, the
            // instruction count is lower.  It'd be even lower if the methods listed here were
            // implemented a little better (e.g. as template methods rather than macros).
            uint8_t dataPinValue = (*portInputRegister(digitalPinToPort(DataPin)) & digitalPinToBitMask(DataPin)) ? 1 : 0; // ==digitalRead(DataPin);

            lastReadInterruptMicroseconds = micros();
            // TODO: There should be code here that detects if bitCounter > 0 && the time since
            //  the last read interrupt is > a millisecond and, if so, log that and reset
            //  bitCounter.

            switch (bitCounter)
            {
            case 0:
                if (dataPinValue == 0) {
                    receivedHasFramingError = false;
                }
                else {
                    this->diagnostics->packetDidNotStartWithZero();

                    // If we get a failure here, it should mean that previous byte is
                    // not actually framed correctly and the stop bit and parity bit somehow
                    // matched our expectations (or maybe they didn't and we got here anyway?)
                    receivedHasFramingError = true;
                    failureTimeMicroseconds = lastReadInterruptMicroseconds;
                }
                ++bitCounter;
                parity = Parity::even;
                break;
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
                if (dataPinValue)
                {
                    ioByte |= (1 << (bitCounter - 1));
                    parity ^= 1;
                }
                ++bitCounter;
                break;
            case 9:
                if (parity != (Parity)dataPinValue) {
                    this->diagnostics->parityError();
                    receivedHasFramingError = true;
                    failureTimeMicroseconds = lastReadInterruptMicroseconds;
                }
                ++bitCounter;
                break;
            case 10:
                if (dataPinValue == 0) {
                    this->diagnostics->packetDidNotEndWithOne();
                    receivedHasFramingError = true;
                    failureTimeMicroseconds = lastReadInterruptMicroseconds;
                }

                if (!receivedHasFramingError) {
                    this->diagnostics->receivedByte(ioByte);
                    this->inputBuffer.push((KeyboardOutput)ioByte);
                }
                bitCounter = 0;
                ioByte = 0;
            }
        }

        void writeInterruptHandler() {
            int valueToSend;

            switch (bitCounter)
            {
            case 0:
                ++bitCounter;
                break;
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
                valueToSend = (this->ioByte & (1 << (bitCounter - 1))) ? HIGH : LOW;
                digitalWrite(DataPin, valueToSend);
                parity ^= valueToSend;
                ++bitCounter;
                break;
            case 9:
                digitalWrite(DataPin, (uint8_t)parity);
                ++bitCounter;
                break;
            case 10:
                pinMode(DataPin, INPUT_PULLUP);
                ++bitCounter;
                break;
            case 11:
                if (digitalRead(DataPin) != LOW) {
                    // It'd be better to actually resend, but I don't think it's a thing,
                    //  and it seems impossible to test anyway.
                    this->diagnostics->sendFrameError();
                }

                enableReadInterrupts();
                break;
            }
        }

        void sendByte(uint8_t byte1) {
            // ISSUE:  If we really want to be tight about it, we should check to see if the keyboard
            //   is already in the middle of transmitting something before we launch into this.

            detachInterrupt(digitalPinToInterrupt(ClockPin));

            // Inhibit communication by pulling Clock low for at least 100 microseconds.
            pinMode(ClockPin, OUTPUT);
            digitalWrite(ClockPin, LOW);
            delayMicroseconds(120);

            // Make sure when interrupts resume, we start in the right state - note that we
            //  start sending data immediately on the first byte, because we will initiate
            //  the start bit in this clock pulse.
            this->receivedHasFramingError = false;
            this->inputBuffer.clear();
            this->bitCounter = 0;
            this->parity = Parity::even;
            this->ioByte = byte1;
            attachInterrupt(digitalPinToInterrupt(ClockPin), Keyboard::staticWriteInterruptHandler, FALLING);

            // Apply "Request-to-send" by pulling Data low, then release Clock.
            pinMode(DataPin, OUTPUT);
            digitalWrite(DataPin, LOW);
            pinMode(ClockPin, INPUT_PULLUP);
        }

        void enableReadInterrupts() {
            this->receivedHasFramingError = false;
            this->bitCounter = 0;
            this->ioByte = 0;
            this->parity = Parity::even;
            this->inputBuffer.clear(); // gratuitious?  Probably...

            attachInterrupt(digitalPinToInterrupt(ClockPin), Keyboard::staticReadInterruptHandler, FALLING);
        }

        static Keyboard *instance;

        static void staticReadInterruptHandler() {
            instance->readInterruptHandler();
        }

        static void staticWriteInterruptHandler() {
            instance->writeInterruptHandler();
        }

        KeyboardOutput expectResponse(uint16_t timeoutInMilliseconds = immediateResponseTimeInMilliseconds)
        {
            unsigned long startMilliseconds = millis();
            unsigned long stopMilliseconds = startMilliseconds + timeoutInMilliseconds;
            unsigned long nowMilliseconds;
            KeyboardOutput actualResponse;
            expectingResult = true;
            do
            {
                actualResponse = this->readScanCode();
                nowMilliseconds = millis();
            } while (actualResponse == KeyboardOutput::none && (nowMilliseconds < stopMilliseconds || (stopMilliseconds < startMilliseconds && startMilliseconds <= nowMilliseconds)));
            expectingResult = false;

            if (actualResponse == KeyboardOutput::none) {
                this->diagnostics->noResponse(KeyboardOutput::none);
            }
            return actualResponse;
        }

        bool expectResponse(KeyboardOutput expectedResponse, uint16_t timeoutInMilliseconds = immediateResponseTimeInMilliseconds) {
            KeyboardOutput actualResponse = expectResponse(timeoutInMilliseconds);

            if (actualResponse == KeyboardOutput::none) {
                // diagnostics already reported
                return false;
            }
            else if ( actualResponse != expectedResponse ) {
                this->diagnostics->incorrectResponse(actualResponse, expectedResponse);
                return false;
            }
            else {
                return true;
            }
        }

        bool expectAck() {
            return this->expectResponse(KeyboardOutput::ack);
        }

        bool sendCommand(ps2CommandCode command) {
            return this->sendData((byte)command);
        }

        bool sendCommand(ps2CommandCode command, byte data) {
            return this->sendCommand(command)
                && this->sendData(data);
        }

        bool sendCommand(ps2CommandCode command, const byte *data, int numBytes) {
            if (!this->sendCommand(command)) {
                return false;
            }
            for (int i = 0; i < numBytes; ++i) {
                if (!this->sendData(data[i])) {
                    return false;
                }
            }
            return true;
        }

        bool sendData(byte data) {
            this->diagnostics->sentByte(data);
            this->sendByte(data);
            bool isOk = this->expectAck();
            if (!isOk)
            {
                this->enableReadInterrupts();
            }
            return isOk;
        }

        void sendNack() {
            this->sendByte((byte)ps2CommandCode::resend);
        }

    public:
        Keyboard(Diagnostics &diagnostics = *Diagnostics::defaultInstance())
            : inputBuffer(diagnostics)
        {
            this->diagnostics = &diagnostics;
            instance = this;
        }


        /**
        * Starts the keyboard "service" by registering the external interrupt.
        * setting the pin modes correctly and driving those needed to high.
        * The best place to call this method is in the setup routine.
        */
        void begin() {
            pinMode(ClockPin, INPUT_PULLUP);
            pinMode(DataPin, INPUT_PULLUP);

            // If the pin that support PWM output, we need to turn it off
            // before getting a digital reading.  DigitalRead does that
            digitalRead(DataPin);

            this->enableReadInterrupts();
        }

        /** Returns the last code sent by the keyboard.  You should call this function frequently
         *  (several times per millisecond) as there is no buffering.  If there is nothing to read,
         *  this method will return 'none'.  It can also return 'garbled' if there's been a framing
         *  error.  You should probably just ignore this result, the keyboard will attempt to retry,
         *  but that's far from a sure-thing.
         */
        KeyboardOutput readScanCode() {
            KeyboardOutput code = this->inputBuffer.pop();

            if (code == KeyboardOutput::none && this->receivedHasFramingError)
            {
                // NACK's affect what the device perceives as the last character sent, so if we
                //  interrupt immediately, it might decide to send the last scan code (which we
                //  already have) again.  The clock is between 17KHz and 10KHz, so the fastest
                //  send time for the 12 bits is going to be around 12*1000000/17000 700us and the
                //  slowest would be 1200us.  Again, that's the full cycle.  Most of the errors
                //  are going to be found at the parity & stop bit, so we'll wait 200us, as a guess,
                //  before asking for a retry.
                if (failureTimeMicroseconds + 200 > micros()) {
                    return KeyboardOutput::none;
                }
                this->sendNack();
                return KeyboardOutput::garbled;
            }

            return code;
        }

        /** \brief Sets the keyboard's onboard LED's.
         *  \details
         *   This method takes several milliseconds and ties up the communication line with the keyboard.
         *   You'll probably want to keep a variable somewhere that records what the current state of the
         *   LED's are, and ensure that you only call this function when they actually change.
         *  \returns Returns true if the keyboard responded appropriately and false otherwise.
         */
        bool sendLedStatus(KeyboardLeds ledStatus)
        {
            return sendCommand(ps2CommandCode::setLeds, (byte)ledStatus);
        }

        /** \brief Resets the keyboard and returns true if the keyboard appears well, false otherwise.
         *  \details
         *   This can take up to a second to complete.  (Because of the Protocol spec).
         *  \returns Returns true if the keyboard responded appropriately and false otherwise.
         */
        bool reset() {
            this->inputBuffer.clear();
            this->sendCommand(ps2CommandCode::reset);

            KeyboardOutput result = this->expectResponse(1000);
            return result == KeyboardOutput::batSuccessful;
        }

        /** \brief Returns the device ID returned by the keyboard - according to the documentation, this
         *         will always be 0xab83.  In the event of an error, this will return 0xffff.
         */
        uint16_t readId() {
            this->sendCommand(ps2CommandCode::readId);
            KeyboardOutput response = this->expectResponse();
            if (response == KeyboardOutput::none) {
                return 0xffff;
            }
            uint16_t id = ((uint8_t)response) << 8;
            response = this->expectResponse();
            if (response == KeyboardOutput::none) {
                return 0xffff;
            }
            id |= (uint8_t)response;
            return id;
        }

        /** \brief Gets the current scancode set.
         */
        ScanCodeSet getScanCodeSet() {
            if (!this->sendCommand(ps2CommandCode::setScanCodeSet, 0)) {
                return ScanCodeSet::error;
            }
            ScanCodeSet result = (ScanCodeSet)this->expectResponse();
            if (result == ScanCodeSet::pcxt || result == ScanCodeSet::pcat || result == ScanCodeSet::ps2) {
                return result;
            }
            else {
                return ScanCodeSet::error;
            }
        }

        /** \brief Sets the current scancode set.
        *   \returns Returns true if the keyboard responded appropriately and false otherwise.
        */
        bool setScanCodeSet(ScanCodeSet newScanCodeSet) {
            return this->sendCommand(ps2CommandCode::setScanCodeSet, (byte)newScanCodeSet);
        }

        /** \brief
         *    Sends the "Echo" command to the keyboard, which should send an "Echo" in return.
         *   This can be used to verifies that a keyboard is connected and working properly.
         *  \returns Returns true if the keyboard responded appropriately and false otherwise.
         */
        bool echo() {
            this->sendByte((byte)ps2CommandCode::echo);
            return this->expectResponse(KeyboardOutput::echo);
        }

        /** \brief Sets the typematic rate (how fast presses come) and the delay (the time
         *         between key down and the start of auto-repeating.
         *  \returns Returns true if the keyboard responded appropriately and false otherwise.
         */
        bool setTypematicRateAndDelay(TypematicRate rate, TypematicStartDelay startDelay) {
            byte combined = (byte)rate | (((byte)startDelay) << 4);
            return this->sendCommand(ps2CommandCode::setTypematicRate, combined);
        }

        /** \brief Restores scan code set, typematic rate, and typematic delay.
         *  \returns Returns true if the keyboard responded appropriately and false otherwise.
         */
        bool resetToDefaults()
        {
            return this->sendCommand(ps2CommandCode::useDefaultSettings);
        }

        /** \brief Allows the keyboard to start sending data again.
         *  \returns Returns true if the keyboard responded appropriately and false otherwise.
         */
        bool enable() { return this->sendCommand(ps2CommandCode::enable); }

        /** \brief Makes it so the keyboard will no longer responsd to keystrokes.
         *  \returns Returns true if the keyboard responded appropriately and false otherwise.
         */
        bool disable() { return this->sendCommand(ps2CommandCode::disable); }

        /** \brief Re-enables typematic and break (unmake, key release) for all keys.
         *  \returns Returns true if the keyboard responded appropriately and false otherwise.
         */
        bool enableBreakAndTypematic() {
            return this->sendCommand(ps2CommandCode::enableBreakAndTypeMaticForAllKeys);
        }

        /** \brief Makes the keyboard no longer send "break" or "unmake" events when keys are released.
         *  \returns Returns true if the keyboard responded appropriately and false otherwise.
         */
        bool disableBreakCodes() {
            return this->sendCommand(ps2CommandCode::disableBreaksForAllKeys);
        }

        /** \brief  Instructs the keyboard to stop sending "break" codes for some keys (that is, it
        *           disables the notifications that come when a key is released.)
        *
         *  \details
         *   This method has no effect if you are not in the ps2 scan code set!
         *
         *   After calling this method, the keyboard will be disabled. call enable to fix that.
         *
         * \param specificKeys An array consisting of valid set-3 scan codes.  If it contains a
         *                     single invalid one, mayhem could ensue.
         * \param numKeys The number of keys to change
         */
        bool disableBreakCodes(const byte *specificKeys, int numKeys) {
            return this->sendCommand(ps2CommandCode::disableBreaksForSpecificKeys, specificKeys, numKeys);
        }

        /** \brief Makes the keyboard no longer send multiple key down events while a key is held down.
         *  \returns Returns true if the keyboard responded appropriately and false otherwise.
         */
        bool disableTypematic() {
            return this->sendCommand(ps2CommandCode::disableTypematicForAllKeys);
        }

        /** \brief Re-enables typematic and break for all keys.
         *  \returns Returns true if the keyboard responded appropriately and false otherwise.
         */
        bool disableBreakAndTypematic() {
            return this->sendCommand(ps2CommandCode::disableBreakAndTypematicForAllKeys);
        }

        /** \brief  Instructs the keyboard to disable typematic (additional key down events when
         *          the user presses and holds a key) for a given set of keys.
         *  \details
         *   This method has no effect if you are not in the ps2 scan code set!
         *
         *   After calling this method, the keyboard will be disabled. call enable to fix that.
         *
         * \param specificKeys An array consisting of valid set-3 scan codes.  If it contains a
         *                     single invalid one, mayhem could ensue.
         * \param numKeys The number of keys to change
         */
        bool disableTypematic(const byte *specificKeys, int numKeys) {
            return this->sendCommand(ps2CommandCode::disableTypematicForSpecificKeys, specificKeys, numKeys);
        }

        /** \brief  Instructs the keyboard to disable typematic (additional key down events when
         *          the user presses and holds a key) and break sequences (events that tell you when
         *          a key has been released) for a given set of keys.
         *
         *  \details
         *   This method has no effect if you are not in the ps2 scan code set!
         *
         *   After calling this method, the keyboard will be disabled. call enable to fix that.
         *
         * \param specificKeys An array consisting of valid set-3 scan codes.  If it contains a
         *                     single invalid one, mayhem could ensue.
         * \param numKeys The number of keys to change
         */
        bool disableBreakAndTypematic(const byte *specificKeys, int numKeys) {
            return this->sendCommand(ps2CommandCode::disableBreakAndTypematicForSpecificKeys, specificKeys, numKeys);
        }
    };

    template<int DataPin, int ClockPin, int BufferSize, typename Diagnostics, typename OnReceivedFunctor>
    Keyboard<DataPin, ClockPin, BufferSize, Diagnostics, OnReceivedFunctor> *Keyboard<DataPin, ClockPin, BufferSize, Diagnostics, OnReceivedFunctor>::instance = nullptr;
}
