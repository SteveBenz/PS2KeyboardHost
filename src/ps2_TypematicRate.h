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