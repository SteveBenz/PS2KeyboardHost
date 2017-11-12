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
     *                    interrupts and the time they're consumed by calls to \ref readScanCode.
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
     *  With all things Arduino, you should understand the performance.  \ref readScanCode takes
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
     */
    template<int DataPin, int ClockPin, int BufferSize = 16, typename Diagnostics = NullDiagnostics>
    class Keyboard {
        static const uint8_t immediateResponseTimeInMilliseconds = 10;

        Diagnostics *diagnostics;

        // These are not marked as volatile because they are only modified in the interrupt
        // handler and at startup (before the interrupt handler is enabled).
        uint8_t ioByte = 0;
        uint8_t bitCounter = 0;
        uint32_t failureTimeMicroseconds = 0;
        uint32_t lastReadInterruptMicroseconds = 0;
        Parity parity = Parity::even;
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
            // TODO: Perhaps there should be code here that detects if bitCounter > 0 && the time since
            //  the last read interrupt is > a millisecond and, if so, log that and reset
            //  bitCounter.
            //
            //  It doesn't really need doing principally because the error-recovery code is already
            //  going to catch this (even if it doesn't do it very well).  If you have other interrupts
            //  in your system and they happen with concurrently with the keyboard, I think it's unlikely
            //  you'll ever be able to craft a foolproof system.  That since the "Ack" request only
            //  gets the previous byte.  What we'd really need to recover would be something that asks
            //  the keyboard for all keys currently pressed, and there's no such thing.
            //
            //  But perhaps I'm wrong and it's not as bad as all that.

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

        /** \brief  Waits for a byte to be sent from the keyboard for a limited amount of time.
         *          It peeks from the buffer, so a subsequent call to inputbuffer.pop will be
         *          necessary to prevent a future call to the readScanCode method from picking it up.
         *  \returns It returns the value it read or none if nothing showed up in the time alotted
         *           or garbled if there was a communcation error in that time.
         */
        KeyboardOutput expectResponse(uint16_t timeoutInMilliseconds = immediateResponseTimeInMilliseconds)
        {
            unsigned long startMilliseconds = millis();
            unsigned long stopMilliseconds = startMilliseconds + timeoutInMilliseconds;
            unsigned long nowMilliseconds;
            KeyboardOutput actualResponse;
            do
            {
                actualResponse = this->inputBuffer.peek();
                if (actualResponse == KeyboardOutput::none && this->receivedHasFramingError) {
                    actualResponse = KeyboardOutput::garbled;
                    // Note that clearing this is intended to prevent future polling from trying to get the
                    //  bad character re-sent, but it might not work, depending on the nature of the error.
                    //  Future interrupts could set it again.
                    this->receivedHasFramingError = false;
                }
                nowMilliseconds = millis();
            } while (actualResponse == KeyboardOutput::none && (nowMilliseconds < stopMilliseconds || (stopMilliseconds < startMilliseconds && startMilliseconds <= nowMilliseconds)));

            if (actualResponse == KeyboardOutput::none) {
                this->diagnostics->noResponse(KeyboardOutput::none);
            }
            return actualResponse;
        }

        /** \brief Waits a limited amount of time for a specific byte to be written by the keyboard.
         *         If it appears, this method returns true.  If a different keycode appears, it will return
         *         false and leave that byte on the input queue.
         */
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
                this->inputBuffer.pop();
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
            this->diagnostics->sentByte((byte)ps2CommandCode::resend);
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

        /** \brief After the keyboard gets power, the PS2 keyboard sends a code that indicates successful
         *    or unsuccessful startup.  This waits for that to happen.
         *  \param timeoutInMillis The number of milliseconds to wait for the keyboard to send its
         *             success/fail message.
         *  \details
         *    All this function does is wait for a proscribed amount of time (750ms, as suggested by
         *    the documentation), for a batSuccessful code.  That's straightforward.  The thing to be
         *    careful of is this - if you call this function in your begin() and then upload the program
         *    to your Arduino, all this is going to do is delay for 750ms and then throw a diagnostic
         *    code - that's because the keyboard has power throughout the event and so doesn't power-on.
         *
         *    But once you get your device into the field, you really can count on the proper behavior
         *    (well, unless you have, say, a reset button that resets the Arduino).
         *
         *    There are two reasonable ways to deal with this - first, if you need to set up the keyboard,
         *    e.g. set up a scan code set and all that, you need to call this method before you get up to
         *    your shennanigans - the keyboard just won't respond until it's done booting.  If that's you,
         *    and you're keen on having Diagnostic data, you can call \ref SimpleDiagnostic::reset right
         *    after this to clean it up.  If you're really keen on diagnostics, then you'll set up a
         *    "RELEASE" preprocessor symbol and skip the reset when you compile in that mode.
         *
         *    If you don't actually need to do any setup, then you can just skip calling this method.  Yes,
         *    the keyboard will send that batSuccessful (hopefully) signal later on, but \ref readScanCode
         *    specifically looks for that code and drops it on the floor when it arrives.
         */
        bool awaitStartup(uint16_t timeoutInMillis = 750) {
            return this->expectResponse(KeyboardOutput::batSuccessful, timeoutInMillis);
        }

        /** \brief
         *   Returns the last code sent by the keyboard.
         *  \details
         *   You should call this function frequently, in your loop implementation most likely.  The
         *   more often you call it, the more responsive your device will be.  If you cannot call it
         *   frequently, make sure you have an appropriately-sized buffer (BufferSize).
         *
         *   If there is nothing to read, this method will return 'none'.
         *
         *   It can also return 'garbled' if there's been a framing error.  A retry will be attempted,
         *   but that's far from a sure-thing.  If you get this result, it likely indicates a collision
         *   with one of your interrupt handlers.  Look into reducing the amount of processing you do
         *   in your interrupt handlers.
         *
         *   In any case, if you see a 'garbled' result, you might be losing some keystrokes, so do
         *   what you can to make sure that it doesn't impact the device too badly.
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
                //
                // In hindsight, I think likely the most effective way to do error detection would
                //  be to wait for a certain amount of time after the last read interrupt when a
                //  failure has previously been detected.  For that to work, then when the keyboard
                //  sends a multi-byte sequence, there'd have to be a pause between one byte and
                //  the next; I haven't validated that such a pause actually exists, however.
                //
                // The other concern is how to ensure that we can get Arduino-time during that
                //  window; it'll be hard to guarantee that in a general purpose library.
                if (failureTimeMicroseconds + 200 > micros()) {
                    return KeyboardOutput::none;
                }
                if (this->bitCounter > 3) {
                    this->sendNack();
                }
                else {
                    this->diagnostics->clockLineGlitch(this->bitCounter);
                    this->receivedHasFramingError = false;
                    this->bitCounter = 0;
                    this->ioByte = 0;
                    this->parity = Parity::even;
                }
                return KeyboardOutput::garbled;
            }
            else if (code == KeyboardOutput::batSuccessful) {
                // The keyboard will send either batSuccessful or batFailure on startup.
                //   The way this class is structured, we can't really be sure that begin()
                //   will be called immediately after power-up, nor can we be sure that the
                //   Arduino wasn't just reset (while the keyboard had power the whole time).
                //   So we can't write code in begin() that just waits for the message.
                //   We could have a flag that is true until the first action is taken
                //   with the keyboard, but that seems needlessly wasteful.
                code = this->inputBuffer.pop();
            }
            else if (code == KeyboardOutput::batFailure) {
                diagnostics->startupFailure();
                code = this->inputBuffer.pop();
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
         *  \param timeoutInMillis The number of milliseconds to wait for the keyboard to send its
         *             success/fail message.
         *  \details
         *   This can take up to a second to complete.  (Because of the Protocol spec).
         *  \returns Returns true if the keyboard responded appropriately and false otherwise.
         */
        bool reset(uint16_t timeoutInMillis = 1000) {
            this->inputBuffer.clear();
            this->sendCommand(ps2CommandCode::reset);
            return this->expectResponse(KeyboardOutput::batSuccessful, timeoutInMillis);
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
            this->inputBuffer.pop();
            uint16_t id = ((uint8_t)response) << 8;
            response = this->expectResponse();
            if (response == KeyboardOutput::none) {
                return 0xffff;
            }
            this->inputBuffer.pop();
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
                this->inputBuffer.pop();
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

    template<int DataPin, int ClockPin, int BufferSize, typename Diagnostics>
    Keyboard<DataPin, ClockPin, BufferSize, Diagnostics> *Keyboard<DataPin, ClockPin, BufferSize, Diagnostics>::instance = nullptr;
}
