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

#include <iostream>
#include <numeric>  // accumulate
#include <cmath>
#include <limits>
#include <algorithm>
#define REAL 0
#define IMAG 1


#include "Fft.hpp"
#include "config.h"

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
    // include a first correction with the factor we know, so the sum will not grow too fast
    for (size_t i = 0; i < m_sum.size(); i++) {
        auto abs = std::sqrt(fft_result[i][REAL] * fft_result[i][REAL] + fft_result[i][IMAG] * fft_result[i][IMAG]);
        m_sum[i] += abs * m_addScale;      // keep double as long as possible
    }
}

template <uint32_t windowSize>
void
Spectrum<windowSize>::scale(double nScale)
{
    for (size_t i = 0; i < m_sum.size(); i++) {
        //std::cout << "Spectrum::scale"
        //          << " i " << i
        //          << " sum " << m_sum[i]
        //          << " by " << nScale << std::endl;
        m_sum[i] *= nScale;
    }
}

template <uint32_t windowSize>
double
Spectrum<windowSize>::getMax()
{
    //double max{std::numeric_limits<double>::min()};
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
    std::ranges::fill(fout, 0.0);
    const size_t sumSize = static_cast<size_t>(static_cast<double>(m_sum.size()) * usageFactor);
    double factorLin = static_cast<double>(cnt) / static_cast<double>(sumSize);
    std::vector<size_t> binCnt;
    binCnt.resize(cnt);
    std::ranges::fill(binCnt, 0);
    double maxIn{std::numeric_limits<double>::min()};
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
    std::ranges::fill(fout, 0.0);
    const size_t sumSize = static_cast<size_t>(static_cast<double>(m_sum.size()) * usageFactor);
    double factorLog = (10.0 - 1.0) / static_cast<double>(sumSize);
    std::vector<size_t> binCnt;
    binCnt.resize(cnt);
    std::ranges::fill(binCnt, 0);
    double maxIn{std::numeric_limits<double>::min()};
    for (size_t i = 0; i < sumSize; ++i) {
        auto n = std::min(static_cast<size_t>(std::log10(1.0 + static_cast<double>(i) * factorLog) * static_cast<double>(cnt)), static_cast<size_t>(cnt-1));
        fout[n] = std::max(fout[n], static_cast<float>(m_sum[i] * factor));
        ++binCnt[n];
        maxIn = std::max(maxIn, m_sum[i]);
    }
    if (maxIn < 0.0001) {
        std::cout << "Spectrum::adjustLog silence " << maxIn << std::endl;
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


ChunkedArray<int16_t>
SinusSignal::generate(size_t samples, float period)
{
    std::cout << "createData"
              << " samples " << samples
              << " min " << std::numeric_limits<int16_t>::min()
              << " max " <<  std::numeric_limits<int16_t>::max()
              << " cd freq~ " << 44100.0f/period << "Hz" << std::endl;
    ChunkedArray<int16_t> data{1};
    float end{static_cast<float>(M_PI * 2.0) / period};
    const size_t blocking{2000};
    for (uint32_t i = 0; i < samples; i += blocking) {
        auto row = std::make_shared<std::vector<int16_t>>();
        row->reserve(blocking);
        for (uint32_t j = 0; j < std::min(static_cast<size_t>(blocking), samples-i); ++j) {
            float x = static_cast<float>(i+j) * end;
            float y = std::sinf(x) * static_cast<float>(std::numeric_limits<int16_t>::max());
            //std::cout << "i " << i << " x " << x << " y " << y << std::endl;
            row->push_back(static_cast<int16_t>(y));
        }
        data.add(row);
    }
    return data;
}


template <uint32_t windowSize, uint32_t hopSize>
Fft<windowSize, hopSize>::Fft(const std::shared_ptr<WindowFunction<windowSize>>& windowFunction)
: m_windowFunction{windowFunction}
{
    m_data = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * windowSize);
    m_fft_result = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * windowSize);
    m_plan_forward = fftw_plan_dft_1d(windowSize, m_data, m_fft_result, FFTW_FORWARD, FFTW_ESTIMATE);
    // others use e.g. https://github.com/GatzeTech/Qt-FFTW/blob/main/mainwindow.cpp
    //mFftIn  = fftw_alloc_real(NUM_SAMPLES);
    //mFftOut = fftw_alloc_real(NUM_SAMPLES);
    //mFftPlan = fftw_plan_r2r_1d(NUM_SAMPLES, mFftIn, mFftOut, FFTW_R2HC,FFTW_MEASURE);

}

template <uint32_t windowSize, uint32_t hopSize>
Fft<windowSize, hopSize>::~Fft()
{
    fftw_destroy_plan(m_plan_forward);
    if (m_data) {
        fftw_free(m_data);
        m_data = nullptr;
    }
    if (m_fft_result) {
        fftw_free(m_fft_result);
        m_fft_result = nullptr;
    }
}

template <uint32_t windowSize, uint32_t hopSize>
std::shared_ptr<Spectrum<windowSize>>
Fft<windowSize, hopSize>::execute(const ChunkedArray<int16_t>& in)
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

    spectrum->setAddScale(m_scale );    // * nScale
    // Process each chunk of the signal
    while (chunkPosition < in.size() && !bStopReadChunks) {   // signal
        // Copy the chunk into our buffer
        for (size_t i = 0; i < windowSize; i++) {
            size_t readIndex = chunkPosition + i;
            if (readIndex < in.size()) {    // signal
                m_data[i][REAL] = in[readIndex] * m_windowFunction->windowing(i);  // signal
                m_data[i][IMAG] = 0.0;
            }
            else {
                // we have read beyond the signal, so zero-pad it!
                m_data[i][REAL] = 0.0;
                m_data[i][IMAG] = 0.0;
                bStopReadChunks = true;
            }
        }
        // Perform the FFT on our chunk
        fftw_execute(m_plan_forward);
        // Copy the first (windowSize/2 + 1) data points into your spectrogram.
        // We do this because the FFT output is mirrored about the nyquist
        // frequency, so the second half of the data is redundant.
        spectrum->add(m_fft_result);
        chunkPosition += hopSize;
        numChunks++;
    }
    auto chunkScale = 1.0 / (static_cast<double>(numChunks));   // undo effect of sliding window
    //chunkScale *= m_scale;
    spectrum->scale(chunkScale);
#   ifdef DEBUG
    std::cout << "Fft::execute"
              << " min freq (4 cd) " << 44100/windowSize << "Hz"
              << " set scale " << m_scale
              << " chunkScale " << chunkScale
              << " chunks " << numChunks << std::endl;
#   endif
    return spectrum;
}

template <uint32_t windowSize, uint32_t hopSize>
double
Fft<windowSize, hopSize>::calibrate(double to)
{
    setScale(1.0);
    SinusSignal sig;
    ChunkedArray<int16_t> in = sig.generate(8000, 44100.0f/1000.0f);    // ~ 1kHz for Cd
    auto spec = execute(in);
    double factor = to / spec->getMax();
    std::cout << "Fft::calibrate"
              << " max " << spec->getMax()
              << " to " << to
              << " resulting factor " << factor << std::endl;
    setScale(factor);
    return getScale();
}

template <uint32_t windowSize, uint32_t hopSize>
double
Fft<windowSize, hopSize>::getScale()
{
    return m_scale;
}

template <uint32_t windowSize, uint32_t hopSize>
void
Fft<windowSize, hopSize>::setScale(double scale)
{
    m_scale = scale;
}


// need template instantiation
template class Spectrum<2048u>;

// need template instantiation
template class Fft<2048u,2048u>;

template class Fft<2048u,1024u>;

// only 4 testing
template class Fft<256u,256u>;