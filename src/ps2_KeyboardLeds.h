#pragma once

namespace ps2 {
	enum class KeyboardLeds {
		Ps2LedCapsLock = 0x4,
		Ps2LedNumLock = 0x2,
		Ps2LedScrollLock = 0x1,

		Ps2LedAll = 0x7,
		Ps2LedNone = 0x0,
	};
	inline KeyboardLeds operator|(KeyboardLeds a, KeyboardLeds b)
	{
		return static_cast<KeyboardLeds>(static_cast<int>(a) | static_cast<int>(b));
	}
}