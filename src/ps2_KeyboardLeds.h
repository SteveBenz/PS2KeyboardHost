#pragma once

namespace ps2 {
    /** \brief The LED's available on a standard PS2 keyboard */
    enum class KeyboardLeds {
        // the legends tell of a 4th LED that some keyboards have
        capsLock = 0x4,
        numLock = 0x2,
        scrollLock = 0x1,

        all = 0x7,
        none = 0x0,
    };
    inline KeyboardLeds operator|(KeyboardLeds a, KeyboardLeds b)
    {
        return static_cast<KeyboardLeds>(static_cast<int>(a) | static_cast<int>(b));
    }
}