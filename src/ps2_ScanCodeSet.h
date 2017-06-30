#pragma once

namespace ps2 {
	enum class ScanCodeSet : uint8_t {
		defaultScanCodeSet = 2,

		pcxt = 1, // PC/XT Keyboard
		pcat = 2, // IBM/AT Keyboard
		ps2 = 3, // PS2

		error = 0xff,
	};
}