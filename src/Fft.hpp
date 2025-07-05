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

#include <vector>
#include <list>
#include <memory>
#include <cstdint>
#include <array>
//#include <complex.h>  seem not to work
#include <fftw3.h>

#include "ChunkedArray.hpp"

using fftw_complex = double[2];

template <uint32_t windowSize = 2048u>
class WindowFunction
{
public:
    virtual double windowing(size_t idx) = 0;
    virtual double getCorrection()
    {
        double sum{};
        for (size_t i = 0; i < windowSize; ++i) {
            sum += windowing(i);
        }
        return sum / static_cast<double>(windowSize);
    }
};

template <uint32_t windowSize = 2048u>
class HammingWindow
: public WindowFunction<windowSize>
{
public:
    double windowing(size_t idx) override
    {
        return window[idx];
    }
    double getCorrection() override
    {
        return HAMMING_OFFS;
    }
    static constexpr auto HAMMING_OFFS{0.53836};
    static constexpr auto HAMMING_FACTOR{0.46164};

protected:
    static consteval // Create a hamming window see https://en.wikipedia.org/wiki/Window_function (modified factors from there)
    std::array<double, windowSize> hamming()
    {
        std::array<double, windowSize> buffer;
        auto end = 2.0 * M_PI / static_cast<double>(windowSize - 1);
        for(size_t i = 0; i < windowSize; i++) {
            buffer[i] =  HAMMING_OFFS - (HAMMING_FACTOR * std::cos( (static_cast<double>(i) * end)));
        }
        return buffer;
    }
    static constexpr std::array<double, windowSize> window = hamming();

};

// need template instantiation
template class HammingWindow<2048u>;

class HammingWindow2k
: public HammingWindow<2048u>
{
public:
};

template <uint32_t windowSize = 2048u>
class Spectrum
{
public:
    Spectrum();
    explicit Spectrum(const Spectrum& orig) = delete;
    virtual ~Spectrum() = default;

    // linear adjustment for frequency
    std::vector<float> adjustLin(size_t cnt, double usageFactor, double factor);
    // logarithmic adjustment for frequency
    std::vector<float> adjustLog(size_t cnt, double usageFactor);
    void add(fftw_complex* fft_result);
    void scale(double nScale);
    double getMax();
    void setAddScale(double addScale);
private:
    std::vector<double> m_sum;
    double m_addScale{1.0};
};

class SinusSignal
{
public:
    SinusSignal() = default;
    explicit SinusSignal(const SinusSignal& orig) = delete;
    virtual ~SinusSignal() = default;

    ChunkedArray<int16_t> generate(size_t samples, float period);

};


template <uint32_t windowSize = 2048u, uint32_t hopSize = 2048u>
class Fft
{
public:
    Fft(const std::shared_ptr<WindowFunction<windowSize>>& windowFunction);
    explicit Fft(const Fft& orig) = delete;
    virtual ~Fft();
    std::shared_ptr<Spectrum<windowSize>> execute(const ChunkedArray<int16_t>& data);
    // since there are many factors test the value out
    double calibrate(double to = 1.0);
    double getScale();
    void setScale(double scale);
protected:
private:
    fftw_complex* m_data{};
    fftw_complex* m_fft_result{};
    fftw_plan m_plan_forward;

    const std::shared_ptr<WindowFunction<windowSize>> m_windowFunction;
    double m_scale{1.0};
};



// fastes variant
class Fft2k
: public Fft<2048u, 2048u>
{
public:
    Fft2k(const std::shared_ptr<WindowFunction<2048u>>& windowFunction)
    : Fft(windowFunction)
    {
    }
    explicit Fft2k(const Fft2k& orig) = delete;
    virtual ~Fft2k()
    {
    }

};

// well not meassurable faster, and not noteably more precise
class Fft2k1k
: public Fft<2048u, 1024u>
{
public:
    Fft2k1k(const std::shared_ptr<WindowFunction<2048u>>& windowFunction)
    : Fft(windowFunction)
    {
    }
    explicit Fft2k1k(const Fft2k1k& orig) = delete;
    virtual ~Fft2k1k()
    {
    }

};
