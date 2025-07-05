/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
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


#include "PrefDialog.hpp"
#include "GlSceneWindow.hpp"
#include "PlaneGeometry.hpp"

PrefDialog::PrefDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, PlaneGeometry* planeGeometry)
: Gtk::Dialog(cobject)
, m_planeGeometry{planeGeometry}
{
    builder->get_widget("maxLevel", m_maxLevel);
    m_maxLevel->set_value(m_planeGeometry->getScale());
    m_maxLevel->signal_value_changed().connect([this] {
        m_planeGeometry->setScale(getMaxLevel());
    });
}

double
PrefDialog::getMaxLevel()
{
    return m_maxLevel->get_value();
}


void
PrefDialog::show(GlSceneWindow* sceneWindow)
{
    auto refBuilder = Gtk::Builder::create();
    try {
        auto appl = sceneWindow->getApplicaiton();
        refBuilder->add_from_resource(appl->get_resource_base_path() + "/prefs-dlg.ui");
        PrefDialog* prefDialog;
        refBuilder->get_widget_derived("pref-dlg", prefDialog, sceneWindow->getPlaneGeometry());
        if (prefDialog) {
            auto planeGeometry = sceneWindow->getPlaneGeometry();
            prefDialog->set_transient_for(*sceneWindow);
            int ret = prefDialog->run();
            if (ret == Gtk::ResponseType::RESPONSE_OK) {
                sceneWindow->getKeyConfig()->setDouble(GlSceneWindow::MAIN_SECTION, GlSceneWindow::SCALE_KEY, prefDialog->getMaxLevel());
                sceneWindow->saveConfig();
            }
            else {
                double maxLevel = sceneWindow->getKeyConfig()->getDouble(GlSceneWindow::MAIN_SECTION, GlSceneWindow::SCALE_KEY, 1.0);
                planeGeometry->setScale(maxLevel);
            }
            delete prefDialog;
        }
        else {
            std::cerr << "PrefDialog::show no pref-dlg" << std::endl;
        }
    }
    catch (const Glib::Error& ex) {
        std::cerr << "PrefDialog::show error loading pref-dlg.ui " << ex.what() << std::endl;
    }
    return;

}