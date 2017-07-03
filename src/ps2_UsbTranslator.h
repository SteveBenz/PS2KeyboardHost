#pragma once

#include "ps2_KeyboardLeds.h"
#include "ps2_KeyboardOutput.h"

namespace ps2 {

    enum class UsbKeyboardLeds {
        none = 0x0,
        numLock = 0x1,
        capsLock = 0x2,
        scrollLock = 0x4,
        all = 0x07,
    };

    struct UsbKeyAction {
        uint8_t hidCode;
        enum {
            KeyUp,
            KeyDown,
            None
        } gesture;
    };

    // Translates from PS2's default scancode set to USB/HID
    template <typename Diagnostics = NullDiagnostics>
    class UsbTranslator
    {
    public:
        UsbTranslator();
        UsbTranslator(Diagnostics &diagnostics);
        void reset();
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