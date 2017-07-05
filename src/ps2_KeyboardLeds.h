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