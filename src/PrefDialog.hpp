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

#include "Pulse.hpp"

class GlSceneWindow;
class PlaneGeometry;

class PrefDialog
: public Gtk::Dialog
, public psc::snd::PulseStreamNotify
{
public:
    PrefDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, PlaneGeometry* planeGeometry);
    virtual ~PrefDialog() = default;

    static void show(GlSceneWindow* sceneWindow);
    void streamNotify(psc::snd::PulseStreamState state) override;
protected:
    void signalGenToggel();
private:
    Gtk::Scale* m_maxLevel;
    Gtk::CheckButton* m_keepSum;
    Gtk::ToggleButton* m_signalGen;
    Gtk::Scale* m_frequency;
    PlaneGeometry* m_planeGeometry;
    std::shared_ptr<psc::snd::SineSource> m_source;
    std::shared_ptr<psc::snd::PulseOut> m_out;
    Gtk::Scale* m_volume;
    Gtk::Scale* m_freqUsage;
    Gtk::ComboBoxText* m_freqMode;
};

