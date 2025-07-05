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

#include <glib-2.0/glib.h>

#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/mainloop.h>
#include <pulse/stream.h>
#include <pulse/glib-mainloop.h>

#include <ConcurrentCollections.hpp>

#include "ChunkedArray.hpp"

class Pulse
{
public:
    Pulse(GMainContext* ctx);
    explicit Pulse(const Pulse& orig) = delete;
    virtual ~Pulse();

    void ctxNotify(const pa_context_state state);

    virtual void serverInfo(const pa_server_info *info) = 0;
    virtual void addData(int16_t *data, size_t actualbytes) = 0;

    uint8_t getChannels();
    void setChannels(uint8_t channels);
    uint32_t getSamplePerSec();
    void setSamplePerSec(uint32_t samples);
    pa_sample_format getFormat();
    void setFormat(pa_sample_format format);
protected:
    pa_glib_mainloop* m_loop{};
    pa_context *m_ctx{};
    pa_stream *m_stream{};
    uint8_t m_channels{1u};
    uint32_t m_samplePerSec{44100u};
    pa_sample_format m_format{PA_SAMPLE_S16NE}; // use NE native endian or LE
};

class PulseIn
: public Pulse
{
public:
    PulseIn(GMainContext* ctx);
    explicit PulseIn(const PulseIn& orig) = delete;
    virtual ~PulseIn() = default;

    void serverInfo(const pa_server_info *info) override;
    void addData(int16_t *data, size_t actualbytes) override;

    ChunkedArray<int16_t> read();

protected:
    TListConcurrent<std::shared_ptr<std::vector<int16_t>>> m_data;

};

// see https://gist.github.com/toroidal-code/8798775
//  for play