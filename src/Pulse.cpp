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
#include <cmath>
#include <cstdio>

#include "Pulse.hpp"
#include "config.h"

namespace psc::snd
{

static void
pa_stream_notify_cb(pa_stream *stream, void* userdata)
{
    const pa_stream_state state = pa_stream_get_state(stream);
    PulseStream* pulse = static_cast<PulseStream*>(userdata);
    pulse->streamNotify(state);
}

static void
pa_stream_read_cb(pa_stream *stream, const size_t /*nbytes*/, void* userdata)
{
    // Careful when to pa_stream_peek() and pa_stream_drop()!
    // c.f. https://www.freedesktop.org/software/pulseaudio/doxygen/stream_8h.html#ac2838c449cde56e169224d7fe3d00824
    int16_t *data = nullptr;
    size_t actualbytes = 0;
    if (pa_stream_peek(stream, (const void**)&data, &actualbytes) != 0) {
        std::cerr << "Failed to peek at stream data" << std::endl;
        return;
    }

    if (data == nullptr && actualbytes == 0) {
        // No data in the buffer, ignore.
        return;
    }
    else if (data == nullptr && actualbytes > 0) {
        // Hole in the buffer. We must drop it.
        if (pa_stream_drop(stream) != 0) {
            std::cerr << "Failed to drop a hole! (Sounds weird, doesn't it?)" << std::endl;
            return;
        }
    }

    // process data
    //std::cout << ">> " << actualbytes << " bytes" << std::endl;
    auto pulse = static_cast<PulseIn*>(userdata);
    pulse->addData(data, actualbytes);

    if (pa_stream_drop(stream) != 0) {
        std::cerr << "Failed to drop data after peeking." << std::endl;
    }
}

static void
pa_server_info_cb(pa_context *ctx, const pa_server_info *info, void* userdata)
{
#   ifdef DEBUG
    std::cout << "Default sink: " << info->default_sink_name << std::endl;
#   endif
    auto pulseCtx = static_cast<PulseCtx*>(userdata);
    pulseCtx->serverInfo(info);
}

static void
pa_context_notify_cb(pa_context *ctx, void* userdata)
{
    const pa_context_state state = pa_context_get_state(ctx);
    auto pulse = static_cast<PulseCtx*>(userdata);
    pulse->ctxNotify(state);
}

static void
context_drain_complete(pa_context*ctx, void *userdata)
{
    auto pulse = static_cast<PulseCtx*>(userdata);
    pulse->disconnect();
}

static void
stream_drain_complete(pa_stream*strm, int success, void *userdata)
{
    printf("stream_drain_complete %p\n", userdata);
    auto pulse = static_cast<PulseStream*>(userdata);
    pulse->disconnect();
}

PulseCtx::PulseCtx(const Glib::RefPtr<Glib::MainContext>& glibCtx)
: m_glibCtx{glibCtx}
{
    m_notifyStreams.reserve(16);
}

void
PulseCtx::init(PulseStream* stream)
{
    if (!m_loop) {
        GMainContext* c_ctx = m_glibCtx->gobj();
        m_loop = pa_glib_mainloop_new(c_ctx); // pa_mainloop_new();
    }
    if (!m_ctx) {
        pa_mainloop_api* api = pa_glib_mainloop_get_api(m_loop); // pa_mainloop_get_api(loop);
        m_ctx = pa_context_new(api, "PulseCtx");
        pa_context_set_state_callback(m_ctx, &pa_context_notify_cb, this);
        if (pa_context_connect(m_ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0) {
            std::cerr << "PA connection failed." << std::endl;
            return;
        }
    }
    if (m_info) {
        stream->serverInfo(m_info);
    }
    else {
        m_notifyStreams.push_back(stream);
    }
}

PulseCtx::~PulseCtx()
{
    disconnect();
}

void
PulseCtx::drain()
{
    if (!pa_context_drain(m_ctx, context_drain_complete, this)) {
        disconnect();
    }
}

void
PulseCtx::disconnect()
{
    if (m_ctx) {
        pa_context_disconnect(m_ctx);
        pa_context_unref(m_ctx);
        m_ctx = nullptr;
    }
    if (m_loop) {
        pa_glib_mainloop_free(m_loop);
        m_loop = nullptr;
    }
}

pa_context*
PulseCtx::getContext()
{
    return m_ctx;
}

void
PulseCtx::ctxNotify(const pa_context_state state)
{
    switch (state) {
        case PA_CONTEXT_CONNECTING:
            std::cout << "Context connecting" << std::endl;
            break;
        case PA_CONTEXT_AUTHORIZING:
            std::cout << "Context auth" << std::endl;
            break;
        case PA_CONTEXT_SETTING_NAME:
            std::cout << "Context setting" << std::endl;
            break;
        case PA_CONTEXT_READY:
            //std::cout << "Context ready" << std::endl;
            pa_context_get_server_info(m_ctx, &pa_server_info_cb, this);
            break;
        case PA_CONTEXT_FAILED:
            std::cout << "Context failed" << std::endl;
            break;
        case PA_CONTEXT_TERMINATED:
            std::cout << "Context term" << std::endl;
            break;
        default:
            std::cout << "Context state: " << state << std::endl;
            break;
    }
}

void
PulseCtx::serverInfo(const pa_server_info *info)
{
    for (PulseStream* stream : m_notifyStreams) {
        stream->serverInfo(info);
    }
    m_notifyStreams.clear();
    m_info = info;
    //m_server_info.emit(info);
}


PulseStream::PulseStream(const std::shared_ptr<PulseCtx>& pulseContext, PulseFormat& format)
: m_pulseContext{pulseContext}
, m_format{format}
{
    m_streamListener.reserve(4);
}

PulseStream::~PulseStream()
{
    disconnect();
}

void
PulseStream::disconnect()
{
    //printf("PulseStream::disconnect %p strm %p\n", (void*)this, (void*)m_stream);
    if (m_stream) {
        notifyListener(PulseStreamState::disconnected); // since the there is no default notification
        pa_stream_disconnect(m_stream);
        pa_stream_unref(m_stream);
        m_stream = nullptr;
    }
    m_streamListener.clear();   // no more notifications beyond this point
}

void
PulseStream::streamNotify(const pa_stream_state state )
{
    PulseStreamState pulseState = PulseStreamState::failed;
    switch (state) {
        case PA_STREAM_CREATING:
            pulseState = PulseStreamState::creating;
            //std::cout << "Stream creating" << std::endl;
            break;
        case PA_STREAM_READY:
            pulseState = PulseStreamState::ready;
            m_ready = true;
            onStreamReady();
            //std::cout << "Stream ready" << std::endl;
            break;
        case PA_STREAM_FAILED:
            m_ready = false;
            {
                auto ctx = m_pulseContext->getContext();
                fprintf(stderr, "Stream failed: %s\n", pa_strerror(pa_context_errno(ctx)));
            }
            break;
        case PA_STREAM_TERMINATED:
            pulseState = PulseStreamState::terminated;
            m_ready = false;
            std::cout << "Stream term" << std::endl;
            break;
        default:
            std::cout << "Stream state: " << state << std::endl;
            break;
    }
    notifyListener(pulseState);
}

void
PulseStream::notifyListener(PulseStreamState pulseState)
{
    for (PulseStreamNotify* lsnr : m_streamListener) {
        lsnr->streamNotify(pulseState);
    }
}

bool
PulseStream::isReady()
{
    return m_stream && m_ready;
}

void
PulseStream::onStreamReady()
{
}

void
PulseStream::addStreamListener(PulseStreamNotify* notify)
{
    m_streamListener.push_back(notify);
}

pa_sample_spec
PulseFormat::toSpec()
{
    pa_sample_spec spec;
    spec.format = format;
    spec.rate = samplePerSec;
    spec.channels = channels;
    return spec;
}

PulseIn::PulseIn(const std::shared_ptr<PulseCtx>& pulseContext, PulseFormat& format)
: PulseStream{pulseContext, format}
{
    //pulseContext->signal_server_info()
    //        .connect(sigc::mem_fun(*this, &PulseIn::serverInfo));
    pulseContext->init(this);
}

void
PulseIn::serverInfo(const pa_server_info *info)
{
    pa_sample_spec spec = m_format.toSpec();
    // Use pa_stream_new_with_proplist instead?
    auto ctx = m_pulseContext->getContext();
    m_stream = pa_stream_new(ctx, "output monitor", &spec, nullptr);

    pa_stream_set_state_callback(m_stream, pa_stream_notify_cb, this);
    pa_stream_set_read_callback(m_stream, pa_stream_read_cb, this);

    std::string monitor_name(info->default_sink_name);
    monitor_name += ".monitor";
    if (pa_stream_connect_record(m_stream, monitor_name.c_str(), nullptr, PA_STREAM_NOFLAGS) != 0) {
        std::cerr << "connection fail" << std::endl;
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
    ChunkedArray<int16_t> read(m_format.channels);
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



float
AudioSource::getVolume()
{
    return m_volume;
}

void
AudioSource::setVolume(float volume)
{
    m_volume = volume;
}


void
SineSource::requestData(size_t samples, int16_t* buffer)
{
#   ifdef DEBUG
    printf("SineSource::requestData sampels %ld idx %ld\n", samples, m_idx);
#   endif
    for (size_t i = 0; i < samples; ++i) {
        float t = 2.0f * static_cast<float>(M_PI) * static_cast<float>(m_idx + i) * m_freq / 44100.0f;
        float v = std::sinf(t) * m_volume / 100.0f;
        buffer[i] = static_cast<int16_t>(v * std::numeric_limits<int16_t>::max());
    }
    m_idx += samples;
}

static void
stream_write_callback(pa_stream *s, size_t length, void *userdata)
{
    PulseOut* pulse = static_cast<PulseOut*>(userdata);
    pulse->requestData(length);
}


static void
stream_started_callback(pa_stream *s, void *userdata)
{
    fprintf(stderr, "Stream started.\n");
}


static void
stream_event_callback(pa_stream *s, const char *name, pa_proplist *pl, void *userdata)
{
    char *t = pa_proplist_to_string_sep(pl, ", ");
    fprintf(stderr, "Got event '%s', properties '%s'\n", name, t);
    pa_xfree(t);
}

static void
stream_buffer_attr_callback(pa_stream *s, void *userdata)
{
    fprintf(stderr, "Stream buffer attributes changed.\n");
}

PulseOut::PulseOut(const std::shared_ptr<PulseCtx>& pulseContext, PulseFormat& format, const std::shared_ptr<AudioSource>& source)
: PulseStream{pulseContext, format}
, m_source{source}
{
    //pulseContext->signal_server_info()
    //        .connect(sigc::mem_fun(*this, &PulseOut::serverInfo));
    pulseContext->init(this);
}


void
PulseOut::serverInfo(const pa_server_info *info)
{
#   ifdef DEBUG
    fprintf(stderr, "Connection established.\n");
#   endif
    pa_sample_spec spec = m_format.toSpec();

    auto ctx = m_pulseContext->getContext();
    if (!(m_stream = pa_stream_new(ctx, "PulseOut", &spec, m_channel_map_set ? &m_channel_map : nullptr))) {
        fprintf(stderr, "pa_stream_new() failed: %s\n", pa_strerror(pa_context_errno(ctx)));
        return;
    }

    pa_stream_set_state_callback(m_stream, pa_stream_notify_cb, this);
    pa_stream_set_write_callback(m_stream, stream_write_callback, this);
    //pa_stream_set_suspended_callback(m_stream, stream_suspended_callback, this);
    //pa_stream_set_moved_callback(m_stream, stream_moved_callback, this);
    //pa_stream_set_underflow_callback(m_stream, stream_underflow_callback, this);
    //pa_stream_set_overflow_callback(m_stream, stream_overflow_callback, this);
    pa_stream_set_started_callback(m_stream, stream_started_callback, this);
    pa_stream_set_event_callback(m_stream, stream_event_callback, this);
    pa_stream_set_buffer_attr_callback(m_stream, stream_buffer_attr_callback, this);

    pa_stream_flags_t flags = static_cast<pa_stream_flags_t>(0);
    pa_buffer_attr buffer_attr;
    if (m_latency > 0) {
        memset(&buffer_attr, 0, sizeof(buffer_attr));
        buffer_attr.tlength = m_latency;
        buffer_attr.minreq = m_process_time;
        buffer_attr.maxlength = (uint32_t) -1;
        buffer_attr.prebuf = (uint32_t) -1;
        buffer_attr.fragsize = m_latency;
        flags = static_cast<pa_stream_flags_t>(static_cast<uint32_t>(flags) | PA_STREAM_ADJUST_LATENCY);
    }

    auto device = pa_stream_get_device_name(m_stream);
    pa_volume_t volume = PA_VOLUME_NORM;
    pa_cvolume cv;
    int r;
    if ((r = pa_stream_connect_playback(m_stream, device, m_latency > 0 ? &buffer_attr : nullptr, flags, pa_cvolume_set(&cv, spec.channels, volume), NULL)) < 0) {
        auto ctx = m_pulseContext->getContext();
        fprintf(stderr, "pa_stream_connect_playback() failed: %s\n", pa_strerror(pa_context_errno(ctx)));
        return;
    }

    std::cout << "Connected to " << device << std::endl;
}

void
PulseOut::onStreamReady()
{
    const pa_buffer_attr *a;
    char cmt[PA_CHANNEL_MAP_SNPRINT_MAX], sst[PA_SAMPLE_SPEC_SNPRINT_MAX];

    fprintf(stderr, "Stream successfully created.\n");
    auto ctx = m_pulseContext->getContext();
    if (!(a = pa_stream_get_buffer_attr(m_stream))) {
        fprintf(stderr, "pa_stream_get_buffer_attr() failed: %s\n", pa_strerror(pa_context_errno(ctx)));
    }
    else {
        fprintf(stderr, "Buffer metrics: maxlength=%u, tlength=%u, prebuf=%u, minreq=%u\n", a->maxlength, a->tlength, a->prebuf, a->minreq);
    }

    fprintf(stderr, "Using sample spec '%s', channel map '%s'.\n",
            pa_sample_spec_snprint(sst, sizeof(sst), pa_stream_get_sample_spec(m_stream)),
            pa_channel_map_snprint(cmt, sizeof(cmt), pa_stream_get_channel_map(m_stream)));

    fprintf(stderr, "Connected to device %s (%u, %ssuspended).\n",
            pa_stream_get_device_name(m_stream),
            pa_stream_get_device_index(m_stream),
            pa_stream_is_suspended(m_stream) ? "" : "not ");
}

void
PulseOut::requestData(size_t length)
{
    if (m_drained) {
        return;     // no more writing
    }
    //printf("PulseOut::requestData length %ld\n", length);
    uint8_t* data{nullptr};
    size_t toWriteBytes{m_writeLong ? static_cast<size_t>(-1) : length};    // strange: we are requested to pass -1 to choose automatic sizing, this is definitely c-style
    if (pa_stream_begin_write(m_stream, reinterpret_cast<void**>(&data), &toWriteBytes) || data == nullptr) {
        auto ctx = m_pulseContext->getContext();
        fprintf(stderr, "pa_stream_begin_write() failed: %s\n", pa_strerror(pa_context_errno(ctx)));
        return;
    }
    auto data16 = reinterpret_cast<int16_t*>(data);
    auto samples = toWriteBytes / sizeof(int16_t);
    m_source->requestData(samples, data16);
    if (pa_stream_write(m_stream, data, toWriteBytes, nullptr, 0, PA_SEEK_RELATIVE) < 0) {
        auto ctx = m_pulseContext->getContext();
        fprintf(stderr, "pa_stream_write() failed: %s\n", pa_strerror(pa_context_errno(ctx)));
    }
}

void
PulseOut::drain()
{
    if (m_drained) {
        return;
    }
    //printf("PulseOut::drain %p\n", (void*)this);
    m_drained = true;
    pa_operation *operation;
    if (!(operation = pa_stream_drain(m_stream, stream_drain_complete, this))) {
        auto ctx = m_pulseContext->getContext();
        fprintf(stderr, "pa_stream_drain(): %s\n", pa_strerror(pa_context_errno(ctx)));
        disconnect();
        return;
    }
    pa_operation_unref(operation);
}

void
PulseOut::setWriteLong(bool writeLong)
{
    m_writeLong = writeLong;
}



} /* namespace psc::snd */

