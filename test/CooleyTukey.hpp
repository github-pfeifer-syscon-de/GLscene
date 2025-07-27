/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4;  coding: utf-8; -*-  */
/*
 * Copyright (C) 2025 RPf <gpl3@pfeifer-syscon.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <cmath>
#include <vector>
#include <complex>

#include "ChunkedArray.hpp"

template <uint32_t windowSize>
class CooleyTukey
{
public:
    CooleyTukey();
    explicit CooleyTukey(const CooleyTukey& orig) = delete;
    virtual ~CooleyTukey() = default;

    std::vector<float> execute(const ChunkedArray<int16_t>& in);


protected:
    static constexpr auto TWO_PI = static_cast<float>(M_PI * 2.0);
    static constexpr auto TWO_PI_N = TWO_PI / static_cast<float>(windowSize);
    static constexpr auto LOGN{std::log2(windowSize)}; /* log N (base 2) */
    std::vector<float> m_hamming;
    std::vector<uint32_t> m_reversed;
    std::vector<std::complex<float>> m_roots;
    int bit_reverse(int x);
    const uint32_t hopSize = windowSize;
    std::vector<float> m_normHamming;
private:

};

