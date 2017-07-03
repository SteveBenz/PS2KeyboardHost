#pragma once
#include <stdint.h>
#include "ps2_KeyboardLeds.h"
#include "ps2_KeyboardOutput.h"

namespace ps2 {

    // This is the default class for diagnostic information for all the classes in this library.
    // It's the class you want if want to drop diagnostic information on the floor
    // (because, say, your device works plenty reliably and there's nothing to debug
    // anymore).  If you're not in that blessed place, then you can create your own
    // class that implements all these methods and stashes the data somewhere.
    class NullDiagnostics {
    public:
        // These usually mean an interrupt got dropped - e.g. because another interrupt
        //  or critical operation was happening when the clock pin dropped.
        void packetDidNotStartWithZero() {}
        void parityError() {}
        void packetDidNotEndWithOne() {}
        void packetIncomplete() {}
        void sendFrameError() {}

        // Polling frequency isn't high enough or the buffer is too small.
        void bufferOverflow() {}

        // This would probably mean a bug in the implementation of the protocol.
        void incorrectResponse(ps2::KeyboardOutput scanCode, ps2::KeyboardOutput expectedScanCode) {}
        void noResponse(ps2::KeyboardOutput expectedScanCode) {}

        // Translator errors
        void noTranslationForKey(bool isExtended, KeyboardOutput code) {}

        //-------------------------------------------------------------------------------
        // Above this line are errors, below it are normal events (or possibly events that
        //  are recovery from errors).

        void sentByte(byte b) {}
        void receivedByte(byte b) {}

        // This is used by Ps2Keyboard.begin() - you only need to provide this if you call that
        //  interface.  Note that this implementation returns something bogus, which is only okay
        //  because all of the implementations are empty, and get optimized away by the compiler.
        static NullDiagnostics *defaultInstance() { return (NullDiagnostics *)nullptr; }
    };
}