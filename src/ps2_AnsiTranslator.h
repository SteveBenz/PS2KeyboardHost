#pragma once

#include "ps2_KeyboardLeds.h"
#include "ps2_KeyboardOutput.h"

namespace ps2 {
    // This class provides a translation from PS2 incoming scancodes to Ansi.  Right now,
    //  the name "Ansi" is aspirational, as the implementation given here only works for
    //  English keyboards.
    template <typename Diagnostics = NullDiagnostics>
    class AnsiTranslator
    {
    public:
        AnsiTranslator();
        AnsiTranslator(Diagnostics &diagnostics);
        void reset();

        // Processes the given scan code from the keyboard.  If it indicates a new, ansi
        //  character has been pressed, it will return the ansi value.  Else it will return
        //  a nul character ('\0').
        char translatePs2Keycode(ps2::KeyboardOutput ps2Scan);

        inline bool isCtrlKeyDown() const { return this->isCtrlDown; }
        inline bool isShiftKeyDown() const { return this->isShiftDown; }

        inline bool setCapsLock(bool newCapsLockValue) { this->isCapsLockMode = newCapsLockValue; }
        inline bool getCapsLock() const { return this->isCapsLockMode; }
        inline bool setNumLock(bool newNumLockValue) { this->isNumLockMode = newNumLockValue; }
        inline bool getNumLock() const { return this->isNumLockMode; }

    private:
        char rawTranslate(KeyboardOutput ps2Key);
        bool isKeyAffectedByNumlock(KeyboardOutput ps2Key, char rawTranslation);

        static const char ps2ToAsciiMap[] PROGMEM;
        static const byte pauseKeySequence[] PROGMEM;

        bool isSpecial;
        bool isUnmake;
        bool isCtrlDown;
        bool isShiftDown;
        bool isCapsLockMode;
        bool isNumLockMode;
        int pauseKeySequenceIndex;
        Diagnostics *diagnostics;
    };
}

#include "ps2_AnsiTranslator.hpp"