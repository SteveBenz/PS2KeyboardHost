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

// These translation tables were distilled from here:
//  http://www.hiemalis.org/~keiji/PC/scancode-translate.pdf
// They could certainly be ProgMem, but it's unlikely to matter to anybody,
// as if you're implementing a PS2->USB conversion on your Arduino, it's
// unlikely you'll be doing anything else that requires a ton of space,
// so we'll keep it simple.

/*PROGMEM*/ const byte ps2ToUsbMap[] = {
    0x00, // [00] unused
    0x42, // [01] F9
    0x00, // [02] unused
    0x3e, // [03] F5
    0x3c, // [04] F3
    0x3a, // [05] F1
    0x3b, // [06] F2
    0x45, // [07] F12
    0x68, // [08] F13
    0x43, // [09] F10
    0x41, // [0a] F8
    0x3f, // [0b] F6
    0x3d, // [0c] F4
    0x2b, // [0d] Tab
    0x35, // [0e] ` ~
    0x67, // [0f] Keypad =
    0x69, // [10] F14
    0xe2, // [11] Left Alt
    0xe1, // [12] Left Shift
    0x00, // [13] unused
    0xe0, // [14] Left Control
    0x14, // [15] q Q
    0x1e, // [16] 1 !
    0x00, // [17] unused
    0x6a, // [18] F15
    0x00, // [19] unused
    0x1d, // [1a] z Z
    0x16, // [1b] s S
    0x04, // [1c] a A
    0x1a, // [1d] w W
    0x1f, // [1e] 2 @
    0x00, // [1f] unused
    0x6b, // [20] F16
    0x06, // [21] c C
    0x1b, // [22] x X
    0x07, // [23] d D
    0x08, // [24] e E
    0x21, // [25] 4 $
    0x20, // [26] 3 #
    0x00, // [27] unused
    0x6c, // [28] F17
    0x2c, // [29] Space
    0x19, // [2a] v V
    0x09, // [2b] f F
    0x17, // [2c] t T
    0x15, // [2d] r R
    0x22, // [2e] 5 %
    0x00, // [2f] unused
    0x6d, // [30] F18
    0x11, // [31] n N
    0x05, // [32] b B
    0x0b, // [33] h H
    0x0a, // [34] g G
    0x1c, // [35] y Y
    0x23, // [36] 6 ^
    0x00, // [37] unused
    0x6e, // [38] F19
    0x00, // [39] unused
    0x10, // [3a] m M
    0x0d, // [3b] j J
    0x18, // [3c] u U
    0x24, // [3d] 7 &
    0x25, // [3e] 8 *
    0x00, // [3f] unused
    0x6f, // [40] F20
    0x36, // [41] , <
    0x0e, // [42] k K
    0x0c, // [43] i I
    0x12, // [44] o O
    0x27, // [45] 0 )
    0x26, // [46] 9 (
    0x00, // [47] unused
    0x70, // [48] F21
    0x37, // [49] . >
    0x38, // [4a] / ?
    0x0f, // [4b] l L
    0x33, // [4c] ; :
    0x13, // [4d] p P
    0x2d, // [4e] - _
    0x00, // [4f] unused
    0x71, // [50] F22
    0x00, // [51] unused
    0x34, // [52] ' "
    0x00, // [53] unused
    0x2f, // [54] [ {
    0x2e, // [55] = +
    0x00, // [56] unused
    0x72, // [57] F23
    0x39, // [58] Caps Lock
    0xe5, // [59] Right Shift
    0x28, // [5a] Return
    0x30, // [5b] ] }
    0x00, // [5c] unused
    0x32, // [5d] Europe 1 (Note 2)
    0x00, // [5e] unused
    0x73, // [5f] F24
    0x00, // [60] unused
    0x64, // [61] Europe 2 (Note 2)
    0x00, // [62] unused
    0x00, // [63] unused
    0x00, // [64] unused
    0x00, // [65] unused
    0x2a, // [66] Backspace
    0x00, // [67] unused
    0x00, // [68] unused
    0x59, // [69] Keypad 1 End
    0x00, // [6a] unused
    0x5c, // [6b] Keypad 4 Left
    0x5f, // [6c] Keypad 7 Home
    0x00, // [6d] unused
    0x00, // [6e] unused
    0x00, // [6f] unused
    0x62, // [70] Keypad 0 Insert
    0x63, // [71] Keypad . Delete
    0x5a, // [72] Keypad 2 Down
    0x5d, // [73] Keypad 5
    0x5e, // [74] Keypad 6 Right
    0x60, // [75] Keypad 8 Up
    0x29, // [76] Escape
    0x53, // [77] Num Lock
    0x44, // [78] F11
    0x57, // [79] Keypad +
    0x5b, // [7a] Keypad 3 PageDn
    0x56, // [7b] Keypad -
    0x55, // [7c] Keypad *
    0x61, // [7d] Keypad 9 PageUp
    0x47, // [7e] Scroll Lock   <-- HAND EDIT!
    0x00, // [7f] unused
    0x00, // [80] unused
    0x00, // [81] unused
    0x00, // [82] unused
    0x40, // [83] F7
};

// This would probably be tighter as a switch statement.
/*PROGMEM*/ const byte extPs2ToUsbMap[] = {
    0x00, // [00] unused
    0x00, // [01] unused
    0x00, // [02] unused
    0x00, // [03] unused
    0x00, // [04] unused
    0x00, // [05] unused
    0x00, // [06] unused
    0x00, // [07] unused
    0x00, // [08] unused
    0x00, // [09] unused
    0x00, // [0a] unused
    0x00, // [0b] unused
    0x00, // [0c] unused
    0x00, // [0d] unused
    0x00, // [0e] unused
    0x00, // [0f] unused
    0x00, // [10] unused
    0xe6, // [11] Right Alt
    0x00, // [12] unused
    0x00, // [13] unused
    0xe4, // [14] Right Control
    0x00, // [15] unused
    0x00, // [16] unused
    0x00, // [17] unused
    0x00, // [18] unused
    0x00, // [19] unused
    0x00, // [1a] unused
    0x00, // [1b] unused
    0x00, // [1c] unused
    0x00, // [1d] unused
    0x00, // [1e] unused
    0xe3, // [1f] Left GUI
    0x00, // [20] unused
    0x00, // [21] unused
    0x00, // [22] unused
    0x00, // [23] unused
    0x00, // [24] unused
    0x00, // [25] unused
    0x00, // [26] unused
    0xe7, // [27] Right GUI
    0x00, // [28] unused
    0x00, // [29] unused
    0x00, // [2a] unused
    0x00, // [2b] unused
    0x00, // [2c] unused
    0x00, // [2d] unused
    0x00, // [2e] unused
    0x65, // [2f] Menu Key
    0x00, // [30] unused
    0x00, // [31] unused
    0x00, // [32] unused
    0x00, // [33] unused
    0x00, // [34] unused
    0x00, // [35] unused
    0x00, // [36] unused
    0x00, // [37] unused
    0x00, // [38] unused
    0x00, // [39] unused
    0x00, // [3a] unused
    0x00, // [3b] unused
    0x00, // [3c] unused
    0x00, // [3d] unused
    0x00, // [3e] unused
    0x00, // [3f] unused
    0x00, // [40] unused
    0x00, // [41] unused
    0x00, // [42] unused
    0x00, // [43] unused
    0x00, // [44] unused
    0x00, // [45] unused
    0x00, // [46] unused
    0x00, // [47] unused
    0x00, // [48] unused
    0x00, // [49] unused
    0x54, // [4a] Keypad / (Note 1)
    0x00, // [4b] unused
    0x00, // [4c] unused
    0x00, // [4d] unused
    0x00, // [4e] unused
    0x00, // [4f] unused
    0x00, // [50] unused
    0x00, // [51] unused
    0x00, // [52] unused
    0x00, // [53] unused
    0x00, // [54] unused
    0x00, // [55] unused
    0x00, // [56] unused
    0x00, // [57] unused
    0x00, // [58] unused
    0x00, // [59] unused
    0x58, // [5a] Keypad Enter
    0x00, // [5b] unused
    0x00, // [5c] unused
    0x00, // [5d] unused
    0x00, // [5e] unused
    0x00, // [5f] unused
    0x00, // [60] unused
    0x00, // [61] unused
    0x00, // [62] unused
    0x00, // [63] unused
    0x00, // [64] unused
    0x00, // [65] unused
    0x00, // [66] unused
    0x00, // [67] unused
    0x00, // [68] unused
    0x4d, // [69] End (Note 1)
    0x00, // [6a] unused
    0x50, // [6b] Left Arrow (Note 1)
    0x4a, // [6c] Home (Note 1)
    0x00, // [6d] unused
    0x00, // [6e] unused
    0x00, // [6f] unused
    0x49, // [70] Insert (Note 1)
    0x4c, // [71] Delete (Note 1)
    0x51, // [72] Down Arrow (Note 1)
    0x00, // [73] unused
    0x4f, // [74] Right Arrow (Note 1)
    0x52, // [75] Up Arrow (Note 1)
    0x00, // [76] unused
    0x00, // [77] unused
    0x00, // [78] unused
    0x00, // [79] unused
    0x4e, // [7a] Page Down (Note 1)
    0x00, // [7b] unused
    0x46, // [7c] Print Screen
    0x4b, // [7d] Page Up (Note 1)
    0x48, // [7e] Pause (when ctrl is down)
    0x00, // [7f] unused
};

static const byte pauseKeySequence[] {
    0xe1, 0x14, 0x77
    //0xe1, 0xfc, 0x00, 0x04, 0x14, 0x77
};

template <typename Diagnostics>
ps2::UsbTranslator<Diagnostics>::UsbTranslator(Diagnostics &diagnostics)
{
    this->isSpecial = false;
    this->isUnmake = false;
    this->pauseKeySequenceIndex = 0;
    this->diagnostics = &diagnostics;
}

template <typename Diagnostics>
void ps2::UsbTranslator<Diagnostics>::reset() {
    isSpecial = false;
    isUnmake = false;
}

template <typename Diagnostics>
ps2::KeyboardLeds ps2::UsbTranslator<Diagnostics>::translateLeds(UsbKeyboardLeds usbLeds)
{
    return (((int)usbLeds & (int)ps2::UsbKeyboardLeds::capsLock) ? ps2::KeyboardLeds::capsLock : ps2::KeyboardLeds::none)
         | (((int)usbLeds & (int)ps2::UsbKeyboardLeds::numLock) ? ps2::KeyboardLeds::numLock : ps2::KeyboardLeds::none)
         | (((int)usbLeds & (int)ps2::UsbKeyboardLeds::scrollLock) ? ps2::KeyboardLeds::scrollLock : ps2::KeyboardLeds::none);
}

template <typename Diagnostics>
ps2::UsbKeyAction ps2::UsbTranslator<Diagnostics>::translatePs2Keycode(ps2::KeyboardOutput ps2Scan)
{
    ps2::UsbKeyAction action;
    action.hidCode = 0;
    action.gesture = ps2::UsbKeyAction::None;

    if (ps2Scan == ps2::KeyboardOutput::unmake)
    {
        this->isUnmake = true;
        return action;
    }

    if (ps2Scan == ps2::KeyboardOutput::extend)
    {
        this->isSpecial = true;
        return action;
    }

    byte usbCode = 0;
    if ((uint8_t)ps2Scan == pauseKeySequence[pauseKeySequenceIndex]) {
        ++pauseKeySequenceIndex;
        if (pauseKeySequenceIndex < sizeof(pauseKeySequence))
            return action;

        usbCode = 0x48; // The Pause key identifier in USB/HID
    }
    else if (this->isSpecial)
    {
        usbCode = (uint8_t)ps2Scan < sizeof(extPs2ToUsbMap) ? extPs2ToUsbMap[(uint8_t)ps2Scan] : 0;
    }
    else
    {
        usbCode = (uint8_t)ps2Scan < sizeof(ps2ToUsbMap) ? ps2ToUsbMap[(uint8_t)ps2Scan] : 0;
    }
    pauseKeySequenceIndex = 0;

    if (usbCode == 0)
    {
        diagnostics->noTranslationForKey(this->isSpecial, ps2Scan);
        this->isUnmake = false;
        this->isSpecial = false;
        return action;
    }

    action.hidCode = usbCode;
    action.gesture = this->isUnmake ? ps2::UsbKeyAction::KeyUp : ps2::UsbKeyAction::KeyDown;

    this->isUnmake = false;
    this->isSpecial = false;

    return action;
}
