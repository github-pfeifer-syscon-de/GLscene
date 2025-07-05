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
#include <fstream>
#include <string>
#include <limits>
#include <stdexcept>

#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/mainloop.h>
#include <pulse/stream.h>
#include <pulse/glib-mainloop.h>


#include "Pulse.hpp"
#include "config.h"

static void
pa_stream_notify_cb(pa_stream *stream, void* userdata)
{
    const pa_stream_state state = pa_stream_get_state(stream);
    switch (state) {
        case PA_STREAM_CREATING:
            std::cout << "Stream creating\n";
            break;
        case PA_STREAM_READY:
            std::cout << "Stream ready\n";
            break;
        case PA_STREAM_FAILED:
            std::cout << "Stream failed\n";
            break;
        case PA_STREAM_TERMINATED:
            std::cout << "Stream term\n";
            break;
        default:
            std::cout << "Stream state: " << state << std::endl;
            break;
    }
}

static void
pa_stream_read_cb(pa_stream *stream, const size_t /*nbytes*/, void* userdata)
{
    // Careful when to pa_stream_peek() and pa_stream_drop()!
    // c.f. https://www.freedesktop.org/software/pulseaudio/doxygen/stream_8h.html#ac2838c449cde56e169224d7fe3d00824
    int16_t *data = nullptr;
    size_t actualbytes = 0;
    if (pa_stream_peek(stream, (const void**)&data, &actualbytes) != 0) {
        std::cerr << "Failed to peek at stream data\n";
        return;
    }

    if (data == nullptr && actualbytes == 0) {
        // No data in the buffer, ignore.
        return;
    }
    else if (data == nullptr && actualbytes > 0) {
        // Hole in the buffer. We must drop it.
        if (pa_stream_drop(stream) != 0) {
            std::cerr << "Failed to drop a hole! (Sounds weird, doesn't it?)\n";
            return;
        }
    }

    // process data
    //std::cout << ">> " << actualbytes << " bytes\n";
    Pulse* pulse = static_cast<Pulse*>(userdata);
    pulse->addData(data, actualbytes);

    if (pa_stream_drop(stream) != 0) {
        std::cerr << "Failed to drop data after peeking.\n";
    }
}

static void
pa_server_info_cb(pa_context *ctx, const pa_server_info *info, void* userdata)
{
#   ifdef DEBUG
    std::cout << "Default sink: " << info->default_sink_name << std::endl;
#   endif
    Pulse* pulse = static_cast<Pulse*>(userdata);
    pulse->serverInfo(info);
}

static void
pa_context_notify_cb(pa_context *ctx, void* userdata)
{
    const pa_context_state state = pa_context_get_state(ctx);
    Pulse* pulse = static_cast<Pulse*>(userdata);
    pulse->ctxNotify(state);
}



Pulse::Pulse(GMainContext* gctx)
{
    m_loop = pa_glib_mainloop_new(gctx); // pa_mainloop_new();
    pa_mainloop_api* api = pa_glib_mainloop_get_api(m_loop); // pa_mainloop_get_api(loop);
    m_ctx = pa_context_new(api, "padump");
    pa_context_set_state_callback(m_ctx, &pa_context_notify_cb, this);
    if (pa_context_connect(m_ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0) {
        std::cerr << "PA connection failed.\n";
        return;
    }
    //pa_mainloop_run(loop, nullptr);
}

Pulse::~Pulse()
{
    if (m_stream) {
        pa_stream_disconnect(m_stream);
        m_stream = nullptr;
    }
    if (m_ctx) {
        pa_context_disconnect(m_ctx);
        m_ctx = nullptr;
    }
    if (m_loop) {
        pa_glib_mainloop_free(m_loop);
        m_loop = nullptr;
    }
}

void
Pulse::ctxNotify(const pa_context_state state)
{
    switch (state) {
        case PA_CONTEXT_CONNECTING:
            std::cout << "Context connecting\n";
            break;
        case PA_CONTEXT_AUTHORIZING:
            std::cout << "Context auth\n";
            break;
        case PA_CONTEXT_SETTING_NAME:
            std::cout << "Context setting\n";
            break;
        case PA_CONTEXT_READY:
            //std::cout << "Context ready\n";
            pa_context_get_server_info(m_ctx, &pa_server_info_cb, this);
            break;
        case PA_CONTEXT_FAILED:
            std::cout << "Context failed\n";
            break;
        case PA_CONTEXT_TERMINATED:
            std::cout << "Context term\n";
            break;
        default:
            std::cout << "Context state: " << state << std::endl;
            break;
    }

}

uint8_t
Pulse::getChannels()
{
    return m_channels;
}

void
Pulse::setChannels(uint8_t channels)
{
    m_channels = channels;
}

uint32_t
Pulse::getSamplePerSec()
{
    return m_samplePerSec;
}

void
Pulse::setSamplePerSec(uint32_t samples)
{
    m_samplePerSec = samples;
}

pa_sample_format
Pulse::getFormat()
{
    return m_format;
}

void
Pulse::setFormat(pa_sample_format format)
{
    m_format = format;
}

PulseIn::PulseIn(GMainContext* ctx)
: Pulse{ctx}
{
}

void
PulseIn::serverInfo(const pa_server_info *info)
{
    pa_sample_spec spec;
    spec.format = m_format;
    spec.rate = m_samplePerSec;
    spec.channels = m_channels;
    // Use pa_stream_new_with_proplist instead?
    m_stream = pa_stream_new(m_ctx, "output monitor", &spec, nullptr);

    pa_stream_set_state_callback(m_stream, &pa_stream_notify_cb, this);
    pa_stream_set_read_callback(m_stream, &pa_stream_read_cb, this);

    std::string monitor_name(info->default_sink_name);
    monitor_name += ".monitor";
    if (pa_stream_connect_record(m_stream, monitor_name.c_str(), nullptr, PA_STREAM_NOFLAGS) != 0) {
        std::cerr << "connection fail\n";
        return;
    }
    std::cout << "Connected to " << monitor_name << std::endl;
}

void
PulseIn::addData(int16_t *data, size_t actualbytes)
{
    std::vector<int16_t> block;
    auto ptr = std::make_shared<std::vector<int16_t>>();
    size_t addsize = actualbytes / sizeof(int16_t);
    ptr->resize(addsize);
    std::copy(data, data + addsize, ptr->begin());
    m_data.push_back(ptr);
}


ChunkedArray<int16_t>
PulseIn::read()
{
    ChunkedArray<int16_t> read(m_channels);
    size_t sum{}, cnt{};
    while (true) {
        auto next = m_data.pop_front();
        if (!next) {
            break;
        }
        sum += next->size();
        ++cnt;
        read.add(next);
    }
#   ifdef DEBUG
    std::cout << "Pulse::read cnt " << cnt << " sum " << sum << std::endl;
#   endif
    return read;
}
