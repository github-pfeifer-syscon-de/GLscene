/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
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

#include <cmath>

#include "CooleyTukey.hpp"

template <uint32_t windowSize>
CooleyTukey<windowSize>::CooleyTukey()
{
    static_assert(std::fmod(std::log2(windowSize), 1.0) == 0.0, "requires 2^windowSize");

    m_reversed.resize(windowSize);
    for (uint32_t n = 0; n < windowSize; n++) {
        m_reversed[n] = bit_reverse(n);
    }
    m_roots.resize(windowSize / 2);
    for (uint32_t n = 0; n < m_roots.size(); n++) {
        m_roots[n] = std::exp(std::complex<float>(0.0f, static_cast<float>(n) * TWO_PI_N));
    }

}

template <uint32_t windowSize>
int
CooleyTukey<windowSize>::bit_reverse(int x)
{
    int y = 0;
    for (int n = LOGN; n--;) {
        y = (y << 1) | (x & 1);
        x >>= 1;
    }
    return y;
}

// if you are looking for other Cooley-Tukey examples
//   audacious has a simpler examples ...

template <uint32_t windowSize>
std::vector<float>
CooleyTukey<windowSize>::execute(const ChunkedArray<int16_t>& in)
{
    // see https://en.wikipedia.org/wiki/Cooley%E2%80%93Tukey_FFT_algorithm
    std::vector<float> spectrum;
    spectrum.resize(windowSize / 2 + 1);
    std::ranges::fill(spectrum, 0.0f);
    if (in.empty()) {
        return spectrum;
    }
    if (m_normHamming.empty()) {
        m_normHamming.resize(windowSize);
        auto end = static_cast<float>(2.0 * M_PI) / static_cast<float>(windowSize - 1);
        for (uint32_t n = 0; n < windowSize; n++) {
            m_normHamming[n] = 0.53836f - (0.46164f * std::cosf( (static_cast<float>(n) * end)));
        }
    }
    uint32_t chunks{};
    std::vector<std::complex<float>> A;
    A.resize(windowSize);
    std::ranges::fill(A, std::complex<float>(0.0f, 0.0f));
    for (uint32_t p = 0; p < in.size(); p += hopSize) {
        for (uint32_t k = 0; k < windowSize; ++k) {
            A[m_reversed[k]] = std::complex<float>((p + k < in.size())
                                                    ? (in[p + k] * static_cast<float>(in.getInputScale()) * m_normHamming[k])
                                                    : 0.0f
                                                  , 0.0f);
        }
        for (uint32_t s = 1, m = 2; s <= LOGN; s++, m *= 2) {
            auto omega_m = std::exp(std::complex<float>(0.0f, -TWO_PI / static_cast<float>(m)));
            for (uint32_t k = 0; k < windowSize; k += m) {
                auto omega{std::complex<float>(1.0f, 0.0f)};
                for (uint32_t j = 0; j < m / 2; ++j ) {
                    auto t = omega * A[k + j + m / 2];
                    auto u = A[k + j];
                    A[k + j] = u + t;
                    A[k + j + m / 2] = u - t;
                    omega *= omega_m;
                }
            }
        }


        /* output values are divided by N */
        /* frequencies from 1 to N/2-1 are doubled */
        for (uint32_t n = 0; n < windowSize / 2 + 1; n++) {
            auto f{(n == windowSize / 2 - 1) ? 1.0f : 2.0f};           /* frequency N/2 is not doubled */
            spectrum[n] += (f * std::abs(A[n]) - 1.0f) / static_cast<float>(windowSize);
        }
        ++chunks;
    }
    float scale = 1.0f / static_cast<float>(chunks);
    for (uint32_t n = 0; n < windowSize / 2 + 1; n++) {
        spectrum[n] += scale;
    }
    return spectrum;
}

template class CooleyTukey<512u>;