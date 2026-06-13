/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2026 RPf
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

#include <ObjLoader.hpp>
#include <glibmm.h>

// test basic load support
static bool
load()
{
    auto file = Gio::File::create_for_path(Glib::get_home_dir())
                                ->get_child("blendr/test.obj");
    if (!file->query_exists()) {
        std::cout << "file " << file->get_path() << " does not exist\n"
                  <<  "   check source -> bring your own model -> check output" << std::endl;
        return false;
    }
    psc::gl::ObjLoader objLoader;
    try {
        objLoader.load(file);
        std::cout << "items " <<  objLoader.getItems() << std::endl;
        for (size_t i = 0; i < objLoader.getItems(); ++i) {
            std::cout << "   item " <<  objLoader.getItem(i)->getInfo() << std::endl;
        }
    }
    catch (const psc::gl::ObjException& exc) {
        std::cout << "Exception " << exc.what() << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    std::setlocale(LC_ALL, "");      // make locale dependent, and make glib accept u8 const !!!
    Glib::init();

    if (!load()) {
        return 1;
    }
    return 0;
}