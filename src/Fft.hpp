/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4;  coding: utf-8; -*-  */
/*
 * Copyright (C) 2025 RPf 
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
//#include <complex.h>  seems not to work
#include <fftw3.h>
#include <vector>

#include "ChunkedArray.hpp"


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

template class HammingWindow<512u>;

class HammingWindow512
: public HammingWindow<512u>
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
    std::vector<float> adjustLin(size_t cnt, double usageFactor, double factor = 1.0, bool keepSum = false);
    // logarithmic adjustment for frequency
    //   the db adjustment for level would be nice,
    //   but we have to deal with changing levels (as we are on the end of the processing chain)
    //     and the lib-fft functions i could not make a fixed connections from input-levels to output
    std::vector<float> adjustLog(size_t cnt, double usageFactor, double factor = 1.0, bool keepSum = false);
    void add(fftw_complex* fft_result);
    void scale(double nScale);
    double getMax();
    void setAddScale(double addScale);
    std::vector<double>& getSum()
    {
        return m_sum;
    }
    void add(int32_t pos, float val)
    {
        m_sum[pos] += val;
    }
    std::vector<double> getVector()
    {
        return m_sum;
    }
private:
    std::vector<double> m_sum;
    double m_addScale{1.0};
    static std::vector<size_t> m_lookup;

};

class SignalGenerator
{
public:
    SignalGenerator();
    virtual ~SignalGenerator() = default;

    float getScale();
    void setScale(float scale);

    virtual ChunkedArray<int16_t> generate(size_t samples, float period) = 0;
protected:
    float m_scale;
};

class SinusSignal
: public SignalGenerator
{
public:
    SinusSignal();
    virtual ~SinusSignal() = default;

    ChunkedArray<int16_t> generate(size_t samples, float period) override;
};

class SquareSignal
: public SignalGenerator
{
public:
    SquareSignal();
    virtual ~SquareSignal() = default;

    ChunkedArray<int16_t> generate(size_t samples, float period) override;
};


template <uint32_t windowSize = 2048u>
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
    uint32_t getHopSize();
    void setHopSize(uint32_t hopSize);
    static constexpr size_t REAL{0};
    static constexpr size_t IMAG{1};

protected:

private:
    uint32_t m_hopSize{windowSize};
    fftw_complex* m_fft_input{};
    fftw_complex* m_fft_result{};
    fftw_plan m_plan_forward;

    const std::shared_ptr<WindowFunction<windowSize>> m_windowFunction;
    double m_scale{1.0};
};


class Fft512
: public Fft<512u>
{
public:
    Fft512()
    : Fft{std::make_shared<HammingWindow512>()}
    {
    }
    explicit Fft512(const Fft512& orig) = delete;
    virtual ~Fft512() = default;

};


class Fft512n256
: public Fft<512u>
{
public:
    Fft512n256()
    : Fft{std::make_shared<HammingWindow512>()}
    {
        setHopSize(256u);
    }
    explicit Fft512n256(const Fft512n256& orig) = delete;
    virtual ~Fft512n256() = default;

};

// fastes variant
class Fft2k
: public Fft<2048u>
{
public:
    Fft2k()
    : Fft{std::make_shared<HammingWindow2k>()}
    {
    }
    explicit Fft2k(const Fft2k& orig) = delete;
    virtual ~Fft2k() = default;

};

// this is not meassurable faster, and not noteably more precise
class Fft2k1k
: public Fft<2048u>
{
public:
    Fft2k1k()
    : Fft{std::make_shared<HammingWindow2k>()}
    {
        setHopSize(1024u);
    }
    explicit Fft2k1k(const Fft2k1k& orig) = delete;
    virtual ~Fft2k1k() = default;

};
