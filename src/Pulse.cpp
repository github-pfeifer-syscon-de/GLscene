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
#include <psc_format.hpp>

#include "Pulse.hpp"
#include "config.h"

namespace psc::snd
{

static inline const char *pa_strnull(const char *x) {
    return x ? x : "(null)";
}

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
#   ifdef DEBUG
    printf("stream_drain_complete %p\n", userdata);
#   endif
    auto pulse = static_cast<PulseStream*>(userdata);
    pulse->disconnect();
}


static void
get_sample_info_callback(pa_context *ctx, const pa_sample_info *inf, int is_last, void *userdata)
{
#   ifdef DEBUG
    std::cout << "get_sample_info_callback"
              << " inf " << inf
              << " is_last " << is_last << std::endl;
#   endif
    if (is_last < 0) {
        std::cout << psc::fmt::format("Failed to get sample information: {}", pa_strerror(pa_context_errno(ctx)))
                  << std::endl;
        return;
    }
    if (is_last) {
        return;
    }
    char chrChanMap[PA_CHANNEL_MAP_SNPRINT_MAX];
    char chrSpec[PA_SAMPLE_SPEC_SNPRINT_MAX];
    if (pa_sample_spec_valid(&inf->sample_spec)) {
        pa_channel_map_snprint(chrChanMap, sizeof(chrChanMap), &inf->channel_map);
        pa_sample_spec_snprint(chrSpec, sizeof(chrSpec), &inf->sample_spec);
    }
    else {
        strlcpy(chrChanMap, "n/a", sizeof(chrChanMap));
        strlcpy(chrSpec, "n/a", sizeof(chrSpec));
    }
    float balance = pa_cvolume_get_balance(&inf->volume, &inf->channel_map);
    std::cout << psc::fmt::format("Sample #{}", inf->index) << std::endl;
    std::cout << psc::fmt::format("\tName: {}", inf->name) << std::endl;
    std::cout << psc::fmt::format("\tSample Specification: {}", chrSpec) << std::endl;
    std::cout << psc::fmt::format("\tChannel Map: {}", chrChanMap) << std::endl;
    char chrChanVol[PA_CVOLUME_SNPRINT_VERBOSE_MAX];
    pa_cvolume_snprint_verbose(chrChanVol, sizeof(chrChanVol), &inf->volume, &inf->channel_map, true);
    std::cout << psc::fmt::format("\tVolume: {}", chrChanVol) << std::endl;
    std::cout << psc::fmt::format("\t        balance {:0.2f}", balance) << std::endl;
    double duration = (double) inf->duration/1000000.0;
    std::cout << psc::fmt::format("\tDuration: {:0.1f}s", duration) << std::endl;
    char chrSize[PA_BYTES_SNPRINT_MAX];
    pa_bytes_snprint(chrSize, sizeof(chrSize), inf->bytes);
    std::cout << psc::fmt::format("\tSize: {}", chrSize) << std::endl;
    std::cout << psc::fmt::format("\tLazy: {}", (inf->lazy ? "yes" : "no")) << std::endl;
    std::cout << psc::fmt::format("\tFilename: {}", (inf->filename ? inf->filename : "n/a")) << std::endl;
    char* pl = pa_proplist_to_string_sep(inf->proplist, "\n\t\t");
    std::cout << psc::fmt::format("\tProperties:\n\t\t{}", pl) << std::endl;
    pa_xfree(pl);
}

static const std::string
get_device_port_type(unsigned int type) {
    switch (type) {
    case PA_DEVICE_PORT_TYPE_UNKNOWN:
        return "Unknown";
    case PA_DEVICE_PORT_TYPE_AUX:
        return "Aux";
    case PA_DEVICE_PORT_TYPE_SPEAKER:
        return "Speaker";
    case PA_DEVICE_PORT_TYPE_HEADPHONES:
        return "Headphones";
    case PA_DEVICE_PORT_TYPE_LINE:
        return "Line";
    case PA_DEVICE_PORT_TYPE_MIC:
        return "Mic";
    case PA_DEVICE_PORT_TYPE_HEADSET:
        return "Headset";
    case PA_DEVICE_PORT_TYPE_HANDSET:
        return "Handset";
    case PA_DEVICE_PORT_TYPE_EARPIECE:
        return "Earpiece";
    case PA_DEVICE_PORT_TYPE_SPDIF:
        return "SPDIF";
    case PA_DEVICE_PORT_TYPE_HDMI:
        return "HDMI";
    case PA_DEVICE_PORT_TYPE_TV:
        return "TV";
    case PA_DEVICE_PORT_TYPE_RADIO:
        return "Radio";
    case PA_DEVICE_PORT_TYPE_VIDEO:
        return "Video";
    case PA_DEVICE_PORT_TYPE_USB:
        return "USB";
    case PA_DEVICE_PORT_TYPE_BLUETOOTH:
        return "Bluetooth";
    case PA_DEVICE_PORT_TYPE_PORTABLE:
        return "Portable";
    case PA_DEVICE_PORT_TYPE_HANDSFREE:
        return "Handsfree";
    case PA_DEVICE_PORT_TYPE_CAR:
        return "Car";
    case PA_DEVICE_PORT_TYPE_HIFI:
        return "HiFi";
    case PA_DEVICE_PORT_TYPE_PHONE:
        return "Phone";
    case PA_DEVICE_PORT_TYPE_NETWORK:
        return "Network";
    case PA_DEVICE_PORT_TYPE_ANALOG:
        return "Analog";
    }
    return psc::fmt::format( "Unknown port type %u", type);
}

static void
get_sink_info_callback(pa_context *ctx, const pa_sink_info *inf, int is_last, void *userdata)
{
    if (is_last < 0) {
        std::cout << psc::fmt::format("Failed to get sink information: {}", pa_strerror(pa_context_errno(ctx)))
                  << std::endl;
        return;
    }
    if (is_last) {
        return;
    }

    char chrSpec[PA_SAMPLE_SPEC_SNPRINT_MAX];
    char *sample_spec = pa_sample_spec_snprint(chrSpec, sizeof(chrSpec), &inf->sample_spec);
    const char* state = "n/a";
    switch (inf->state) {
    case PA_SINK_INIT:
        state = "Init";
        break;
    case PA_SINK_RUNNING:
        state = "Running";
        break;
    case PA_SINK_IDLE:
        state = "Idle";
        break;
    case PA_SINK_SUSPENDED:
        state = "Suspended";
        break;
    case PA_SINK_UNLINKED:
        state = "Unlinked";
        break;
    case PA_SINK_INVALID_STATE:
        state = "Invalid state";
        break;
    }

    char chrChanMap[PA_CHANNEL_MAP_SNPRINT_MAX];
    char *channel_map = pa_channel_map_snprint(chrChanMap, sizeof(chrChanMap), &inf->channel_map);
    float volume_balance = pa_cvolume_get_balance(&inf->volume, &inf->channel_map);

    char chrCVolume[PA_CVOLUME_SNPRINT_VERBOSE_MAX];
    pa_cvolume_snprint_verbose(chrCVolume, sizeof(chrCVolume), &inf->volume, &inf->channel_map, inf->flags & PA_SINK_DECIBEL_VOLUME);
    char chrVolume[PA_VOLUME_SNPRINT_VERBOSE_MAX];
    pa_volume_snprint_verbose(chrVolume, sizeof(chrVolume), inf->base_volume, inf->flags & PA_SINK_DECIBEL_VOLUME);
    std::cout << psc::fmt::format("Sink #{}", inf->index) <<std::endl;
    std::cout << psc::fmt::format("\tState: {}", state) << std::endl;
    std::cout << psc::fmt::format("\tName: {}", inf->name) <<std::endl;
    std::cout << psc::fmt::format("\tDescription: {}", pa_strnull(inf->description)) <<std::endl;
    std::cout << psc::fmt::format("\tDriver: {}", pa_strnull(inf->driver)) <<std::endl;
    std::cout << psc::fmt::format("\tSample Specification: {}", sample_spec) <<std::endl;
    std::cout << psc::fmt::format("\tChannel Map: {}", channel_map) <<std::endl;
    std::cout << psc::fmt::format("\tOwner Module: {}", inf->owner_module) <<std::endl;
    std::cout << psc::fmt::format("\tMute: {}", (inf->mute ? "yes" : "no")) <<std::endl;
    std::cout << psc::fmt::format("\tVolume: {}", chrCVolume) <<std::endl;
    std::cout << psc::fmt::format("\t        balance {:0.2f}", volume_balance) <<std::endl;
    std::cout << psc::fmt::format("\tBase Volume: {}", chrVolume) <<std::endl;
    std::cout << psc::fmt::format("\tMonitor Source: {}", pa_strnull(inf->monitor_source_name)) << std::endl;
    std::cout << psc::fmt::format("\tLatency: {:0.0f} usec, configured {:0.0f} usec", (double) inf->latency, (double) inf->configured_latency) <<std::endl;
    std::cout << psc::fmt::format("\tFlags: {}{}{}{}{}{}{}"
                                    , inf->flags & PA_SINK_HARDWARE ? "HARDWARE " : ""
                                    , inf->flags & PA_SINK_NETWORK ? "NETWORK " : ""
                                    , inf->flags & PA_SINK_HW_MUTE_CTRL ? "HW_MUTE_CTRL " : ""
                                    , inf->flags & PA_SINK_HW_VOLUME_CTRL ? "HW_VOLUME_CTRL " : ""
                                    , inf->flags & PA_SINK_DECIBEL_VOLUME ? "DECIBEL_VOLUME " : ""
                                    , inf->flags & PA_SINK_LATENCY ? "LATENCY " : ""
                                    , inf->flags & PA_SINK_SET_FORMATS ? "SET_FORMATS " : "") <<std::endl;
    char *pl = pa_proplist_to_string_sep(inf->proplist, "\n\t\t");
    std::cout << psc::fmt::format("\tProperties:\n\t\t{}", pl) <<std::endl;
    pa_xfree(pl);

    if (inf->ports) {
        pa_sink_port_info **p;

        std::cout << "\tPorts:" << std::endl;
        for (p = inf->ports; *p; p++) {
            const char* avail = "n/a";
            switch ((*p)->available) {
            case PA_PORT_AVAILABLE_UNKNOWN:
                avail = "availability unknown";
                break;
            case PA_PORT_AVAILABLE_YES:
                avail = "available";
                break;
            case PA_PORT_AVAILABLE_NO:
                avail = "not available";
                break;
            }
            std::cout << psc::fmt::format("\t\t{}: {} (type: {}, priority: {}{}{}, {})",
                    (*p)->name, (*p)->description, get_device_port_type((*p)->type),
                    (*p)->priority, (*p)->availability_group ? ", availability group: " : "",
                    (*p)->availability_group ? (*p)->availability_group : "", avail)
                    << std::endl;
        }
    }

    if (inf->active_port) {
        std::cout<< psc::fmt::format("\tActive Port: {}", inf->active_port->name) << std::endl;
    }
    if (inf->formats) {
        uint8_t j;

        std::cout << "\tFormats:" << std::endl;
        char chrFormat[PA_FORMAT_INFO_SNPRINT_MAX];
        for (j = 0; j < inf->n_formats; j++) {
            pa_format_info_snprint(chrFormat, sizeof(chrFormat), inf->formats[j]);
            std::cout << psc::fmt::format("\t\t{}", chrFormat)
                     << std::endl;
        }
    }
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
PulseCtx::listSinks()
{
    // these functions are from https://github.com/pulseaudio/pulseaudio/blob/master/src/utils/pactl.c#L1526
    pa_context_get_sink_info_list(m_ctx, get_sink_info_callback, this);
}

void
PulseCtx::listSamples()
{
    pa_context_get_sample_info_list(m_ctx, get_sample_info_callback, this);
}

void
PulseCtx::ctxNotify(const pa_context_state state)
{
    switch (state) {
        case PA_CONTEXT_CONNECTING:
#           ifdef DEBUG
            std::cout << "Context connecting" << std::endl;
#           endif
            break;
        case PA_CONTEXT_AUTHORIZING:
#           ifdef DEBUG
            std::cout << "Context auth" << std::endl;
#           endif
            break;
        case PA_CONTEXT_SETTING_NAME:
#           ifdef DEBUG
            std::cout << "Context setting" << std::endl;
#           endif
            break;
        case PA_CONTEXT_READY:
#           ifdef DEBUG
            std::cout << "Context ready" << std::endl;
#           endif
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
#           ifdef DEBUG
            std::cout << "Stream creating" << std::endl;
#           endif
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
                std::cout << psc::fmt::format("Stream failed: {}", pa_strerror(pa_context_errno(ctx)))
                          << std::endl;
            }
            break;
        case PA_STREAM_TERMINATED:
            pulseState = PulseStreamState::terminated;
            m_ready = false;
#           ifdef DEBUG
            std::cout << "Stream term" << std::endl;
#           endif
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
    // this gives just some info about
    const pa_buffer_attr *a;
#   ifdef DEBUG
    std::cout << "Stream ready." << std::endl;
#   endif
    auto ctx = m_pulseContext->getContext();
    if (!(a = pa_stream_get_buffer_attr(m_stream))) {
        std::cout << psc::fmt::format("pa_stream_get_buffer_attr() failed: {}", pa_strerror(pa_context_errno(ctx))) << std::endl;
    }
    else {
#       ifdef DEBUG
        std::cout << psc::fmt::format("Buffer metrics: maxlength={}, tlength={}, prebuf={}, minreq={}"
                                      , a->maxlength, a->tlength, a->prebuf, a->minreq) << std::endl;
#       endif
    }
    char sst[PA_SAMPLE_SPEC_SNPRINT_MAX];
    pa_sample_spec_snprint(sst, sizeof(sst), pa_stream_get_sample_spec(m_stream));
#   ifdef DEBUG
    std::cout << psc::fmt::format("Using sample spec '{}'", sst) << std::endl;
#   endif
    char cmt[PA_CHANNEL_MAP_SNPRINT_MAX];
    pa_channel_map_snprint(cmt, sizeof(cmt), pa_stream_get_channel_map(m_stream));
#   ifdef DEBUG
    std::cout << psc::fmt::format("Channel map '{}'", cmt) << std::endl;

    std::cout << psc::fmt::format("Connected to device {} ({}, {}suspended).",
            pa_stream_get_device_name(m_stream),
            pa_stream_get_device_index(m_stream),
            pa_stream_is_suspended(m_stream) ? "" : "not ") << std::endl;
#   endif
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
#   ifdef DEBUG
    std::cout << "Connected to " << monitor_name << std::endl;
#   endif
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

void
PulseIn::onStreamReady()
{
    PulseStream::onStreamReady();   // still show infos

    //auto channelMap = pa_stream_get_channel_map(m_stream);



//
//    float volume_balance = pa_cvolume_get_balance(&i->volume, &i->channel_map);
//
//    pa_json_encoder_add_member_raw_json(encoder, "volume", pa_cvolume_to_json_object(&i->volume, &i->channel_map, i->flags & PA_SINK_DECIBEL_VOLUME));
//    pa_json_encoder_add_member_double(encoder, "balance", volume_balance, 2);
//    pa_json_encoder_add_member_raw_json(encoder, "base_volume", pa_volume_to_json_object(i->base_volume, i->flags & PA_SINK_DECIBEL_VOLUME));
//
	//
    //char volInf[80];
    //pa_channel_map map;
    //pa_channel_map_init_stereo(&map);

    //pa_context_set_sink_input_volume

    //
    //std::cout << "Vol %s" << pa_cvolume_snprint(volInf, sizeof(volInf), &cv)
    //          << std::endl;
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
AudioGenerator::requestData(size_t samples, int16_t* buffer)
{
#   ifdef DEBUG
    printf("AudioSource::requestData sampels %ld idx %ld\n", samples, m_idx);
#   endif
    float volFactor{static_cast<float>(std::numeric_limits<int16_t>::max()) * m_volume / 100.0f};
    switch (m_shape) {
        case AudioShape::Sine: {
            float tFactor{2.0f * static_cast<float>(M_PI) * m_freq / 44100.0f};
            for (size_t i = 0; i < samples; ++i) {
                float t = static_cast<float>(m_idx + i) * tFactor;
                float v = std::sinf(t) * volFactor;
                buffer[i] = static_cast<int16_t>(v);
            }
        }
        case AudioShape::Square: {
            size_t tFactor{static_cast<size_t>(44100.0f / m_freq)};
            size_t tFactor2{tFactor / 2};
            for (size_t i = 0; i < samples; ++i) {
                auto t = m_idx + i;
                float v = (t % tFactor < tFactor2 ? 1.0f : -1.0f) * volFactor;
                buffer[i] = static_cast<int16_t>(v);
            }
        }
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
#   ifdef DEBUG
    std::cout << "Stream started." << std::endl;
#   endif
}


static void
stream_event_callback(pa_stream *s, const char *name, pa_proplist *pl, void *userdata)
{
#   ifdef DEBUG
    char *t = pa_proplist_to_string_sep(pl, ", ");
    std::cout << psc::fmt::format("Got event '{}', properties '{}'", name, t)
              << std::endl;
    pa_xfree(t);
#   endif
}

static void
stream_buffer_attr_callback(pa_stream *strm, void *userdata)
{
#   ifdef DEBUG
    std::cout << "Stream buffer attributes changed." << std::endl;
#   endif
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
        std::cout << psc::fmt::format("pa_stream_new() failed: {}", pa_strerror(pa_context_errno(ctx)))
                  << std::endl;
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
        std::cout << psc::fmt::format("pa_stream_connect_playback() failed: {}", pa_strerror(pa_context_errno(ctx)))
                  << std::endl;
        return;
    }
#   ifdef DEBUG
    std::cout << "Connected to " << device << std::endl;
#   endif
}

void
PulseOut::onStreamReady()
{
    PulseStream::onStreamReady();   // show infos
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
        std::cout << psc::fmt::format("pa_stream_begin_write() failed: {}", pa_strerror(pa_context_errno(ctx)))
                  << std::endl;
        return;
    }
    auto data16 = reinterpret_cast<int16_t*>(data);
    auto samples = toWriteBytes / sizeof(int16_t);
    m_source->requestData(samples, data16);
    if (pa_stream_write(m_stream, data, toWriteBytes, nullptr, 0, PA_SEEK_RELATIVE) < 0) {
        auto ctx = m_pulseContext->getContext();
        std::cout << psc::fmt::format("pa_stream_write() failed: {}", pa_strerror(pa_context_errno(ctx)))
                  << std::endl;
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
        std::cout << psc::fmt::format("pa_stream_drain(): {}", pa_strerror(pa_context_errno(ctx)))
                  << std::endl;
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

