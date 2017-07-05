#pragma once

#include "ps2_KeyboardLeds.h"
#include "ps2_KeyboardOutput.h"

namespace ps2 {

    /** \brief The bitfield that describes USB LED's. */
    enum class UsbKeyboardLeds {
        none = 0x0,
        numLock = 0x1,
        capsLock = 0x2,
        scrollLock = 0x4,
        all = 0x07,
    };

    /** \brief A translated PS2 keystroke - it indicates either a keydown, a key up, or that there should be no immediate action. */
    struct UsbKeyAction {
        uint8_t hidCode;
        enum {
            KeyUp,
            KeyDown,
            None
        } gesture;
    };

    /** \brief Translates from PS2's default scancode set to USB/HID.
     *
     * \details
     *  The translation was mostly culled from http://www.hiemalis.org/~keiji/PC/scancode-translate.pdf
     */
    template <typename Diagnostics = NullDiagnostics>
    class UsbTranslator
    {
    public:
        UsbTranslator();
        UsbTranslator(Diagnostics &diagnostics);

        /**  \brief causes it to forget about any scan codes that it has recorded so far and start fresh.
         */
        void reset();

        /** \brief Examines a scan code (from the default PS2 scan code set) and translates it into the
         *  corresponding USB/HID scan code.
         *
         * \returns A gesture to send to the USB Keyboard - either a key up, key down, or no-action.
         *   (No action would happen if the scan code was found to be a part of a multi-byte sequence
         *   from the PS2.)
         */
        UsbKeyAction translatePs2Keycode(ps2::KeyboardOutput ps2Scan);
        KeyboardLeds translateLeds(UsbKeyboardLeds usbLeds);

    private:
        bool isSpecial;
        bool isUnmake;
        int pauseKeySequenceIndex;
        Diagnostics *diagnostics;
    };
}

#include "ps2_UsbTranslator.hpp"