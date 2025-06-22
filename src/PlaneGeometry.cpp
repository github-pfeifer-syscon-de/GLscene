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
PlaneGeometry::PlaneGeometry(PlaneContext *_ctx)
: ctx{_ctx}
, lastms{}
{
    //std::cout << "n: " << n << " n²: " << n*n << std::endl;
    build();
}


PlaneGeometry::~PlaneGeometry()
{
    //for (std::list<Row *>::iterator p = rows.begin(); p != rows.end(); ++p) {
    //    Row *pRow = *p;
    //    delete pRow;
    //}
    //rows.clear();
}

void
PlaneGeometry::advance()
{
    gint64 time = g_get_monotonic_time();    // the promise is this does not get screwed up by time adjustments
    gint32 ms = (time / TIMESCALE) % TIMESCALE;
    float step = getStep();
    if (ms < lastms) {      // remove old row at back add new at front
        auto pRow = rows.front();
        pRow.resetAll();
        rows.pop_front();
        pRow = psc::mem::make_active<Row>(ctx, PLANE_TILES);
        ctx->addGeometry(pRow);
        auto& last = rows.back();
        if (auto lRow = pRow.lease())  {
            lRow->build(last, Z_MAX-step, Z_MIN, step);
        }
        rows.push_back(pRow);
    }
    uint32_t z = PLANE_TILES-1;
    for (auto& pRow : rows) {
        if (auto lRow = pRow.lease()) {
            float zp = getZat(static_cast<float>(z) + (static_cast<float>(ms) / static_cast<float>(TIMESCALE)));
            lRow->setScalePos(X_OFFS, 0.0f, zp, 1.0);
        }
        --z;
    }
    lastms = ms;
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

void
PlaneGeometry::build()
{
    psc::mem::active_ptr<Row> last;
    float step = getStep();
    for (uint32_t z = 0; z < PLANE_TILES; ++z) {
        float zp = getZat(static_cast<float>(z));
        auto pRow = psc::mem::make_active<Row>(ctx, PLANE_TILES);
        ctx->addGeometry(pRow);
        if (auto lRow = pRow.lease()) {
            lRow->build(last, zp, Z_MIN, step);
        }
        rows.push_back(pRow);
        last = pRow;
    }
}


