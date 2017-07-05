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