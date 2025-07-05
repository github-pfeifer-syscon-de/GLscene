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

#include <gtkmm.h>
#include <iostream>
#include <exception>
#include <thread>
#include <future>

#include "GlSceneWindow.hpp"
#include "GlSceneApp.hpp"

GlSceneApp::GlSceneApp(int argc, char **argv)
: Gtk::Application(argc, argv, "de.pfeifer_syscon.glscene")
{

}

GlSceneApp::GlSceneApp(const GlSceneApp& orig)
{
}

void
GlSceneApp::on_activate()
{
    if (m_gltermAppWindow) {
    add_window(*m_gltermAppWindow);
    m_gltermAppWindow->present();
    }
}


void
GlSceneApp::on_action_quit()
{
    if (m_gltermAppWindow)  {
        m_gltermAppWindow->hide();
        delete m_gltermAppWindow;
        m_gltermAppWindow = nullptr;
    }

  // Not really necessary, when Gtk::Widget::hide() is called, unless
  // Gio::Application::hold() has been called without a corresponding call
  // to Gio::Application::release().
  quit();
}

void
GlSceneApp::on_startup()
{
    // Call the base class's implementation.
    Gtk::Application::on_startup();

    m_gltermAppWindow = new GlSceneWindow(this);
    // Add actions and keyboard accelerators for the application menu.
    add_action("quit", sigc::mem_fun(*this, &GlSceneApp::on_action_quit));
    set_accel_for_action("app.quit", "<Ctrl>Q");

    auto refBuilder = Gtk::Builder::create();
    try {
        refBuilder->add_from_resource(get_resource_base_path() + "/app-menu.ui");
    }
    catch (const Glib::Error& ex) {
        std::cerr << "GlSceneApp::on_startup(): " << ex.what() << std::endl;
        return;
    }

    auto object = refBuilder->get_object("appmenu");
    auto app_menu = Glib::RefPtr<Gio::MenuModel>::cast_dynamic(object);
    if (app_menu)
        set_app_menu(app_menu);
    else
        std::cerr << "GlSceneApp::on_startup(): No \"appmenu\" object in app_menu.ui"
                  << std::endl;
}

int main(int argc, char** argv)
{
    auto app = GlSceneApp(argc, argv);

    return app.run();
}