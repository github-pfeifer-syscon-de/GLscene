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
    m_planView = new GlPlaneView(this);
    auto planeView = Gtk::manage(new NaviGlArea(m_planView));
    add(*planeView);
    //set_decorated(FALSE);
    set_default_size(640, 480);
    show_all_children();
}

 GlSceneWindow::~GlSceneWindow()
 {
     if (m_planView) {
         delete m_planView;
         m_planView = nullptr;
     }
 }

void
GlSceneWindow::on_action_preferences()
{
    PrefDialog::show(this);
}

Gtk::Application*
GlSceneWindow::getApplicaiton()
{
    return m_application;
}

PlaneGeometry*
GlSceneWindow::getPlaneGeometry()
{
    return m_planView->getPlaneGeometry();
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
    m_keyConfig->setDouble(GlSceneWindow::MAIN_SECTION, GlSceneWindow::SCALE_KEY, planGeom->getScale());
    m_keyConfig->setBoolean(GlSceneWindow::MAIN_SECTION, GlSceneWindow::KEEP_SUM_KEY, planGeom->isKeepSum());
    m_keyConfig->setDouble(GlSceneWindow::MAIN_SECTION, GlSceneWindow::FREQ_USE_KEY, planGeom->getAudioUsageRate());
    m_keyConfig->setString(GlSceneWindow::MAIN_SECTION, GlSceneWindow::FREQ_SCALE_MODE_KEY, planGeom->getScaleMode());

    m_keyConfig->saveConfig();
}

void
GlSceneWindow::restoreConfig()
{
    auto planGeom = m_planView->getPlaneGeometry();
    planGeom->setScale(m_keyConfig->getDouble(GlSceneWindow::MAIN_SECTION, GlSceneWindow::SCALE_KEY, 1.0));
    planGeom->setKeepSum(m_keyConfig->getBoolean(GlSceneWindow::MAIN_SECTION, GlSceneWindow::KEEP_SUM_KEY, false));
    planGeom->setAudioUsageRate(m_keyConfig->getDouble(GlSceneWindow::MAIN_SECTION, GlSceneWindow::FREQ_USE_KEY, 0.5));
    planGeom->setScaleMode(m_keyConfig->getString(GlSceneWindow::MAIN_SECTION, GlSceneWindow::FREQ_SCALE_MODE_KEY, "L"));
}