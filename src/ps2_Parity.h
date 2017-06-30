#pragma once
namespace ps2 {

	enum class Parity : uint8_t {
		odd = 0,
		even = 1,
	};
	inline Parity operator^=(Parity& a, int bit)
	{
		a = bit ? ((a == Parity::odd) ? Parity::even : Parity::odd) : a;
	}
}
