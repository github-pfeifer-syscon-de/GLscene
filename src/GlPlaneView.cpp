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
#include <cstdlib>
#include <iomanip>
#include <fstream>

#include <gtkmm.h>
#include <epoxy/gl.h>
#include <iostream>
#include <GL/glu.h>
#include <cmath>

#include "GlPlaneView.hpp"
#include "PlaneGeometry.hpp"

GlPlaneView::GlPlaneView(Gtk::Application* application)
: Scene()

, m_application{application}
{
}

GlPlaneView::~GlPlaneView()
{
    // destroy is done with unrealize as this has the right context
}

Position
GlPlaneView::getIntialPosition()
{
    return Position(0.0f,12.0f,14.0f);
}

Rotational
GlPlaneView::getInitalAngleDegree()
{
    return Rotational(-25.0f, 0.0f, 0.0f);
}

guint32 GlPlaneView::getAnimationMs()
{
    return 1000/15;
}


gboolean
GlPlaneView::init_shaders(Glib::Error &error) {
    gboolean ret = TRUE;
    try {
        m_planeContext = new PlaneContext();
        gsize szVertex,szFragm;
        Glib::RefPtr<const Glib::Bytes> refVertex = Gio::Resource::lookup_data_global(m_application->get_resource_base_path() + "/vertex.glsl");
        Glib::RefPtr<const Glib::Bytes> refFragm = Gio::Resource::lookup_data_global(m_application->get_resource_base_path() + "/fragment.glsl");
        ret = m_planeContext->createProgram(refVertex->get_data(szVertex), refFragm->get_data(szFragm), error);
        if (ret) {
            m_smokeContext = new SmokeContext();
            gsize szVertex,szFragm;
            Glib::RefPtr<const Glib::Bytes> refVertex = Gio::Resource::lookup_data_global(m_application->get_resource_base_path() + "/smoke-vertex.glsl");
            Glib::RefPtr<const Glib::Bytes> refFragm = Gio::Resource::lookup_data_global(m_application->get_resource_base_path() + "/waveshader.glsl"); //  smoke-fragment
            ret = m_smokeContext->createProgram(refVertex->get_data(szVertex), refFragm->get_data(szFragm), error);
        }
    }
    catch (const Glib::Error &err) {
        std::cout << "Error compiling " << err.what() << std::endl;
        ret = FALSE;
    }
    return ret;
}

void
GlPlaneView::init(Gtk::GLArea *glArea)
{
    m_naviGlArea = (NaviGlArea *)glArea;
    m_planePane = new PlaneGeometry(m_planeContext);
    psc::gl::checkError("plane createVao");
    //std::cout << "geo vert: " << m_plane->getNumVertex()
    //          << " idx: " << m_plane->getNumIndex()
    //          << std::endl;
    m_smokePlane = m_smokeContext->createGeometry(GL_TRIANGLES);
    if (auto lSmokePlane = m_smokePlane.lease()) {
        Position p1(-10.0f, 0.0, -10.0f);   UV uv1(0.0f, 0.0f);
        Position p2(10.0f, 0.0, -10.0f);    UV uv2(1.0f, 0.0f);
        Position p3(10.0f, 0.0, 10.0f);     UV uv3(1.0f, 1.0f);
        Position p4(-10.0f, 0.0, 10.0f);    UV uv4(0.0f, 1.0f);
        lSmokePlane->addPoint(&p1, NULL, NULL, &uv1);
        lSmokePlane->addPoint(&p2, NULL, NULL, &uv2);
        lSmokePlane->addPoint(&p3, NULL, NULL, &uv3);
        lSmokePlane->addPoint(&p4, NULL, NULL, &uv4);
        lSmokePlane->addIndex(0, 1, 2);
        lSmokePlane->addIndex(2, 3, 0);
        lSmokePlane->setScalePos(-15.0f, 0.0f, 0.0f, 1.0f);
        lSmokePlane->create_vao();
        psc::gl::checkError("smoke createVao");
    }

    float s = 0.05f;
    m_font = std::make_shared<psc::gl::Font2>("sans-serif");
    m_text = psc::mem::make_active<psc::gl::Text2>(GL_TRIANGLES, m_planeContext, m_font);
    auto lText = m_text.lease();
    lText->setSensitivity(0.1f);
    Glib::ustring txt("Cello 0,0");
    lText->setText(txt);
    Position p(1.0f, 0.0f, 0.0f);
    lText->setPosition(p);
    lText->setScale(s);
    Rotational r(0.0f, 45.0f, 0.0f);
    lText->setRotation(r);
    //m_planeContext->addGeometry(m_text);

    m_text1 = psc::mem::make_active<psc::gl::Text2>(GL_TRIANGLES, m_planeContext, m_font);
    auto lText1 = m_text1.lease();
    lText1->setSensitivity(0.1f);
    Glib::ustring txt1("Cello 90,0");
    lText1->setText(txt1);
    Position pt1(0.0f, 1.0f, 0.0f);
    lText1->setPosition(pt1);
    lText1->setScale(s);
    Rotational r1(90.0f, 45.0f, 0.0f);
    lText1->setRotation(r1);
    //m_planeContext->addGeometry(m_text1);

    m_text2 = psc::mem::make_active<psc::gl::Text2>(GL_TRIANGLES, m_planeContext, m_font);
    auto lText2 = m_text2.lease();
    lText2->setSensitivity(0.1f);
    Glib::ustring txt2("Cello 180,0");
    lText2->setText(txt2);
    Position pt2(0.0f, -1.0f, 0.0f);
    lText2->setPosition(pt2);
    lText2->setScale(s);
    Rotational r2(180.0f, 45.0f, 0.0f);
    lText2->setRotation(r2);
    //m_planeContext->addGeometry(m_text2);

    m_text3 = psc::mem::make_active<psc::gl::Text2>(GL_TRIANGLES, m_planeContext, m_font);
    auto lText3 = m_text3.lease();
    lText3->setSensitivity(0.1f);
    Glib::ustring txt3("Cello 270,0");
    lText3->setText(txt3);
    Position pt3(-1.0f, 0.0f, 0.0f);
    lText3->setPosition(pt3);
    lText3->setScale(s);
    Rotational r3(270.0f, 45.0f, 0.0f);
    lText3->setRotation(r3);
    //m_planeContext->addGeometry(m_text3);
}

void
GlPlaneView::unrealize()
{
    if (m_planeContext) {
        delete m_planeContext;
    }
    if (m_planePane) {
        delete m_planePane;
    }
    if (m_smokeContext){
        delete m_smokeContext;
    }
}


void
GlPlaneView::draw(Gtk::GLArea *glArea, Matrix &proj, Matrix &view)
{

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    psc::gl::checkError("clear");

    m_planeContext->use();
    psc::gl::checkError("useProgram");
    m_planePane->advance();
    psc::gl::checkError("advance");

    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //checkError("glBlendFunc");
    //glEnable(GL_BLEND);
    //checkError("glEnable(GL_BLEND)");
    //glDisable(GL_DEPTH_TEST);
    //checkError("glDisable(GL_DEPTH_TEST)");

    UV uv(glArea->get_width(), glArea->get_height());
    m_planeContext->setResolution(uv);
    GLfloat lineWidth = 3;
    m_planeContext->setLineWidth(lineWidth);
    glLineWidth(lineWidth);
    Matrix projView = proj * view;
    m_planeContext->display(projView);
    psc::gl::checkError("display plane");
    m_planeContext->unuse();
    //glDisable(GL_BLEND);
    //checkError("glDisable(GL_BLEND)");
    //glEnable(GL_DEPTH_TEST);
    //checkError("glEnable(GL_DEPTH_TEST)");

    m_smokeContext->use();
    psc::gl::checkError("useSmokeCtx");

    // UV res(0.02f, 0.02f); // fits smoke
    UV res(1.0f, 1.0f);   // fits wave
    m_smokeContext->setResolution(res);
    gint64 time = g_get_monotonic_time();    // the promise is this does not get screwed up by time adjustments
    float t = (float)((double)time/3.0E6);
    //std::cout << "t: " << t << std::endl;
    m_smokeContext->setTime(t);

    m_smokeContext->display(projView);
    m_smokeContext->unuse();
}


psc::gl::aptrGeom2
GlPlaneView::on_click_select(GdkEventButton* event, float mx, float my)
{
    //auto selected = m_smokeContext->hit(mx, my);
    //if (!selected) {
    auto selected = m_planeContext->hit2(mx, my);
    //}
    return selected;
}