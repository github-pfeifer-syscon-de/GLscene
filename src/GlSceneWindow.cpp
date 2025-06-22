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

GlSceneWindow::GlSceneWindow(GlSceneApp* application)
: Gtk::ApplicationWindow()
{
    set_title("Flow");
    auto pix = Gdk::Pixbuf::create_from_resource(application->get_resource_base_path() + "/glscene.png");
    set_icon(pix);

    GlPlaneView *plane = new GlPlaneView(application);
    auto planeView = Gtk::manage(new NaviGlArea(plane));
    add(*planeView);
    //set_decorated(FALSE);
    set_default_size(640, 480);
    show_all_children();
}


void
GlSceneWindow::on_action_preferences()
{
    //Gtk::Dialog *dlg = m_monglView.monitors_config();
    //dlg->set_transient_for(*this);
    //dlg->run();
}