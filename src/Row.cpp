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

#include "Row.hpp"
#include "PlaneGeometry.hpp"

Row::Row(GeometryContext *_ctx, uint32_t _n)
: psc::gl::Geom2(GL_LINES, _ctx)
, m_n(_n)
{
}


Position
Row::get(uint32_t i)
{
    return pos[i];
}

void
Row::build2(const psc::mem::active_ptr<Row>& aPrev, float z, float min, float step)
{
    setScalePos(PlaneGeometry::X_OFFS, 0.0f, z, 1.0f);

    Color color(0.15f, 1.0f, 0.15f);
    auto lPrev = aPrev.lease();
    for (uint32_t x = 0; x < m_n; ++x) {
        auto last = 0.0f;
        if (lPrev) {
            Position p = lPrev->get(x);   // as we want to connect to previous need to store coord here as well
            last = p.y;
        }
        float xp = min + step * x;
    	float h = drand48();
		float yp = last + h;
        yp = std::min(yp, 4.5f);
        yp = std::max(yp, 0.0f);
        float zp = 0.0f;
        Position p(xp, yp, zp);
        addPoint(&p, &color, nullptr, nullptr);
        pos.push_back(p);
        if (lPrev) {
            Position p = lPrev->get(x);   // as we want to connect to previous need to store coord here as well
            p.z -= step;
            addPoint(&p, &color, nullptr, nullptr);
        }
    }
    for (unsigned int x = 0; x < m_n; ++x) {
        int i1 = x;
        int i2 = x + m_n;
        int i3 = (x+1);
        if (lPrev) {
            addIndex(i1, i2);
        }
        if (x < m_n-1) {
            addIndex(i1, i3);
        }
    }
    if (lPrev) {
        for (uint32_t x = 1; x < m_n; ++x) {
            int i1 = m_n + (x-1);
            int i2 = m_n + (x);
            addIndex(i1, i2);         // avoid open lines at end
        }
    }
    create_vao();
}


float
Row::getXat(uint32_t x, float step)
{
    return PlaneGeometry::Z_MIN + step * static_cast<float>(x);
}

void
Row::build(const psc::mem::active_ptr<Row>& prev, float z, float min, float step)
{
    setScalePos(PlaneGeometry::X_OFFS, 0.0f, z, 1.0f);

    Color color(0.15f, 1.0f, 0.15f);
    auto lPrev = prev.lease();
    for (uint32_t x = 0; x < m_n; ++x) {
        float xp = getXat(x, step);
//		float yp = 0.0f;
//		int d = std::abs((int)x - i);
//		if (d <= 3) {
//			float dh = h;
//			for (int j = 0; j < d; ++j) {
//				dh = dh / 2.0f;
//			}
//			yp += dh;
//		}
        float preVal{};
        if (lPrev) {
            for (int32_t j = -2; j <= 2; ++j) {
                auto i = std::max(j + static_cast<int>(x), 0);
                i = std::min(i, static_cast<int>(m_n - 1));
                Position p = lPrev->get(i);   // as we want to connect to previous need to store coord here as well
                preVal += p.y;
            }
        }
        preVal /= 5.0f;
        float yp = preVal + (static_cast<float>(drand48()) - (preVal / (MAX_Y))) * 2.0f;
        yp = std::min(yp, MAX_Y);
        yp = std::max(yp, MIN_Y);
        float zp = 0.0f;
        Position p(xp, yp, zp);
        addPoint(&p, &color, nullptr, nullptr);
        pos.emplace_back(std::move(p));
    }
    if (lPrev) {
        for (uint32_t x = 0; x < m_n; ++x) {
            Position p = lPrev->get(x);   // as we want to connect to previous need to store coord here as well
            p.z += step;
            addPoint(&p, &color, nullptr, nullptr);
        }
    }
    for (uint32_t x = 0; x < m_n; ++x) {
        uint32_t i1 = x;                 // this is index for pos
        uint32_t i2 = x + m_n;           // this is the index for previous
        uint32_t i3 = (x+1);             // this is the index for neighbor
        if (lPrev) {
            addIndex(i1, i2);       // connect pos & previous
        }
        if (x < m_n-1) {            // if not last
            addIndex(i1, i3);       // connect pos & next
        }
    }
    if (lPrev) {
        for (uint32_t x = 1; x < m_n; ++x) {
            int i1 = m_n + (x-1);
            int i2 = m_n + (x);
            addIndex(i1, i2);         // avoid open lines at end
        }
    }
    create_vao();
}
