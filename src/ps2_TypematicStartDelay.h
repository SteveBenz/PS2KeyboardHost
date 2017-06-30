#pragma once

namespace ps2 {
	enum class TypematicStartDelay : uint8_t {
		longestDelay = 0x3,
		shortestDelay = 0x0,
		defaultDelay = 0x1,

		_0_25_sec = 0x0,
		_0_50_sec = 0x1,
		_0_75_sec = 0x2,
		_1_00_sec = 0x3,
	};
}