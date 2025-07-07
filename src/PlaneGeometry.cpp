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

#include <iostream>

#include "PlaneGeometry.hpp"
#include "PlaneContext.hpp"
#include "config.h"
//
//
//     Y
//     |   Z
//     |  /back(rows)
//     | /
//     |/diff = step
//     O______X
//    /
//   /front(rows)
//  -Z
//
PlaneGeometry::PlaneGeometry(PlaneContext *_ctx)
: ctx{_ctx}
, lastms{}
{
    //std::cout << "n: " << n << " n²: " << n*n << std::endl;
    build();
}

void
PlaneGeometry::advance(gint64 time)
{
    if (m_startTime == -1) {
        m_startTime = time;
    }
    gint32 ms = static_cast<gint32>(((time - m_startTime) / TIMESCALE) % TIMESCALE);
    m_frontAlpha = static_cast<float>(TIMESCALE - ms) / static_cast<float>(TIMESCALE);
    m_backAlpha = static_cast<float>(ms) / static_cast<float>(TIMESCALE);
    float step = getStep();
    if (ms < lastms) {      // remove old row at front add add at back
        m_frontRow = rows.front();
        rows.pop_front();
        rows.push_back(m_backRow);
        auto pRow = psc::mem::make_active<Row>(ctx, PLANE_TILES);
        if (auto lRow = pRow.lease())  {
            float zp = getZat(0.0f);    // we will reposition so this does not matter
            if (!m_pulseCtx) {
                Glib::RefPtr<Glib::MainContext> ctx = Glib::MainContext::get_default();
                m_pulseCtx = std::make_shared<psc::snd::PulseCtx>(ctx);
            }
            if (!m_pulseIn) {
                psc::snd::PulseFormat fmt;
                m_pulseIn = std::make_shared<psc::snd::PulseIn>(m_pulseCtx, fmt);
            }
            if (!m_fft) {
                auto windowFunction = std::make_shared<HammingWindow2k>();
                m_fft = std::make_shared<Fft2k1k>(windowFunction);
                m_fft->calibrate(100.0);
            }
            auto data = m_pulseIn->read();
            auto spec = m_fft->execute(data);
            std::vector<float> values;
            if (m_scaleMode == "O") {
                values = spec->adjustLog(PLANE_TILES, m_audioUsageRate, m_scale, m_keepSum);
            }
            else {
                values = spec->adjustLin(PLANE_TILES, m_audioUsageRate, m_scale, m_keepSum);
            }
            lRow->build(m_backRow, zp, Z_MIN, step, values);
        }
        m_backRow = pRow;
    }
    uint32_t z = static_cast<float>(PLANE_TILES-1);
    float timeFract = static_cast<float>(ms) / static_cast<float>(TIMESCALE);
    if (auto lRow = m_frontRow.lease()) {
        float zp = getZat(static_cast<float>(z) + timeFract);
        lRow->setScalePos(X_OFFS, 0.0f, zp, 1.0);
    }
    --z;
    for (auto& pRow : rows) {
        if (auto lRow = pRow.lease()) {
            float zp = getZat(static_cast<float>(z) + timeFract);
            lRow->setScalePos(X_OFFS, 0.0f, zp, 1.0);
        }
        --z;
    }
    if (auto lRow = m_backRow.lease()) {
        float zp = getZat(static_cast<float>(z) + timeFract);
        lRow->setScalePos(X_OFFS, 0.0f, zp, 1.0);
    }
    lastms = ms;
}


psc::mem::active_ptr<Row>
PlaneGeometry::getFrontRow()
{
    return m_frontRow;
}

psc::mem::active_ptr<Row>
PlaneGeometry::getBackRow()
{
    return m_backRow;
}

float
PlaneGeometry::getFrontAlpha()
{
    return m_frontAlpha;
}

float
PlaneGeometry::getBackAlpha()
{
    return m_backAlpha;
}

float
PlaneGeometry::getStep()
{
    return STEP;
}

float
PlaneGeometry::getZat(float index)
{
    float step = getStep();
    auto z = Z_MIN + step * index;
    return z;
}

std::list<psc::gl::aptrGeom2>
PlaneGeometry::getMidRows()
{
    std::list<psc::gl::aptrGeom2> list;
    for (auto& row : rows) {
        list.push_back(row);
    }
    return list;
}

void
PlaneGeometry::build()
{
    psc::mem::active_ptr<Row> last;
    float step = getStep();
    for (uint32_t z = 0; z < PLANE_TILES; ++z) {
        float zp = getZat(static_cast<float>(z));
        auto pRow = psc::mem::make_active<Row>(ctx, PLANE_TILES);
        if (z == 0) {
            m_backRow = pRow;
        }
        else if (z == PLANE_TILES - 1) {
            m_frontRow = pRow;
        }
        else {
            rows.push_back(pRow);
        }
        if (auto lRow = pRow.lease()) {
            std::vector<float> data;
            data.resize(PLANE_TILES);
            lRow->build(last, zp, Z_MIN, step, data);
        }
        last = pRow;
    }
}


double
PlaneGeometry::getScale()
{
    return m_scale;
}

void
PlaneGeometry::setScale(double scale)
{
#   ifdef DEBUG
    std::cout << "PlaneGeometry::setScale " << scale << std::endl;
#   endif
    m_scale = scale;
}

bool
PlaneGeometry::isKeepSum()
{
    return m_keepSum;
}

void
PlaneGeometry::setKeepSum(bool keepSum)
{
    m_keepSum = keepSum;
}


std::string
PlaneGeometry::getScaleMode()
{
    return m_scaleMode;
}

void
PlaneGeometry::setScaleMode(const std::string& scaleMode)
{
    m_scaleMode = scaleMode;
}

double
PlaneGeometry::getAudioUsageRate()
{
    return m_audioUsageRate;
}

void
PlaneGeometry::setAudioUsageRate(double useRate)
{
    m_audioUsageRate = useRate;
}

std::shared_ptr<psc::snd::PulseCtx>
PlaneGeometry::getPulseContext()
{
    return m_pulseCtx;
}
