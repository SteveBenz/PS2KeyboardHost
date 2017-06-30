#pragma once
#include <stdint.h>
#include "ps2_KeyboardLeds.h"
#include "ps2_KeyboardOutput.h"

namespace ps2 {

	// This is the default class for diagnostic information from the Ps2Keyboard class.
	// It's the class you want if want to drop diagnostic information on the floor
	// (because, say, your device works plenty reliably and there's nothing to debug
	// anymore).  If you're not in that blessed place, then you can create your own
	// class that implements all these methods and stashes the data somewhere.
	class NullDiagnostics {
	public:
		// This pretty much means there's a programming error in the code.
		void readInterruptWhileWriting() {}

		// These usually mean an interrupt got dropped - e.g. because another interrupt
		//  or critical operation was happening when the clock pin dropped.
		void packetDidNotStartWithZero() {}
		void parityError() {}
		void packetDidNotEndWithOne() {}
		void packetIncomplete() {}

		void BufferOverflow() {}

		// These usually mean an interrupt got dropped - e.g. because another interrupt
		//  or critical operation was happening when the clock pin dropped.
		void sendFrameError() {}

		// The keyboard did not understand the last message sent to it (nack or LED-set)
		void nackReceived() {}

		// A command was sent to the keyboard, but it didn't respond with an Ack
		void noAckReceived() {}

		//-------------------------------------------------------------------------------
		// Above this line are errors, below it are normal events (or possibly events that
		//  are recovery from errors).

		// Reports when a nack gets sent
		void sentNack() {}
		// Records that a successful "ACK" package was (the keyboard responds with an ACK to 
		//  an LED change message and other control messages).
		void sentAck() {}

		void sentSetLed(ps2::KeyboardLeds ledStatus) {}

		void returnedBadScanCode() {}
		void returnedScanCode(ps2::KeyboardOutput scanCode) {}

		void incorrectResponse(ps2::KeyboardOutput scanCode, ps2::KeyboardOutput expectedScanCode) {}
		void noResponse(ps2::KeyboardOutput expectedScanCode) {}

		// This is used by Ps2Keyboard.begin() - you only need to provide this if you call that
		//  interface.  Note that this implementation returns something bogus, which is only okay
		//  because all of the implementations are empty, and get optimized away by the compiler.
		static NullDiagnostics *defaultInstance() { return (NullDiagnostics *)nullptr; }
	};
}