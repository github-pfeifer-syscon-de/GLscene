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

#include <vector>
#include <Geom2.hpp>

class Row
: public psc::gl::Geom2 {
public:
    Row(GeometryContext *_ctx, uint32_t _n);
    virtual ~Row() = default;
    void build(const psc::mem::active_ptr<Row>& prev, float z, float min, float step, const std::vector<float>& values);
    Position get(uint32_t i);

    static constexpr auto MAX_Y{4.5f};
    static constexpr auto MIN_Y{0.0f};
protected:
    float getXat(uint32_t x, float step);
    float getY(const psc::mem::active_ptr<Row>& prev, uint32_t x);

private:
    const uint32_t m_n;
    std::vector<Position> m_pos;
};

