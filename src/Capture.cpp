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
//#include <complex.h>

#include "Capture.hpp"


Capture::Capture(std::string_view dev, uint32_t outSize)
: m_outSize{outSize}
, m_dev{dev}
{

}


Capture::~Capture()
{
    if (m_capture_handle) {
        snd_pcm_close(m_capture_handle);
        fprintf(stdout, "audio interface closed\n");
    }

}

snd_pcm_t*
Capture::init()
{
    int err;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_t *capture_handle{};
    if ((err = snd_pcm_open(&capture_handle, m_dev.c_str(), SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf (stderr, "cannot open audio device %s (%s)\n",
                 m_dev.c_str(), snd_strerror (err));
        snd_pcm_close(capture_handle);
        return nullptr;
    }

    fprintf(stdout, "audio interface opened\n");

    if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
        fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n",
                 snd_strerror (err));
        snd_pcm_close(capture_handle);
        return nullptr;
    }

    fprintf(stdout, "hw_params allocated\n");

    if ((err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0) {
        fprintf(stderr, "cannot initialize hardware parameter structure (%s)\n",
                 snd_strerror (err));
        snd_pcm_close(capture_handle);
        return nullptr;
    }

    fprintf(stdout, "hw_params initialized\n");

    if ((err = snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        fprintf(stderr, "cannot set access type (%s)\n",
                 snd_strerror (err));
        snd_pcm_close(capture_handle);
        return nullptr;
    }

    fprintf(stdout, "hw_params access setted\n");

    if ((err = snd_pcm_hw_params_set_format(capture_handle, hw_params, format)) < 0) {
        fprintf(stderr, "cannot set sample format (%s)\n",
                 snd_strerror (err));
        snd_pcm_close(capture_handle);
        return nullptr;
    }

    fprintf(stdout, "hw_params format setted\n");

    if ((err = snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &rate, 0)) < 0) {
        fprintf (stderr, "cannot set sample rate (%s)\n",
                 snd_strerror (err));
        snd_pcm_close(capture_handle);
        return nullptr;
    }

    fprintf(stdout, "hw_params rate setted\n");

    if ((err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, channels)) < 0) {
        fprintf (stderr, "cannot set channel count (%s)\n",
                 snd_strerror (err));
        return capture_handle;
    }

    fprintf(stdout, "hw_params channels setted\n");

    if ((err = snd_pcm_hw_params(capture_handle, hw_params)) < 0) {
        fprintf (stderr, "cannot set parameters (%s)\n",
                 snd_strerror (err));
        snd_pcm_close(capture_handle);
        return nullptr;
    }

    fprintf(stdout, "hw_params setted\n");

    snd_pcm_hw_params_free(hw_params);

    fprintf(stdout, "hw_params freed\n");

    if ((err = snd_pcm_prepare(capture_handle)) < 0) {
        fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
                 snd_strerror (err));
        snd_pcm_close(capture_handle);
        return nullptr;
    }
    fprintf(stdout, "audio interface prepared\n");
    return capture_handle;
}

std::vector<float>
Capture::read()
{
    std::vector<float> out;
    if (!m_capture_handle) {
        m_capture_handle = init();
        if (!m_capture_handle) {
            return out;
        }
    }
    std::vector<int16_t> buffer;
    buffer.resize(buffer_frames * snd_pcm_format_width(format) / 8 * channels);
    for (uint32_t i = 0; i < buffer.size(); ++i) {
        buffer[i] = 0;
    }

    fprintf(stdout, "buffer allocated\n");

    int err;
    if ((err = snd_pcm_readi(m_capture_handle, &buffer[0], buffer_frames)) != buffer_frames) {
        fprintf (stderr, "err %d read from audio interface failed (%s)\n",
                 err, snd_strerror(err));
    }
    //fprintf(stdout, "read data ");
    //for (int i = 0; i < 16; ++i) {
    //    fprintf(stdout, "%d;", buffer[i]);
    //}
    //fprintf (stdout, "\n");
    out.resize(buffer.size());
    for (uint32_t i = 0; i < buffer.size(); ++i) {
        out[i] = static_cast<float>(buffer[i]) / 32768.0f;
    }

    return out;
}

