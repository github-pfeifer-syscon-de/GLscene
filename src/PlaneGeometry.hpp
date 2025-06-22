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

#include "PlaneContext.hpp"
#include "Row.hpp"

class PlaneGeometry  {
public:
    PlaneGeometry(PlaneContext *_ctx);
    virtual ~PlaneGeometry();
    float getStep();
    void build();
    void advance();
    static constexpr auto PLANE_TILES{20u};   // was 40
    static constexpr auto Z_MIN{-10.0f};
    static constexpr auto Z_MAX{10.0f};
    static constexpr auto X_OFFS{15.0f};     // use 0.0f for ceneted
    static constexpr auto STEP{(Z_MAX-Z_MIN) / static_cast<float>(PLANE_TILES-1)};
    static constexpr auto TIMESCALE{1000l};
protected:
    float getZat(float z);
private:
    PlaneContext *ctx;
    std::list<psc::mem::active_ptr<Row>> rows;
    int32_t lastms;
};

