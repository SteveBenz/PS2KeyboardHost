#pragma once

namespace ps2 {
	enum class TypematicRate : uint8_t {
		fastestRate = 0x00,
		slowestRate = 0x1f,
		defaultRate = 0x0b,

		_30_0_cps = 0x00,
		_26_7_cps = 0x01,
		_24_0_cps = 0x02,
		_21_8_cps = 0x03,
		_10_9_cps = 0x0b,
		_02_3_cps = 0x1d,
		_02_1_cps = 0x1e,
		_02_0_cps = 0x1f,
		// ... fill this the rest of the way if you care
	};
}