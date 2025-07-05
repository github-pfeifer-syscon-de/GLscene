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
#include <epoxy/gl.h>
#include <stdlib.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/normal.hpp> // triangleNormal


#include "Row.hpp"
#include "PlaneGeometry.hpp"

Row::Row(GeometryContext *_ctx, uint32_t _n)
: psc::gl::Geom2(GL_TRIANGLES, _ctx)
, m_n(_n)
{
}


Position
Row::get(uint32_t i)
{
    return m_pos[i];
}


float
Row::getXat(uint32_t x, float step)
{
    return PlaneGeometry::Z_MIN + step * static_cast<float>(x);
}

float
Row::getY(const psc::mem::active_ptr<Row>& prev, uint32_t x)
{
//    float preVal{};
//    auto lPrev = prev.lease();
//    if (lPrev) {
//        for (int32_t j = -2; j <= 2; ++j) {
//            auto i = std::max(j + static_cast<int>(x), 0);
//            i = std::min(i, static_cast<int>(m_n - 1));
//            Position p = lPrev->get(i);
//            preVal += p.y;
//        }
//        preVal /= 5.0f;
//    }
    auto l = lrand48() % m_n;
    float yp = l == x ? 2.0f : 0.0f;//preVal + (static_cast<float>(drand48()) - (preVal / MAX_Y)) * 2.0f;    //
    yp = std::min(yp, MAX_Y);
    yp = std::max(yp, MIN_Y);
    return yp;
}

void
Row::build(const psc::mem::active_ptr<Row>& prev
          , float z
          , float min
          , float step
          , const std::vector<float>& values)
{
    setScalePos(PlaneGeometry::X_OFFS, 0.0f, z, 1.0f);

    Color colorGreen(0.15f, 0.6f, 0.15f);
    Color colorRed(0.6f, 0.15f, 0.15f);
    auto lPrev = prev.lease();
    std::vector<Position> prevs;
    for (uint32_t x = 0; x < m_n; ++x) {
        float xp = getXat(x, step);
        //float yp = getY(prev, x);
        float yp = values[x];
        float zp = 0.0f;
        Position p(xp, yp, zp);
        m_pos.emplace_back(std::move(p));
        Position prevPos;
        if (lPrev) {
            prevPos = lPrev->get(x);   // as we want to connect to previous need to store coord here as well
            prevPos.z += step;
        }
        else {
            prevPos.x = xp;
            prevPos.y = 0.0f;
            prevPos.z = step;
        }
        prevs.emplace_back(std::move(prevPos));
    }
    for (uint32_t x = 0; x < m_n; ++x) {
        auto pos = m_pos[x];
        auto pre = prevs[x];
        Color color(glm::mix(colorGreen, colorRed, pos.y / 4.5f));
        Position next{};
        if (x < m_n-1) {
            next = m_pos[x+1];
        }

        auto norm = glm::triangleNormal(pos, next, pre);
        addPoint(&pos, &color, &norm, nullptr);
        addPoint(&pre, &color, &norm, nullptr);
    }

    for (uint32_t x = 0; x < m_n; ++x) {
        uint32_t i1 = x * 2;                 // this is index for pos
        uint32_t i2 = x * 2 + 1;             // this is the index for previous
        uint32_t i3 = (x + 1) * 2;           // this is the index for neighbor
        uint32_t i4 = (x + 1) * 2 + 1;       // this is the index for previous  neighbor
        if (x < m_n-1) {            // if not last
            addIndex(i1, i2, i3);       // connect pos & next
            addIndex(i2, i4, i3);       // connect pos & neigb.
        }
    }
    create_vao();
}
