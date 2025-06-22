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
#include <stdlib.h>

#include "Bump.hpp"

Bump::Bump()
{
}

Bump::Bump(Bump &other)
{
    d = other.d;
    h = other.h;
    r = other.r;
    xm = other.xm;
    ym = other.ym;
}

Bump::~Bump()
{
}

void 
Bump::advance() 
{
    d += r ? 0.05 : -0.05;
    if (r) {
        if (d > h) {
            r = false;
        }
    }
    else {
        if (d < 0.0f) {
            init();
        }
    }
}

void
Bump::init()
{
    h = (drand48() * 2.0f + 1.5f);
    r = true;
    d = 0.0f;
    xm = (8.0f-drand48() * 16.0f);
    ym = (8.0f-drand48() * 16.0f);
}

float
Bump::get(float x, float y)
{
    float xd = x - xm;
    float yd = y - ym;
    float xy2 = (xd * xd + yd * yd);
    float d22 = 2.0f * d * d;
    if (xy2 < d22) {
        return (d22 - xy2) * 0.2f ;
    }
    return 0.0f;
}
