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

#include <string_view>
#include <string>
#include <vector>
#include <cstdint>

#include <alsa/asoundlib.h>

// Just as reference using Alsa

class Capture
{
public:
    Capture(std::string_view dev, uint32_t outSize);
    explicit Capture(const Capture& orig) = delete;
    virtual ~Capture();
    [[nodiscard]]
    std::vector<float> read();

    static constexpr snd_pcm_format_t format{SND_PCM_FORMAT_S16_LE};
    static constexpr long buffer_frames{4410};
    unsigned int rate{44100};
    static constexpr unsigned int channels{1};
    std::vector<float> fft(const std::vector<float>& data);

protected:
    snd_pcm_t* init();
private:
    const uint32_t m_outSize;
    std::string m_dev;
    snd_pcm_t* m_capture_handle{};
};

