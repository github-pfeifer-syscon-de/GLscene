/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4;  coding: utf-8; -*-  */
/*
 * Copyright (C) 2025 RPf <gpl3@pfeifer-syscon.de>
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
#include <memory>
#include <Plot.hpp>

class GlSceneWindow;

class SpectrumPlot
: public psc::ui::Plot
{
public:
    SpectrumPlot(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, GlSceneWindow* sceneWindow);
    explicit SpectrumPlot(const SpectrumPlot& orig) = delete;
    virtual ~SpectrumPlot() = default;

    static void show(GlSceneWindow* sceneWindow, const std::shared_ptr<psc::ui::PlotView>& func);

protected:
    Gtk::CheckButton* m_active;
    Gtk::Scale* m_scale;
};

