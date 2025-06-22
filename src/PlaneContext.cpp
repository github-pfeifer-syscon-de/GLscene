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


#include "PlaneContext.hpp"


PlaneContext::PlaneContext()
: NaviContext()
, m_screen(0)
, m_lineWidth(0)
{
}


PlaneContext::~PlaneContext()
{
}

void
PlaneContext::updateLocation()
{
    m_screen = glGetUniformLocation(m_program, "screen");

    m_lineWidth = glGetUniformLocation(m_program, "lineWidth");
    NaviContext::updateLocation();
}

void PlaneContext::setResolution(UV &uv)
{
    glUniform2fv(m_screen, 1, &uv[0]);
}

void PlaneContext::setLineWidth(float lineWidth)
{
    glUniform1f(m_lineWidth, lineWidth);
}

bool
PlaneContext::useNormal()
{
    return false;
}

bool
PlaneContext::useColor()
{
    return true;   // FALSE
}

bool
PlaneContext::useUV()
{
    return false;   // TRUE
}
