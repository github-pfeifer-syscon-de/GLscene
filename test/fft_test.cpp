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
#include <complex.h>        // the divertion to complex for fftw3 does not work for c++ as this forwards to #inc<complex>
#include <complex>
#include <fftw3.h>
#include <numeric>  // accumulate
#include <algorithm>
#include <psc_format.hpp>

#include "CooleyTukey.hpp"
#include "Fft.hpp"

#define REAL 0
#define IMAG 1

#define XSTR(x) STR(x)
#define STR(x) #x

static constexpr uint32_t windowSize{2048};
static constexpr uint32_t hopSize{2048};



template<uint32_t windowSize>
static bool
check_fft(Fft<windowSize>* fft, const std::string& name, ChunkedArray<int16_t>& data)
{

    //std::list<std::shared_ptr<std::vector<int16_t>>> list;
    //list.emplace_back(std::move(data));
    auto start = std::chrono::steady_clock::now();
    auto spec = fft->execute(data);
    //auto out = spec->adjustLin(16, 0.5);
    auto finish = std::chrono::steady_clock::now();
    double elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(finish - start).count();
    std::cout << name <<  " duration " << elapsed_seconds << std::endl;
    double err2k2k{};
    auto max = std::numeric_limits<double>::lowest();
    auto sum = spec->getSum();
    for (uint32_t i = 0; i < sum.size(); ++i) {
        max = std::max(max, sum[i]);
    }
    for (uint32_t i = 0; i < sum.size(); ++i) {
        if (i >= 11 && i <= 14) {
        std::cout << name << " " << i << " = " << sum[i] << std::endl;
        }
        if (sum[i] < max) {
            err2k2k += sum[i];
        }
    }
    std::cout << name << " err " << err2k2k << std::endl;

    return true;
}


static bool
check_alsa(const std::string& name, ChunkedArray<int16_t>& data)
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
//
//    const double cnt{16.0};
//    const uint32_t sumCnt{200};
//    double factorLin = cnt / static_cast<double>(sumCnt);
//    double factorLog = (10.0 - 1.0) / static_cast<double>(sumCnt);
//    for (size_t i = 0; i < sumCnt; ++i) {
//        auto nLin = static_cast<size_t>(static_cast<double>(i) * factorLin);
//        auto nLog = static_cast<size_t>(std::log10(1.0 + static_cast<double>(i) * factorLog) * static_cast<double>(cnt));
//        auto f = (22050.0 / static_cast<double>(sumCnt-1) * static_cast<double>(i+1));
//        std::cout << "i " << i
//                  << " f " << f
//                  << " lin " << nLin
//                  << " log " << nLog << std::endl;
//    }
    return true;
}

#define REAL 0
#define IMAG 1
void calc_freq(const float* data, float* freq);

static bool
check_fftaud()
{
//    std::cout << "--------------------------------" << std::endl;
//    std::vector<float> data;
//    data.resize(512);
//    float end{static_cast<float>(M_PI * 2.0) / 20.0f};
//    for (uint32_t i = 0; i < data.size(); ++i) {
//        data[i] = std::sinf(static_cast<float>(i) * end) ;  // * std::numeric_limits<int16_t>::max()
//    }
//    std::vector<float> freq;
//    freq.resize(256);
//    calc_freq(&data[0], &freq[0]);
//    for (uint32_t i = 0; i < freq.size(); ++i) {
//        std::cout << "aud " << i << " = " << freq[i] << std::endl;
//    }
    std::cout << "--------------------------------" << std::endl;
    SinusSignal sinus;
    auto dataFft = sinus.generate(2000, 20.0f);
    CooleyTukey<512> fft;
    auto start = std::chrono::steady_clock::now();
    //auto out = spec->adjustLin(16, 0.5);
    auto spec = fft.execute(dataFft);
    auto finish = std::chrono::steady_clock::now();
    double elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(finish - start).count();
    std::cout << "cooleyTukey duration " << elapsed_seconds << std::endl;
    for (uint32_t i = 0; i < spec.size() / 2 + 1; ++i) {
        std::cout << "cooleytukey " << i << " = " << spec[i] << std::endl;
    }


    start = std::chrono::steady_clock::now();
    //auto out = spec->adjustLin(16, 0.5);
    spec = fft.exec_wiki(dataFft);
    finish = std::chrono::steady_clock::now();
    elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(finish - start).count();
    std::cout << "cooleyTukey (wiki) duration " << elapsed_seconds << std::endl;
    for (uint32_t i = 0; i < spec.size() / 2 + 1; ++i) {
        std::cout << "cooleytukey wiki " << i << " = " << spec[i+1] << std::endl;
    }





    const size_t windowSize{512};
    // since the memory layout is all the same fake it
    //auto fft_input = fftw_alloc_complex(windowSize);
    //auto fft_result = fftw_alloc_complex(windowSize);
    auto fft_input = fftw_alloc_real(windowSize);
    auto fft_result = fftw_alloc_real(windowSize);
    double period{M_PI * 2.0 / 32.0};
    for (uint32_t i = 0; i < windowSize; ++i) {
        //fft_input[i] = std::complex<double>(std::sin(static_cast<double>(i) * period), 0.0);
        fft_input[i] = std::sin(static_cast<double>(i) * period);
        //std::cout << "in i " << i << " " << fft_input[i] << std::endl;
    }
    //fftw_plan plan_forward = fftw_plan_dft_1d(windowSize, fft_input, fft_result, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_plan plan_forward = fftw_plan_r2r_1d(windowSize, fft_input, fft_result, FFTW_R2HC, FFTW_ESTIMATE);
    const auto half {windowSize / 2};
    for (uint32_t i = 0, j = half; i < half; ++i, ++j) {   // as we create a half complex output
        auto val{std::sqrt(fft_result[i] * fft_result[i] + fft_result[j] * fft_result[j])};
        //std::cout << "dft " << i << " = r " << fft_result[i].real() << " i " << fft_result[i].imag() << " val " << val << std::endl;
        std::cout << "dft " << i << " val " << val << std::endl;
    }


    std::cout << "CompI " XSTR(_Complex_I)
              << " compl () " XSTR(complex)
              << "I " XSTR(I) << std::endl;

    fftw_destroy_plan(plan_forward);
    fftw_free(fft_input);
    fftw_free(fft_result);

    return true;
}


/*
 *
 */
int main(int argc, char** argv)
{
    for (uint32_t i = 3000; i < 3000; i+=1000) {
        SinusSignal sinus;
        auto data = sinus.generate(i, 40.0f);
        // Fft512n256 in  512 max at 13 with 0.733947    filled 256
        // Fft512n256 in 1000 max at 13 with 0.723768    filled  24
        // Fft512n256 in 2000 max at 13 with 0.562941    filled  48
        // Fft512n256 in 3000 max at 13 with 0.530473    filled  72
        // Fft512n256 in 4000 max at 13 with 0.516253    filled  96
        // Fft512n256 in 5000 max at 13 with 0.508062    filled 120
        // Fft512n256 in 10000 max at 13 with 0.490902   filled 240

        // full window
        // 512n256 1000 13 = 1.93394
        // 512n256 2000 13 = 1.16085
        // 512n256 3000 13 = 1.07483
        // 512n256 4000 13 = 1.04168
        // 512n256 5000 13 = 1.02416

        // Fft512 in  512 max at 13 with 0.483754  filled   0
        // Fft512 in 1000 max at 13 with 0.96432   filled  24
        // Fft512 in 2000 max at 13 with 0.642255  filled  48
        // Fft512 in 3000 max at 13 with 0.577272  filled  72
        // Fft512 in 4000 max at 13 with 0.548864  filled  96
        // Fft512 in 5000 max at 13 with 0.532507  filled 120
        // Fft512 in 10000 max at 13 with 0.498157 filled 240

        // full window
        // 512 1000 13 = 0.967509
        // 512 2000 13 = 1.45125
        // 512 3000 13 = 1.20919
        // 512 4000 13 = 1.12848
        // 512 5000 13 = 1.08821

        //Fft512 fft;
        Fft512 fft; // n256
        //fft.calibrate(100.0);
        auto name = psc::fmt::format("512 {}", i);
        if (!check_fft<512u>(&fft, name, data)) {
            return 1;
        }
    }

    //Fft2k1k fftPrec;
    //fftPrec.calibrate(100.0);
    //if (!check_fft(&fftPrec, "2k1k", data)) {
    //    return 2;
    //}
    //if (!check_alsa("local ", data)) {
    //    return 3;
    //}
    //if (!check_fftaud()) {
    //    return 4;
    //}

    return 0;
}

