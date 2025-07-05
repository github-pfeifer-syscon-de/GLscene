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

#pragma once

#include <gtkmm.h>
#include <Font2.hpp>
#include <Text2.hpp>
#include <NaviGlArea.hpp>
#include <Scene.hpp>
#include <Geom2.hpp>

#include "PlaneContext.hpp"
#include "PlaneGeometry.hpp"
#include "SmokeContext.hpp"

class GlSceneWindow;

class GlPlaneView
: public Scene {
public:
    GlPlaneView(GlSceneWindow* glSceneWindow);
    virtual ~GlPlaneView();
    Position getIntialPosition() override;
    Rotational getInitalAngleDegree() override;
    guint32 getAnimationMs() override;
    gboolean init_shaders(Glib::Error &error) override;
    void init(Gtk::GLArea *glArea) override;
    void unrealize() override;
    void draw(Gtk::GLArea *glArea, Matrix &proj, Matrix &view) override;
    psc::gl::aptrGeom2 on_click_select(GdkEventButton* event, float mx, float my) override;
    PlaneGeometry* getPlaneGeometry();
protected:
    static constexpr auto USE_TRANSPARENCY{true};
private:
    NaviGlArea *m_naviGlArea;
    PlaneContext* m_planeContext{};
    PlaneGeometry* m_planePane{};
    std::shared_ptr<psc::gl::Font2> m_font;
    psc::mem::active_ptr<psc::gl::Text2> m_text;
    psc::mem::active_ptr<psc::gl::Text2> m_text1;
    psc::mem::active_ptr<psc::gl::Text2> m_text2;
    psc::mem::active_ptr<psc::gl::Text2> m_text3;

    SmokeContext* m_smokeContext{};
    psc::mem::active_ptr<psc::gl::Geom2> m_smokePlane;
    Gtk::Application* m_application;
    GlSceneWindow* m_glSceneWindow;
};
