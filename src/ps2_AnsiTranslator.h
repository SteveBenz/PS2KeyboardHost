#pragma once

#include "ps2_KeyboardLeds.h"
#include "ps2_KeyboardOutput.h"

namespace ps2 {
    /** \brief
     *   This class provides a translation from PS2 incoming scancodes to Ansi.  Right now,
     *   the name "Ansi" is aspirational, as the implementation given here only works for
     *   English keyboards.
     *
     *  \details
     *   This will translate shift keys, caps lock, num lock and the ctrl key.  E.g. if the
     *   user types "Ctrl+G", \ref translatePs2Keycode will return Ascii 7.  If the caps lock
     *   key has been pressed before and the user types "g", it will return 'G'.  If the user
     *   types shift+H under these circumstances, it will return 'h'.
     *
     * \tparam Diagnostics A sink for debugging information.
     */
    template <typename Diagnostics = NullDiagnostics>
    class AnsiTranslator
    {
    public:
        AnsiTranslator();
        AnsiTranslator(Diagnostics &diagnostics);
        void reset();

        /** \brief Processes the given scan code from the keyboard.  It only gives you keydown
         *          events for keys that have an ansi translation (e.g. the "g" key has an effect,
         *         the "Home" key does not.)
         *  \returns
         *   If it indicates a new, ansi character has been pressed, it will return the ansi value, otherwise
         *   it will return a nul character ('\0').
         */
        char translatePs2Keycode(ps2::KeyboardOutput ps2Scan);

        /** \brief Gets the state of the Ctrl key.
         *  \returns True if the key was pressed down as of the last call to \translatePs2Keycode.
         */
        inline bool isCtrlKeyDown() const { return this->isCtrlDown; }

        /** \brief Gets the state of the Shift key.
         *  \returns True if the key was pressed down as of the last call to \translatePs2Keycode.
         */
        inline bool isShiftKeyDown() const { return this->isShiftDown; }

        /** \brief Sets the state of the caps lock mode.
         *  \details Note that this has no effect on the PS2 keyboard or the PS2 keyboard LED.
         *           It just effects how keypresses are translated.
         */
        inline void setCapsLock(bool newCapsLockValue) { this->isCapsLockMode = newCapsLockValue; }

        /** \brief Gets the state of the caps lock mode. */
        inline bool getCapsLock() const { return this->isCapsLockMode; }

        /** \brief Sets the state of the num lock mode.
         *  \details Note that this has no effect on the PS2 keyboard or the PS2 keyboard LED.
         *           It just effects how keypresses are translated.
         */
        inline void setNumLock(bool newNumLockValue) { this->isNumLockMode = newNumLockValue; }

        /** \brief Gets the state of the num lock mode. */
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