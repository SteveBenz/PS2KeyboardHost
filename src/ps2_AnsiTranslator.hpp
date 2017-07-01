#pragma once
#include <Arduino.h>

namespace ps2 {
	// For reference: http://www.computer-engineering.org/ps2keyboard/scancodes2.html
	const char AnsiTranslator::ps2ToAsciiMap[] PROGMEM = {
		'\t', // [0d] Tab
		'`',  // [0e] ` ~
		'=',  // [0f] Keypad =
		'\0', // [10] F14
		'\0', // [11] Left Alt
		'\0', // [12] Left Shift
		'\0', // [13] unused
		'\0', // [14] Left Control
		'q',  // [15] q Q
		'1',  // [16] 1 !
		'\0', // [17] unused
		'\0', // [18] F15
		'\0', // [19] unused
		'z',  // [1a] z Z
		's',  // [1b] s S
		'a',  // [1c] a A
		'w',  // [1d] w W
		'2',  // [1e] 2 @
		'\0', // [1f] unused
		'\0', // [20] F16
		'c',  // [21] c C
		'x',  // [22] x X
		'd',  // [23] d D
		'e',  // [24] e E
		'4',  // [25] 4 $
		'3',  // [26] 3 #
		'\0', // [27] unused
		'\0', // [28] F17
		' ',  // [29] Space
		'v',  // [2a] v V
		'f',  // [2b] f F
		't',  // [2c] t T
		'r',  // [2d] r R
		'5',  // [2e] 5 %
		'\0', // [2f] unused
		'\0', // [30] F18
		'n',  // [31] n N
		'b',  // [32] b B
		'h',  // [33] h H
		'g',  // [34] g G
		'y',  // [35] y Y
		'6',  // [36] 6 ^
		'\0', // [37] unused
		'\0', // [38] F19
		'\0', // [39] unused
		'm',  // [3a] m M
		'j',  // [3b] j J
		'u',  // [3c] u U
		'7',  // [3d] 7 &
		'8',  // [3e] 8 *
		'\0', // [3f] unused
		'\0', // [40] F20
		',',  // [41] , <
		'k',  // [42] k K
		'i',  // [43] i I
		'o',  // [44] o O
		'0',  // [45] 0 )
		'9',  // [46] 9 (
		'\0', // [47] unused
		'\0', // [48] F21
		'.',  // [49] . >
		'/',  // [4a] / ?
		'l',  // [4b] l L
		';',  // [4c] ; :
		'p',  // [4d] p P
		'-',  // [4e] - _
		'\0', // [4f] unused
		'\0', // [50] F22
		'\0', // [51] unused
		'\'', // [52] ' "
		'\0', // [53] unused
		'[',  // [54] [ {
		'=',  // [55] = +
		'\0', // [56] unused
		'\0', // [57] F23
		'\0', // [58] Caps Lock
		'\0', // [59] Right Shift
		'\r', // [5a] Return
		']',  // [5b] ] }
		'\0', // [5c] unused
		'\\', // [5d] \ |
		'\0', // [5e] unused
		'\0', // [5f] F24
		'\0', // [60] unused
		'\0', // [61] Europe 2 (Note 2)
		'\0', // [62] unused
		'\0', // [63] unused
		'\0', // [64] unused
		'\0', // [65] unused
		'\b', // [66] Backspace
		'\0', // [67] unused
		'\0', // [68] unused
		'1',  // [69] Keypad 1 End
		'\0', // [6a] unused
		'4',  // [6b] Keypad 4 Left
		'7',  // [6c] Keypad 7 Home
		'\0', // [6d] unused
		'\0', // [6e] unused
		'\0', // [6f] unused
		'0',  // [70] Keypad 0 Insert
		'.',  // [71] Keypad . Delete
		'2',  // [72] Keypad 2 Down
		'5',  // [73] Keypad 5
		'6',  // [74] Keypad 6 Right
		'8',  // [75] Keypad 8 Up
		(char)27, // [76] Escape
		'\0', // [77] Num Lock
		'\0', // [78] F11
		'+',  // [79] Keypad +
		'3',  // [7a] Keypad 3 PageDn
		'-',  // [7b] Keypad -
		'*',  // [7c] Keypad *
		'9',  // [7d] Keypad 9 PageUp
	};

	const byte AnsiTranslator::pauseKeySequence[] PROGMEM {
		0xe1, 0x14, 0x77
	};

	AnsiTranslator::AnsiTranslator()
	{
		isSpecial = false;
		isUnmake = false;
		isCtrlDown = false;
		isShiftDown = false;
		isCapsLockMode = false;
		isNumLockMode = false;
		pauseKeySequenceIndex = 0;
	}

	void AnsiTranslator::reset() {
		isSpecial = false;
		isUnmake = false;
	}

	char AnsiTranslator::translatePs2Keycode(KeyboardOutput ps2Scan)
	{
		if (ps2Scan == KeyboardOutput::unmake)
		{
			this->isUnmake = true;
			return '\0';
		}

		if (ps2Scan == KeyboardOutput::extend)
		{
			this->isSpecial = true;
			return '\0';
		}

		byte usbCode = 0;
		if ((uint8_t)ps2Scan == pgm_read_byte(pauseKeySequence + this->pauseKeySequenceIndex)) {
			++this->pauseKeySequenceIndex;
			if (this->pauseKeySequenceIndex < sizeof(pauseKeySequence))
				return '\0';

			this->pauseKeySequenceIndex = 0;
			this->isSpecial = false;
			this->isUnmake = false;
			return '\0';
		}

		switch (ps2Scan) {
		case KeyboardOutput::sc2_L_SHIFT:
		case KeyboardOutput::sc2_R_SHIFT:
			this->isShiftDown = !this->isUnmake;
			break;
		case KeyboardOutput::sc2_CTRL:
			this->isCtrlDown = !this->isUnmake;
			break;
		}

		// We have a complete make or unmake sequence here, so we'll reset to be ready for the next key
		//  and we can return when we know something...
		pauseKeySequenceIndex = 0;

		if (this->isUnmake || (this->isSpecial && ps2Scan != KeyboardOutput::sc2_KP_ENTER)) {
			// We only care about unmakes for modifier keys
			// None of the extended set are normal characters except for the Keypad Enter key
			this->isUnmake = false;
			this->isSpecial = false;
			return '\0';
		}

		switch (ps2Scan) {
		case KeyboardOutput::sc2_NUM:
			this->isNumLockMode = !this->isNumLockMode;
			return '\0';
		case KeyboardOutput::sc2_CAPS:
			this->isCapsLockMode = !this->isCapsLockMode;
			return '\0';
		}

		char charTranslation = this->rawTranslate(ps2Scan);
		if (charTranslation == '\0') {
			return '\0';
		}
		else if (!this->isNumLockMode && isKeyAffectedByNumlock(ps2Scan, charTranslation)) {
			return '\0';
		}
		else if (charTranslation >= 'a' && charTranslation <= 'z')
		{
			// Shift  Caps  ToUpper?
			//   F     F      F
			//   T     F      T
			//   F     T      T
			//   T     T      F
			if (charTranslation >= 'a' && charTranslation <= 'z' && (this->isShiftDown != this->isCapsLockMode)) {
				charTranslation = charTranslation - 'a' + 'A';
			}
			if (charTranslation >= 'a' && charTranslation <= 'z' && this->isCtrlDown) {
				charTranslation = charTranslation - 'a' + 1;
			}
		}
		else if (this->isShiftDown) {
			switch (charTranslation) {
			case '`':
				charTranslation = '~';
				break;
			case '1':
				charTranslation = '!';
				break;
			case '2':
				charTranslation = '@';
				break;
			case '3':
				charTranslation = '#';
				break;
			case '4':
				charTranslation = '$';
				break;
			case '5':
				charTranslation = '%';
				break;
			case '6':
				charTranslation = '^';
				break;
			case '7':
				charTranslation = '&';
				break;
			case '8':
				charTranslation = '*';
				break;
			case '9':
				charTranslation = '(';
				break;
			case '0':
				charTranslation = ')';
				break;
			case '-':
				charTranslation = '_';
				break;
			case '=':
				charTranslation = '+';
				break;
			case '[':
				charTranslation = '{';
				break;
			case ']':
				charTranslation = '}';
				break;
			case ';':
				charTranslation = ':';
				break;
			case '\'':
				charTranslation = '"';
				break;
			case ',':
				charTranslation = '<';
				break;
			case '.':
				charTranslation = '>';
				break;
			case '/':
				charTranslation = '?';
				break;
			case '\\':
				charTranslation = '|';
				break;
			}
		}

		return charTranslation;
	}

	char AnsiTranslator::rawTranslate(KeyboardOutput ps2Scan) {
		return ((uint8_t)ps2Scan >= 0x0d && ((uint8_t)ps2Scan - 0x0d) < sizeof(ps2ToAsciiMap))
			? (char)pgm_read_byte(ps2ToAsciiMap + (uint8_t)ps2Scan - 0x0d)
			: '\0';
	}

	bool AnsiTranslator::isKeyAffectedByNumlock(KeyboardOutput ps2Scan, char rawTranslation) {
		if (ps2Scan < KeyboardOutput::sc2_KP_1)
			return false;
		return rawTranslation == '.' || (rawTranslation >= '0' && rawTranslation <= '9');
	}
}