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

#include <glibmm.h>
#include <sigc++/sigc++.h>
#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>

#include <ConcurrentCollections.hpp>

#include "ChunkedArray.hpp"

namespace psc::snd
{

class PulseStream;

class PulseCtx
{
public:
    PulseCtx(const Glib::RefPtr<Glib::MainContext>& ctx);
    explicit PulseCtx(const PulseCtx& orig) = delete;
    virtual ~PulseCtx();

    void ctxNotify(const pa_context_state state);
    void init(PulseStream* stream);

    void serverInfo(const pa_server_info *info);
    //using type_server_info = sigc::signal<void(const pa_server_info *info)>;
    //type_server_info signal_server_info()
    //{
    //    return m_server_info;
    //}
    // this is the preferable method to stop using the ctx
    //   but be aware of that this results in additional callbacks
    //     -> it is a bad idea to immediately destroy the object
    void drain();
    void disconnect();
    pa_context *getContext();

protected:
    Glib::RefPtr<Glib::MainContext> m_glibCtx;
    pa_glib_mainloop* m_loop{};
    pa_context *m_ctx{};
    //type_server_info m_server_info;
    std::vector<PulseStream*> m_notifyStreams;
    std::vector<std::shared_ptr<PulseStream>> m_streams;
    const pa_server_info* m_info{};
};

struct PulseFormat
{
    uint8_t channels{1u};
    uint32_t samplePerSec{44100u};
    pa_sample_format format{PA_SAMPLE_S16NE}; // use NE native endian or LE

    pa_sample_spec toSpec();
};

enum class PulseStreamState
{
      creating
    , ready
    , failed
    , terminated
    , disconnected
};

struct PulseStreamNotify
{
    virtual void streamNotify(PulseStreamState state) = 0;
};

class PulseStream
{
protected:
    PulseStream(const std::shared_ptr<PulseCtx>& pulseContext, PulseFormat& format);
    explicit PulseStream(const PulseStream& orig) = delete;
    virtual ~PulseStream();
    void notifyListener(PulseStreamState state);

public:
    void disconnect();
    void streamNotify(const pa_stream_state state);
    virtual void serverInfo(const pa_server_info *info) = 0;
    bool isReady();
    virtual void onStreamReady();
    void addStreamListener(PulseStreamNotify* notify);
protected:
    std::shared_ptr<PulseCtx> m_pulseContext;
    PulseFormat m_format;
    pa_stream *m_stream{};
    bool m_ready{false};
    std::vector<PulseStreamNotify*> m_streamListener;
};

class PulseIn
: public PulseStream
{
public:
    PulseIn(const std::shared_ptr<PulseCtx>& pulseContext, PulseFormat& format);
    explicit PulseIn(const PulseIn& orig) = delete;
    virtual ~PulseIn() = default;

    void serverInfo(const pa_server_info *info) override;
    void addData(int16_t *data, size_t actualbytes) ;

    ChunkedArray<int16_t> read();

protected:
    TListConcurrent<std::shared_ptr<std::vector<int16_t>>> m_data;

};

class AudioSource
{
public:
    AudioSource()  = default;
    explicit AudioSource(const AudioSource& orig) = delete;
    virtual ~AudioSource() = default;
    float getVolume(); // volume adapted into a 0..100 range
    void setVolume(float volume);

    virtual void requestData(size_t samples, int16_t* buffer) = 0;
protected:
    float m_volume{10.0};
};

class SineSource
: public AudioSource
{
public:
    SineSource() = default;
    explicit SineSource(const SineSource& orig) = delete;
    virtual ~SineSource() = default;

    float getFrequency()
    {
        return m_freq;
    }
    void setFrequency(float freq)
    {
        m_freq = freq;
    }

    void requestData(size_t samples, int16_t* buffer) override;
private:
    float m_freq{441.0};
    size_t m_idx{};
};

class PulseOut
: public PulseStream
{
public:
    PulseOut(const std::shared_ptr<PulseCtx>& pulseContext, PulseFormat& format, const std::shared_ptr<AudioSource>& source);
    explicit PulseOut(const PulseOut& orig) = delete;
    virtual ~PulseOut() = default;
    // this is the preferable method to stop using the stream
    //   but be aware that this results in additional callbacks
    //     -> TODO it is a bad idea to immediately destroy the object
    //             wait for it? would require to stash the instance away ...
    void drain();
    // use either a short buffer to write or the suggested maximum
    void setWriteLong(bool writeLong);
    void serverInfo(const pa_server_info *info) override;
    void requestData(size_t length);
    void onStreamReady() override;

protected:
    std::shared_ptr<AudioSource> m_source;
    uint32_t m_latency{};
    uint32_t m_process_time{};
    pa_channel_map m_channel_map{};
    bool m_channel_map_set{false};
    bool m_drained{false};
    bool m_writeLong{true};
};

} /* namespace psc::snd */
