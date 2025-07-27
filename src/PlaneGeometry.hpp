/*
 * Copyright (C) 2018 rpf
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

#include <gtkmm.h>
#include <epoxy/gl.h>
#include <list>
#include <Geom2.hpp>
#include <KeyConfig.hpp>

#include "PlaneContext.hpp"
#include "Row.hpp"
#include "Fft.hpp"
#include "Pulse.hpp"

class AudioListener
{
public:
    virtual void notifyAudio(const std::vector<double>& fft) = 0;
};

class PlaneGeometry
{
public:
    PlaneGeometry(PlaneContext *_ctx, const std::shared_ptr<KeyConfig>& keyConfig);
    virtual ~PlaneGeometry() = default;
    float getStep();
    virtual void build() = 0;
    virtual void advance(gint64 time) = 0;
    virtual psc::mem::active_ptr<Row> getFrontRow() = 0;
    std::list<psc::gl::aptrGeom2> getMidRows();
    virtual psc::mem::active_ptr<Row> getBackRow() = 0;
    virtual float getFrontAlpha() = 0;
    virtual float getBackAlpha() = 0;
    static constexpr auto PLANE_TILES{40u};   // was 40
    static constexpr auto Z_MIN{-10.0f};
    static constexpr auto Z_MAX{10.0f};
    static constexpr auto X_OFFS{PlaneContext::showSmokeShader ? 12.0f : 0.0f};
    static constexpr auto STEP{(Z_MAX-Z_MIN) / static_cast<float>(PLANE_TILES-1)};
    static constexpr auto TIMESCALE{500l};
    double getScale();
    void setScale(double scale);
    bool isKeepSum();
    void setKeepSum(bool keepSum);
    std::string getScaleMode();
    void setScaleMode(const std::string& scaleMode);
    double getAudioUsageRate();
    void setAudioUsageRate(double useRate);
    void saveConfig();
    void restoreConfig();
    std::vector<double> getAudioAsArray();
    std::shared_ptr<psc::snd::PulseCtx> getPulseContext();
    void addAudioListener(AudioListener* audioListener);
    void removeAudioListener(AudioListener* audioListener);
protected:
    float getZat(float z);
    std::vector<float> buildValues();

    PlaneContext *ctx;
    std::shared_ptr<KeyConfig> m_keyConfig;
    std::list<psc::mem::active_ptr<Row>> rows;
    int32_t lastms;
    gint64 m_startTime{-1l};
    std::shared_ptr<Fft512> m_fft;
    std::shared_ptr<Spectrum<512>> m_spec;
    std::shared_ptr<psc::snd::PulseCtx> m_pulseCtx;
    std::shared_ptr<psc::snd::PulseIn> m_pulseIn;
    double m_scale{1.0};
    bool m_keepSum{false};
    std::string m_scaleMode;
    double m_audioUsageRate{0.5};
    AudioListener* m_audioListener{nullptr};
};

// this is expected to look like we are traveling forward
class ForwardPlaneGeometry
: public PlaneGeometry
{
public:
    ForwardPlaneGeometry(PlaneContext *_ctx, const std::shared_ptr<KeyConfig>& keyConfig);
    virtual ~ForwardPlaneGeometry() = default;

    void advance(gint64 time) override;
    psc::mem::active_ptr<Row> getFrontRow() override;
    psc::mem::active_ptr<Row> getBackRow() override;
    float getFrontAlpha() override;
    float getBackAlpha() override;
    void build() override;



protected:
    psc::mem::active_ptr<Row> m_frontRow;
    psc::mem::active_ptr<Row> m_backRow;
    float m_frontAlpha;
    float m_backAlpha;

};

// and the other way around
class BackwardPlaneGeometry
: public PlaneGeometry
{
public:
    BackwardPlaneGeometry(PlaneContext *_ctx, const std::shared_ptr<KeyConfig>& keyConfig);
    virtual ~BackwardPlaneGeometry() = default;

    void advance(gint64 time) override;
    psc::mem::active_ptr<Row> getFrontRow() override;
    psc::mem::active_ptr<Row> getBackRow() override;
    float getFrontAlpha() override;
    float getBackAlpha() override;
    void build() override;

protected:
    psc::mem::active_ptr<Row> m_frontRow;
    psc::mem::active_ptr<Row> m_backRow;
    float m_frontAlpha;
    float m_backAlpha;

};