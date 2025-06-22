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


#include "TextContext.hpp"

TextContext::TextContext()
: m_color_location(0)
{
}


TextContext::~TextContext()
{
}

void TextContext::updateLocation()
{
    NaviContext::updateLocation();
    m_color_location = glGetUniformLocation(m_program, "color");
}



void
TextContext::setColor(Color &c)
{
    m_color = c;
    glUniform3fv(m_color_location, 1, &m_color[0]);
}

bool
TextContext::useNormal() {
    return FALSE;
}

bool
TextContext::useColor() {
    return FALSE;
}

bool
TextContext::useUV() {
    return TRUE;
}
