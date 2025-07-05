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

#include <iostream>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <limits>
#include <chrono>
#include <fftw3.h>
#include <numeric>  // accumulate
#include <algorithm>
#define REAL 0
#define IMAG 1



#include "Fft.hpp"


static constexpr uint32_t windowSize{2048};
static constexpr uint32_t hopSize{2048};



template<uint32_t windowSize, uint32_t hopSize>
static bool
check_fft(Fft<windowSize, hopSize>* fft, const char* name, ChunkedArray<int16_t>& data)
{

    //std::list<std::shared_ptr<std::vector<int16_t>>> list;
    //list.emplace_back(std::move(data));
    auto start = std::chrono::steady_clock::now();
    auto spec = fft->execute(data);
    auto out = spec->adjustLog(16, 0.5);
    auto finish = std::chrono::steady_clock::now();
    double elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(finish - start).count();
    std::cout << name <<  " duration " << elapsed_seconds << std::endl;
    float err2k2k{};
    for (uint32_t i = 0; i < out.size(); ++i) {
        std::cout << name << " " << i << " = " << out[i] << std::endl;
        if (out[i] < 50.0f) {
            err2k2k += out[i];
        }
    }
    std::cout << name << " err " << err2k2k << std::endl;

    return true;
}


static bool
check_alsa(const char* name, ChunkedArray<int16_t>& data)
{
    // may be useful if alsa is on option
//    Capture capture("default", 16);
//    auto data = capture.read();
//    for (uint32_t i = 0; i < data.size(); ++i) {
//        std::cout << "read " << i << " = " << data[i] << std::endl;
//    }
//    auto out = executeFft(data, 16, 100.0f);
//    float err2k2k{};
//    for (uint32_t i = 0; i < out.size(); ++i) {
//        std::cout << name << " " << i << " = " << out[i] << std::endl;
//        if (out[i] < 50.0f) {
//            err2k2k += out[i];
//        }
//    }
//    std::cout << name << " err " << err2k2k << std::endl;

    const double cnt{16.0};
    const uint32_t sumCnt{200};
    double factorLin = cnt / static_cast<double>(sumCnt);
    double factorLog = (10.0 - 1.0) / static_cast<double>(sumCnt);
    for (size_t i = 0; i < sumCnt; ++i) {
        auto nLin = static_cast<size_t>(static_cast<double>(i) * factorLin);
        auto nLog = static_cast<size_t>(std::log10(1.0 + static_cast<double>(i) * factorLog) * static_cast<double>(cnt));
        auto f = (22050.0 / static_cast<double>(sumCnt-1) * static_cast<double>(i+1));
        std::cout << "i " << i
                  << " f " << f
                  << " lin " << nLin
                  << " log " << nLog << std::endl;
    }
    return true;
}

/*
 *
 */
int main(int argc, char** argv)
{
    SinusSignal sinus;
    auto data = sinus.generate(8000, 20.0f);
    auto hamming = std::make_shared<HammingWindow2k>();
    Fft2k fft(hamming);
    fft.calibrate(100.0);
    if (!check_fft(&fft, "2k2k", data)) {
        return 1;
    }
    Fft2k1k fftPrec(hamming);
    fftPrec.calibrate(100.0);
    if (!check_fft(&fftPrec, "2k1k", data)) {
        return 2;
    }
    if (!check_alsa("local ", data)) {
        return 3;
    }

    return 0;
}

