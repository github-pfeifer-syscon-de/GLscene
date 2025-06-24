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
    gint32 ms = (time / TIMESCALE) % TIMESCALE;
    m_frontAlpha = static_cast<float>(TIMESCALE - ms) / TIMESCALE;
    m_backAlpha = static_cast<float>(ms) / TIMESCALE;
    //std::cout << "Font " << m_frontAlpha
    //          << " back " << m_backAlpha << std::endl;
    float step = getStep();
    if (ms < lastms) {      // remove old row at back add new at front
        m_frontRow = rows.front();
        rows.pop_front();
        rows.push_back(m_backRow);
        auto pRow = psc::mem::make_active<Row>(ctx, PLANE_TILES);
        auto& last = rows.back();
        if (auto lRow = pRow.lease())  {
            lRow->build(last, Z_MAX-step, Z_MIN, step);
        }
        m_backRow = pRow;
    }
    uint32_t z = PLANE_TILES-1;
    if (auto lRow = m_frontRow.lease()) {
        float zp = getZat(static_cast<float>(z) + (static_cast<float>(ms) / static_cast<float>(TIMESCALE)));
        lRow->setScalePos(X_OFFS, 0.0f, zp, 1.0);
    }
    --z;
    for (auto& pRow : rows) {
        if (auto lRow = pRow.lease()) {
            float zp = getZat(static_cast<float>(z) + (static_cast<float>(ms) / static_cast<float>(TIMESCALE)));
            lRow->setScalePos(X_OFFS, 0.0f, zp, 1.0);
        }
        --z;
    }
    if (auto lRow = m_backRow.lease()) {
        float zp = getZat(static_cast<float>(z) + (static_cast<float>(ms) / static_cast<float>(TIMESCALE)));
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
            //ctx->addGeometry(pRow);
            rows.push_back(pRow);
        }
        if (auto lRow = pRow.lease()) {
            lRow->build(last, zp, Z_MIN, step);
        }
        last = pRow;
    }
}


