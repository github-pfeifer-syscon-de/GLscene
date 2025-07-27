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
#include <thread>
#include <future>

#include <gtkmm.h>

#include "GlSceneWindow.hpp"
#include "GlSceneApp.hpp"
#include "PrefDialog.hpp"

GlSceneWindow::GlSceneWindow(GlSceneApp* application)
: Gtk::ApplicationWindow()
, m_application{application}
{
    set_title("Flow");
    auto pix = Gdk::Pixbuf::create_from_resource(application->get_resource_base_path() + "/glscene.png");
    set_icon(pix);

    m_keyConfig = std::make_shared<KeyConfig>("glscene.conf");
    add_action("preferences", sigc::mem_fun(this, &GlSceneWindow::on_action_preferences));
    add_action("plot", sigc::mem_fun(this, &GlSceneWindow::on_action_plot));
    m_planView = new GlPlaneView(this);
    auto planeView = Gtk::manage(new NaviGlArea(m_planView));
    add(*planeView);
    //set_decorated(FALSE);
    set_default_size(480, 320);
    show_all_children();
}

GlSceneWindow::~GlSceneWindow()
{
    if (m_planView) {
        delete m_planView;
        m_planView = nullptr;
    }
}

class PlotHamm
: public psc::ui::PlotFunction
{
public:
    PlotHamm(double xMin, double xMax)
    : PlotFunction(xMin, xMax)
    {
    }
    explicit PlotHamm(const PlotHamm& orig) = delete;
    virtual ~PlotHamm() = default;

    // to check hamming functions
    double calculate(double x)
    {
        //return x * x;
        //auto HAMMING_OFFS{0.53836};
        //auto HAMMING_FACTOR{0.46164};
        auto end = 2.0 * M_PI / xAxis.getMax();
        //return HAMMING_OFFS - (HAMMING_FACTOR * std::cos( (x * end)));

        return 1.0 - 0.85 * std::cos(x * end);
    }
};

PlotAudio::PlotAudio(const std::shared_ptr<PlaneGeometry>& geom, double upperFreq)
: PlotDiscrete(std::vector<double>())
, m_upperFreq{upperFreq}
, m_geom{geom}
{
    geom->addAudioListener(this);
}

PlotAudio::~PlotAudio()
{
    m_geom->removeAudioListener(this);
}

void
PlotAudio::notifyAudio(const std::vector<double>& values)
{
    if (m_plotDrawing && m_plotDrawing->isActive()) {
#       ifdef DEBUG
        std::cout << "PlotAudio::notifyAudio " << values.size() << std::endl;
#       endif
        m_hzPerSlot = m_upperFreq / static_cast<double>(values.size());
        m_values = values;
        m_plotDrawing->refresh();
    }
}

Glib::ustring
PlotAudio::getLabel(size_t idx)
{
    size_t markAt = static_cast<size_t>(MARK_HZ / m_hzPerSlot);
    if (idx % markAt == 0) {
        size_t hz = static_cast<size_t>(static_cast<double>(idx) * m_hzPerSlot);
        return Glib::ustring::sprintf("%dk", hz / 1000);
    }
    return "";
}



void
GlSceneWindow::on_action_preferences()
{
    PrefDialog::show(this);
}

void
GlSceneWindow::on_action_plot()
{
    //auto func{std::make_shared<PlotHamm>(0.0, 100.0)};
    auto geom = m_planView->getPlaneGeometry();
    auto func{std::make_shared<PlotAudio>(geom, 22050.0)};
    psc::ui::Plot::show(this, func);
}

Gtk::Application*
GlSceneWindow::getApplicaiton()
{
    return m_application;
}

std::shared_ptr<PlaneGeometry>
GlSceneWindow::getPlaneGeometry()
{
    return m_planView->getPlaneGeometry();
}

GlPlaneView*
GlSceneWindow::getPlaneView()
{
    return m_planView;
}

std::shared_ptr<KeyConfig>
GlSceneWindow::getKeyConfig()
{
    return m_keyConfig;
}

void
GlSceneWindow::saveConfig()
{
    auto planGeom = m_planView->getPlaneGeometry();
    planGeom->saveConfig();
    m_keyConfig->setString(GlSceneWindow::MAIN_SECTION, GlSceneWindow::MOVEMENT_KEY, m_planView->getMovement());

    m_keyConfig->saveConfig();
}

void
GlSceneWindow::restoreConfig()
{
    // keep this as first as the remaing depends on this
    m_planView->setMovement(m_keyConfig->getString(GlSceneWindow::MAIN_SECTION, GlSceneWindow::MOVEMENT_KEY, "F"));
    auto planGeom = m_planView->getPlaneGeometry();
    planGeom->restoreConfig();
}