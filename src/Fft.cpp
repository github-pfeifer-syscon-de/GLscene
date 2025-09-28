/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
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

#include <iostream>
#include <numeric>  // accumulate
#include <cmath>
#include <limits>
#include <algorithm>


#include "Fft.hpp"
#include "config.h"

template <uint32_t windowSize>
std::vector<size_t>
Spectrum<windowSize>::m_lookup;

template <uint32_t windowSize>
Spectrum<windowSize>::Spectrum()
{
    m_sum.resize(windowSize / 2 + 1);  // fixed relationship, the remaining bins are mirrored
    std::ranges::fill(m_sum, 0.0);
}


template <uint32_t windowSize>
void
Spectrum<windowSize>::setAddScale(double addScale)
{
    m_addScale = addScale;
}


template <uint32_t windowSize>
void
Spectrum<windowSize>::add(fftw_complex* fft_result)
{
    // Copy the first (windowSize/2 + 1) data points into your spectrogram.
    // We do this because the FFT output is mirrored about the nyquist
    // frequency, so the second half of the data is redundant.
    // include a first correction with the factor we know, so the sum will not grow too fast
    for (size_t i = 0; i < m_sum.size(); i++) {
        auto abs = std::sqrt(fft_result[i][Fft<windowSize>::REAL] * fft_result[i][Fft<windowSize>::REAL] + fft_result[i][Fft<windowSize>::IMAG] * fft_result[i][Fft<windowSize>::IMAG]);
        auto scale = m_addScale;
        if (i < m_sum.size() -1) {
            scale *= 2.0;
        }
        //if (i >= 11 && i <= 14) {
        //    std::cout << "add " << i
        //              << " abs " << abs
        //              << " scale " << scale << std::endl;
        //}
        m_sum[i] += abs * scale / static_cast<double>(windowSize);      // keep double as long as possible
    }
}

template <uint32_t windowSize>
void
Spectrum<windowSize>::scale(double nScale)
{
#   ifdef DEBUG
    std::cout << "Spectrum::scale"
    //          << " sum " << m_sum[i]
              << " by " << nScale << std::endl;
#   endif
    for (size_t i = 0; i < m_sum.size(); i++) {
        m_sum[i] *= nScale;
    }
}

template <uint32_t windowSize>
double
Spectrum<windowSize>::getMax()
{
    //double max{std::numeric_limits<double>::lowest()};
    //for (size_t i = 0; i < m_sum.size(); i++) {
    //    max = std::max(max, m_sum[i]);
    //}
    //return max;
    return std::ranges::max(m_sum);
}


/*
 * with a linear adjustment ~ the frequencies upto ~2k go into the lowest bin
 *   which is not intuitive
 */
template <uint32_t windowSize>
std::vector<float>
Spectrum<windowSize>::adjustLin(size_t cnt, double usageFactor, double factor, bool keepSum)
{
    std::vector<float> fout;
    fout.resize(cnt);
    std::ranges::fill(fout, 0.0f);
    const size_t sumSize = static_cast<size_t>(static_cast<double>(m_sum.size()) * usageFactor);
    double factorLin = static_cast<double>(cnt) / static_cast<double>(sumSize);
    std::vector<size_t> binCnt;
    binCnt.resize(cnt);
    std::ranges::fill(binCnt, 0);
    double maxIn{std::numeric_limits<double>::lowest()};
    for (size_t i = 0; i < sumSize; ++i) {
        auto n = static_cast<size_t>(static_cast<double>(i) * factorLin);
        fout[n] += static_cast<float>(m_sum[i] * factor);
        ++binCnt[n];
        maxIn = std::max(maxIn, m_sum[i]);
    }
    if (maxIn < 0.0001) {
        return fout;        // don't scale silence
    }
    float maxOut{std::numeric_limits<float>::min()};
    for (size_t n = 0; n < fout.size(); ++n) {
        if (!keepSum) {
            fout[n] /= static_cast<float>(binCnt[n]);
        }
        maxOut = std::max(maxOut, fout[n]);
    }
    //auto scale = static_cast<float>(maxVal) / maxOut;
    //for (size_t i = 0; i < fout.size(); ++i) {
    //    fout[i] *= scale;
    //}
#   ifdef DEBUG
    std::cout << "Spectrum::adjustLin"
              << " usageFactor " << usageFactor
              << " factor " << factorLin
              << " maxIn " << maxIn
              << " maxOut " << maxOut << std::endl;
#   endif
    return fout;
}


/**
 * @param cnt number of resulting "bins"
 * @param factor focus on the specific range e.g. 0.5 will limit the shown frequencies to ~10k
 *         as the full range would be for a CD-sample rate of 44100 up to 22050,
 *         which usually is not the region of interest.
 * @return a vector with adjusted bins
 */
template <uint32_t windowSize>
std::vector<float>
Spectrum<windowSize>::adjustLog(size_t cnt, double usageFactor, double factor, bool keepSum)
{
    std::vector<float> fout;
    fout.resize(cnt);
    std::ranges::fill(fout, 0.0f);
    const size_t sumSize = static_cast<size_t>(static_cast<double>(m_sum.size()) * usageFactor);
    double factorLog = (10.0 - 1.0) / static_cast<double>(sumSize);
    std::vector<size_t> binCnt;
    binCnt.resize(cnt);
    std::ranges::fill(binCnt, 0);
    double maxIn{std::numeric_limits<double>::lowest()};
    for (size_t i = 0; i < sumSize; ++i) {
        auto n = std::min(static_cast<size_t>(std::log10(1.0 + static_cast<double>(i) * factorLog) * static_cast<double>(cnt)), static_cast<size_t>(cnt-1));
        fout[n] = std::max(fout[n], static_cast<float>(m_sum[i] * factor));
        ++binCnt[n];
        maxIn = std::max(maxIn, m_sum[i]);
    }
    if (maxIn < 0.0001) {
#       ifdef DEBUG
        std::cout << "Spectrum::adjustLog silence " << maxIn << std::endl;
#       endif
        return fout;        // dont scale silence up
    }
    float maxOut{std::numeric_limits<float>::min()};
    for (size_t n = 0; n < fout.size(); ++n) {
        if (!keepSum) {
            fout[n] /= static_cast<float>(binCnt[n]);
        }
        maxOut = std::max(maxOut, fout[n]);
    }
    //auto scale = static_cast<float>(maxVal) / maxOut;
    //for (size_t i = 0; i < fout.size(); ++i) {
    //    fout[i] *= scale;
    //}
#   ifdef DEBUG
    std::cout << "Spectrum::adjustLog"
              << " usageFactor " << usageFactor
              << " factor log " << factorLog
              << " maxIn " << maxIn
              << " maxOut " << maxOut << std::endl;
#   endif
    return fout;
}



// need template instantiation
template class Spectrum<2048u>;

template class Spectrum<512u>;

SignalGenerator::SignalGenerator()
: m_scale{static_cast<float>(std::numeric_limits<int16_t>::max())}
{
}

float
SignalGenerator::getScale()
{
    return m_scale;
}

void
SignalGenerator::setScale(float scale)
{
    m_scale = scale;
}

SinusSignal::SinusSignal()
: SignalGenerator()
{
}

ChunkedArray<int16_t>
SinusSignal::generate(size_t samples, float period)
{
#   ifdef DEBUG
    std::cout << "SinusSignal::generate"
              << " samples " << samples
              << " scale " << m_scale  << std::endl;
#   endif
    ChunkedArray<int16_t> data{1};
    float end{static_cast<float>(M_PI * 2.0) / period};
    const size_t blocking{2000};
    const auto scale = getScale();
    for (uint32_t i = 0; i < samples; i += blocking) {
        auto row = std::make_shared<std::vector<int16_t>>();
        row->reserve(blocking);
        for (uint32_t j = 0; j < std::min(static_cast<size_t>(blocking), samples-i); ++j) {
            float x = static_cast<float>(i+j) * end;
            float y = std::sinf(x) * scale;
            //std::cout << "i " << i << " x " << x << " y " << y << std::endl;
            row->push_back(static_cast<int16_t>(y));
        }
        data.add(row);
    }
    return data;
}

SquareSignal::SquareSignal()
: SignalGenerator()
{
}

ChunkedArray<int16_t>
SquareSignal::generate(size_t samples, float period)
{
#   ifdef DEBUG
    std::cout << "SquareSignal::generate"
              << " samples " << samples
              << " scale " << m_scale << std::endl;
#   endif
    ChunkedArray<int16_t> data{1};
    int iperiod = static_cast<int>(period);
    int iperiod2{ iperiod / 2};
    const size_t blocking{2000};
    const auto scale = getScale();
    for (uint32_t i = 0; i < samples; i += blocking) {
        auto row = std::make_shared<std::vector<int16_t>>();
        row->reserve(blocking);
        for (uint32_t j = 0; j < std::min(static_cast<size_t>(blocking), samples-i); ++j) {
            int x = (i+j) % iperiod;
            float y = (x <= iperiod2 ? 1.0f : -1.0f)  * scale;
            //std::cout << "i " << i << " x " << x << " y " << y << std::endl;
            row->push_back(static_cast<int16_t>(y));
        }
        data.add(row);
    }
    return data;
}



template <uint32_t windowSize>
Fft<windowSize>::Fft(const std::shared_ptr<WindowFunction<windowSize>>& windowFunction)
: m_windowFunction{windowFunction}
{
    m_fft_input = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * windowSize);
    m_fft_result = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * windowSize);
    m_plan_forward = fftw_plan_dft_1d(windowSize, m_fft_input, m_fft_result, FFTW_FORWARD, FFTW_ESTIMATE);
    // others use e.g. https://github.com/GatzeTech/Qt-FFTW/blob/main/mainwindow.cpp
    //mFftIn  = fftw_alloc_real(NUM_SAMPLES);
    //mFftOut = fftw_alloc_real(NUM_SAMPLES);
    //mFftPlan = fftw_plan_r2r_1d(NUM_SAMPLES, mFftIn, mFftOut, FFTW_R2HC,FFTW_MEASURE);

}

template <uint32_t windowSize>
Fft<windowSize>::~Fft()
{
    fftw_destroy_plan(m_plan_forward);
    if (m_fft_input) {
        fftw_free(m_fft_input);
        m_fft_input = nullptr;
    }
    if (m_fft_result) {
        fftw_free(m_fft_result);
        m_fft_result = nullptr;
    }
}

template <uint32_t windowSize>
std::shared_ptr<Spectrum<windowSize>>
Fft<windowSize>::execute(const ChunkedArray<int16_t>& in)
{
    auto spectrum = std::make_shared<Spectrum<windowSize>>();
    if (in.empty()) {
        return spectrum;
    }
    if (in.getChannels() != 1) {
        throw std::runtime_error("fft only prepared for a single channel!");
    }

    // see https://ofdsp.blogspot.com/2011/08/short-time-fourier-transform-with-fftw3.html
    // more on windowing https://download.ni.com/evaluation/pxi/Understanding%20FFTs%20and%20Windowing.pdf
    size_t chunkPosition{};
    bool bStopReadChunks{false};
    size_t numChunks{};
    // https://dsp.stackexchange.com/questions/63001/why-should-i-scale-the-fft-using-1-n
    //auto nScale = 1.0 / std::sqrt(static_cast<double>(windowSize));
                         //  * static_cast<double>(std::numeric_limits<int16_t>::max()));      // since we want output not depend on on used input (and here it's simpler to apply than on input with the same result)

    spectrum->setAddScale(m_scale);    // * nScale
    // Process each chunk of the signal
    const auto inputScale = in.getInputScale();
    uint32_t filled{};
    double maxIn1{std::numeric_limits<double>::lowest()};
    [[maybe_unused]]
    double maxIn2{std::numeric_limits<double>::lowest()};
    // only use the parts that are fully usable, to avoid the uncertainty when padding (-> try to stabilize the output levels)
    while (chunkPosition < in.size() - windowSize - 1 && !bStopReadChunks) {   // use only complete windows (inaccurate, waters down max values)
        // Copy the chunk into our buffer
        for (size_t i = 0; i < windowSize; i++) {
            size_t readIndex = chunkPosition + i;
            if (readIndex < in.size()) {    // signal
                auto v = static_cast<double>(in[readIndex] * inputScale);  // signal
                maxIn1 = std::max(maxIn1, v);
                auto w = v * m_windowFunction->windowing(i);
                maxIn2 = std::max(maxIn2, w);
                m_fft_input[i][REAL] = w;
                m_fft_input[i][IMAG] = 0.0;
            }
            else {
                // we have read beyond the signal, so zero-pad it!
                m_fft_input[i][REAL] = 0.0;
                m_fft_input[i][IMAG] = 0.0;
                bStopReadChunks = true;
                ++filled;
            }
        }
        // Perform the FFT on our chunk
        fftw_execute(m_plan_forward);
        spectrum->add(m_fft_result);
        chunkPosition += m_hopSize;
        numChunks++;
    }
    auto chunkScale = 1.0 / (static_cast<double>(numChunks));   // undo effect of sliding window (reduce by windowing)
    //chunkScale *= m_scale;
    spectrum->scale(chunkScale);
#   ifdef DEBUG
    std::cout << "Fft::execute"
            //  << " min freq (4 cd) " << 44100/windowSize << "Hz"
              << " set scale " << m_scale
              << " maxIn1 " << maxIn1
              << " maxIn2 " << maxIn2
              << " filled " << filled
              << " chunks " << numChunks << std::endl;
#   endif
    return spectrum;
}

template <uint32_t windowSize>
double
Fft<windowSize>::calibrate(double to)
{
    setScale(1.0);
    SinusSignal sig;
    ChunkedArray<int16_t> in = sig.generate(8000, 44100.0f/1000.0f);    // ~ 1kHz for Cd
    auto spec = execute(in);
    double factor = to / spec->getMax();
#   ifdef DEBUG
    std::cout << "Fft::calibrate"
              << " max " << spec->getMax()
              << " to " << to
              << " resulting factor " << factor << std::endl;
#   endif
    setScale(factor);
    return getScale();
}

template <uint32_t windowSize>
double
Fft<windowSize>::getScale()
{
    return m_scale;
}

template <uint32_t windowSize>
void
Fft<windowSize>::setScale(double scale)
{
    m_scale = scale;
}

template <uint32_t windowSize>
uint32_t
Fft<windowSize>::getHopSize()
{
    return m_hopSize;
}

template <uint32_t windowSize>
void
Fft<windowSize>::setHopSize(uint32_t hopSize)
{
    m_hopSize = hopSize;
}

// need template instantiation
template class Fft<2048u>;

// maybe audacious is right, avoid the lowest frequencies,
// as for modern music these are most prominent
template class Fft<512u>;

// only 4 testing
template class Fft<256u>;
