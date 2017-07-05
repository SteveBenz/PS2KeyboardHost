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
#include "ps2_KeyboardLeds.h"
#include "ps2_KeyboardOutput.h"

namespace ps2 {

    /** \brief
     *  This is the default class for diagnostic information for all the classes in this library.
     *  It's the class you want if want to drop diagnostic information on the floor
     *  (because, say, your device works plenty reliably and there's nothing to debug
     *  anymore).  If you're not in that blessed place, then you can create your own
     *  class that implements all these methods and stashes the data somewhere.
     */
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