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


#include "SmokeContext.hpp"

SmokeContext::SmokeContext()
: NaviContext()
{
}


void
SmokeContext::updateLocation()
{
    m_screen = glGetUniformLocation(m_program, "u_resolution");
    m_time = glGetUniformLocation(m_program, "u_time");
    NaviContext::updateLocation();
}

void
SmokeContext::setResolution(UV &uv)
{
    glUniform2fv(m_screen, 1, &uv[0]);
}

void
SmokeContext::setTime(float time)
{
    glUniform1f(m_time, time);
}

bool
SmokeContext::useNormal()
{
    return FALSE;
}

bool
SmokeContext::useColor()
{
    return FALSE;
}

bool
SmokeContext::useUV()
{
    return TRUE;
}
