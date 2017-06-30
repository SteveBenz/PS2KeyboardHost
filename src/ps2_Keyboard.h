/*
Keyboard.h - PS2 Keyboard library
Copyright (c) 2007 Free Software Foundation.  All right reserved.
Written by Christian Weichel <info@32leaves.net>

** Mostly rewritten Paul Stoffregen <paul@pjrc.com>, June 2010
** Modified for use with Arduino 13 by L. Abraham Smith, <n3bah@microcompdesign.com> *
** Modified for easy interrup pin assignement on method begin(datapin,irq_pin). Cuningan <cuninganreset@gmail.com> **

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
	 *  Implements the PS2 keyboard protocol.  To use this class, you need to set up an instance
	 *  of it, with the data and clock pins supplied as template arguments, and then you need to
	 *  call begin to start the protocol.  After that, you need to frequently poll the readScanCode
	 *  method to read the stream of keypresses.
	 *
	 *  The other methods allow you to set up the keyboard in a variety of ways.  Be aware that while
	 *  the readScanCode interface is very fast (in the microseconds), the other commands can take
	 *  much longer (usually a few milliseconds).  All of the keyboard setup methods support a bool that
	 *  reports success or not, but you really shouldn't burden your program with error recovery for that.
	 *  First, if you're building the idea of being able to plug your keyboard in & out, that's a bad
	 *  idea, as PS2 keyboards can fry themselves if you hot-swap them.  The biggest source of legitimate
	 *  error I know of is long-running interrupts which cause the PS2 clock interrupt to be skipped.
	 *  Having short-lived interrupt routines is one of the ways to having a happy life in Arduino-land,
	 *  but if you can't help that then do all of the PS2 keyboard setup before enabling interrupts for
	 *  your other devices.
	 *
	 *  With an Arduino, the less time you spend in your interrupt callbacks the better.  That's because
	 *  when one interrupt is being processed, another is being blocked.  As a rule, keyboard input should
	 *  not be treated as a real-time problem (that is, a problem requiring sub-millisecond responses)
	 *  because the human that's typing on it is incapable of that kind of precision.  So the preferred
	 *  way to deal with this class is polling - that is, you should have a normal-mode task that just
	 *  pings the readScanCode method.  If you're going to be polling it very frequently (multiple times
	 *  per millisecond), then you can go with a small buffer size (1 being the minimum).  Each full
	 *  keystroke, including data for both up and down, takes 3, sometimes 5, and in a few rare cases
	 *  more.  The default of 16 is going to be overkill for most applications, but if you're not
	 *  pushing against your RAM limit, why chance it?
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
			// instruction count is lower.
			uint8_t dataPinValue = (*portInputRegister(digitalPinToPort(DataPin)) & digitalPinToBitMask(DataPin)) ? 1 : 0; // ==digitalRead(DataPin);

			lastReadInterruptMicroseconds = micros();

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

		KeyboardOutput expectResponse(uint16_t timeoutInMilliseconds = immediateResponseTimeInMilliseconds) {
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

		bool sendCommand(ps2CommandCode command)
		{
			return this->sendData((byte)command);
		}

		bool sendCommand(ps2CommandCode command, byte data)
		{
			return this->sendCommand(command)
				&& this->sendData(data);
		}

		bool sendCommand(ps2CommandCode command, const byte *data, int numBytes)
		{
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

		bool sendData(byte data)
		{
			this->sendByte(data);
			bool isOk = this->expectAck();
			if (!isOk)
			{
				this->enableReadInterrupts();
			}
			return isOk;
		}

		void sendNack() {
			this->diagnostics->sentNack();
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
		 *  (several times per millisecond) as there is no buffering.
		 */
		KeyboardOutput readScanCode() {
			KeyboardOutput code = this->inputBuffer.pop();

			unsigned long nowMicroseconds = micros();
			if (code == KeyboardOutput::none && this->receivedHasFramingError)
			{
				// NACK's affect what the device perceives as the last character sent, so if we
				//  interrupt immediately, it might decide to send the last scan code (which we
				//  already have) again.  The clock is between 17KHz and 10KHz, so the fastest
				//  send time for the 12 bits is going to be around 12*1000000/17000 700us and the
				//  slowest would be 1200us.  Again, that's the full cycle.  Most of the errors
				//  are going to be found at the parity & stop bit, so we'll wait 200us, as a guess,
				//  before resending;
				if (failureTimeMicroseconds + 200 > nowMicroseconds) {
					return KeyboardOutput::none;
				}
				this->sendNack();
				this->diagnostics->returnedBadScanCode();
				return KeyboardOutput::garbled;
			}

			if (code != KeyboardOutput::none)
			{
				this->diagnostics->returnedScanCode(code);
			}
			return code;
		}

		bool sendLedStatus(KeyboardLeds ledStatus)
		{
			return sendCommand(ps2CommandCode::setLeds, (byte)ledStatus);
		}

		// Resets the keyboard and returns true if the keyboard appears well, false otherwise.
		// This can take up to a second to complete.
		bool reset() {
			this->inputBuffer.clear();
			this->sendCommand(ps2CommandCode::reset);

			KeyboardOutput result = this->expectResponse(1000);
			return result == KeyboardOutput::batSuccessful;
		}

		// Returns the device ID returned by the keyboard - according to the documentation, this
		//  will always be 0xab83.  In the event of an error, this will return 0xffff.
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

		bool setScanCodeSet(ScanCodeSet newScanCodeSet) {
			return this->sendCommand(ps2CommandCode::setScanCodeSet, (byte)newScanCodeSet);
		}

		// Sends the "Echo" command to the keyboard, which should send an "Echo" in return.
		// This can be used to verifies that a keyboard is connected and working properly.
		// Returns true if the keyboard responded appropriately and false otherwise.
		// This method may take several milliseconds to complete.
		bool echo() {
			this->sendByte((byte)ps2CommandCode::echo);
			return this->expectResponse(KeyboardOutput::echo);
		}

		bool setTypematicRateAndDelay(TypematicRate rate, TypematicStartDelay startDelay) {
			byte combined = (byte)rate | (((byte)startDelay) << 4);
			return this->sendCommand(ps2CommandCode::setTypematicRate, combined);
		}

		// Restores scan code set, typematic rate, and typematic delay.
		bool resetToDefaults()
		{
			return this->sendCommand(ps2CommandCode::useDefaultSettings);
		}

		bool enable() { return this->sendCommand(ps2CommandCode::enable); }
		bool disable() { return this->sendCommand(ps2CommandCode::disable); }

		// Instructs the keyboard to resume sending "break" codes (see enableBreakCodes).
		// This method has no effect if you are not in the ps2 scan code set!
		bool enableBreakAndTypematic() {
			return this->sendCommand(ps2CommandCode::enableBreakAndTypeMaticForAllKeys);
		}

		// Instructs the keyboard to resume sending "break" codes (see enableBreakCodes).
		// This method has no effect if you are not in the ps2 scan code set!
		bool disableBreakCodes() {
			return this->sendCommand(ps2CommandCode::disableBreaksForAllKeys);
		}

		// Instructs the keyboard to stop sending "break" codes for some keys (see enableBreakCodes).
		// This method has no effect if you are not in the ps2 scan code set!
		// SpecificKeys should be an array consisting of valid set-3 scan codes.  If it contains a
		// single invalid one, mayhem could ensue.
		//
		// note that after calling this method, the keyboard will be disabled. call enable to fix that.
		bool disableBreakCodes(const byte *specificKeys, int numKeys) {
			return this->sendCommand(ps2CommandCode::disableBreaksForSpecificKeys, specificKeys, numKeys);
		}

		// Instructs the keyboard not to do typematic repeats
		// This method has no effect if you are not in the ps2 scan code set!
		bool disableTypematic() {
			return this->sendCommand(ps2CommandCode::disableTypematicForAllKeys);
		}

		// Instructs the keyboard not to do typematic repeats
		// This method has no effect if you are not in the ps2 scan code set!
		bool disableBreakAndTypematic() {
			return this->sendCommand(ps2CommandCode::disableBreakAndTypematicForAllKeys);
		}

		// Instructs the keyboard to stop sending "break" codes for some keys (see enableBreakCodes).
		// This method has no effect if you are not in the ps2 scan code set!
		// SpecificKeys should be an array consisting of valid set-3 scan codes.  If it contains a
		// single invalid one, mayhem could ensue.
		//
		// note that after calling this method, the keyboard will be disabled. call enable to fix that.
		bool disableTypematic(const byte *specificKeys, int numKeys) {
			return this->sendCommand(ps2CommandCode::disableTypematicForSpecificKeys, specificKeys, numKeys);
		}

		// Instructs the keyboard to stop sending "break" codes for some keys (see enableBreakCodes).
		// This method has no effect if you are not in the ps2 scan code set!
		// SpecificKeys should be an array consisting of valid set-3 scan codes.  If it contains a
		// single invalid one, mayhem could ensue.
		//
		// note that after calling this method, the keyboard will be disabled. call enable to fix that.
		bool disableBreakAndTypematic(const byte *specificKeys, int numKeys) {
			return this->sendCommand(ps2CommandCode::disableBreakAndTypematicForSpecificKeys, specificKeys, numKeys);
		}
	};

	template<int DataPin, int ClockPin, int BufferSize, typename Diagnostics, typename OnReceivedFunctor>
	Keyboard<DataPin, ClockPin, BufferSize, Diagnostics, OnReceivedFunctor> *Keyboard<DataPin, ClockPin, BufferSize, Diagnostics, OnReceivedFunctor>::instance = nullptr;
}
