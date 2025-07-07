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

#include "Row.hpp"
#include "GlPlaneView.hpp"
#include "PlaneGeometry.hpp"
#include "GlSceneWindow.hpp"

GlPlaneView::GlPlaneView(GlSceneWindow* glSceneWindow)
: Scene()
, m_application(glSceneWindow->getApplicaiton())
, m_glSceneWindow{glSceneWindow}
{
}

GlPlaneView::~GlPlaneView()
{
    // destroy is done with unrealize as this has the right context
}

Position
GlPlaneView::getIntialPosition()
{
    return Position(0.0f,12.0f,18.0f);
}

Rotational
GlPlaneView::getInitalAngleDegree()
{
    return Rotational(-25.0f, 0.0f, 0.0f);
}

guint32 GlPlaneView::getAnimationMs()
{
    return 1000/20;
}


gboolean
GlPlaneView::init_shaders(Glib::Error &error) {
    gboolean ret = true;
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
        ret = false;
    }
    return ret;
}

void
GlPlaneView::init(Gtk::GLArea *glArea)
{
    m_naviGlArea = (NaviGlArea *)glArea;
    //std::cout << "GlPlaneView::init"  << std::boolalpha << glArea->get_double_buffered() << std::endl;
    glArea->set_double_buffered(false);     // reduce effort for animation, runs smoother
    m_planePane = new PlaneGeometry(m_planeContext);
    auto keyConfig = m_glSceneWindow->getKeyConfig();
    m_planePane->setScale(keyConfig->getDouble(GlSceneWindow::MAIN_SECTION, GlSceneWindow::SCALE_KEY, 1.0));
    m_planePane->setKeepSum(keyConfig->getBoolean(GlSceneWindow::MAIN_SECTION, GlSceneWindow::KEEP_SUM_KEY, false));
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
        lSmokePlane->addPoint(&p1, nullptr, nullptr, &uv1);
        lSmokePlane->addPoint(&p2, nullptr, nullptr, &uv2);
        lSmokePlane->addPoint(&p3, nullptr, nullptr, &uv3);
        lSmokePlane->addPoint(&p4, nullptr, nullptr, &uv4);
        lSmokePlane->addIndex(0, 1, 2);
        lSmokePlane->addIndex(2, 3, 0);
        lSmokePlane->setScalePos(-12.0f, 0.0f, 0.0f, 1.0f);
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
    gint64 time = g_get_monotonic_time();    // the promise is this does not get screwed up by time adjustments
    m_planePane->advance(time);
    psc::gl::checkError("advance");

    if (USE_TRANSPARENCY) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        psc::gl::checkError("glBlendFunc");
        glEnable(GL_BLEND);
        psc::gl::checkError("glEnable(GL_BLEND)");
        //glDisable(GL_DEPTH_TEST);
        //psc::gl::checkError("glDisable(GL_DEPTH_TEST)");
    }

    UV uv(glArea->get_width(), glArea->get_height());
    m_planeContext->setResolution(uv);
    GLfloat lineWidth = 3.0f;
    m_planeContext->setLineWidth(lineWidth);
    glLineWidth(lineWidth);
    m_planeContext->setAlpha(1.0f);

    Vector light{0.f, -1.f, 0.f};
    m_planeContext->setLight(light);

    Matrix projView = proj * view;
    auto midRows = m_planePane->getMidRows();
    m_planeContext->display(projView, midRows);
    psc::gl::checkError("display plane");
    m_planeContext->setAlpha(m_planePane->getFrontAlpha());
    std::list<psc::mem::active_ptr<psc::gl::Geom2>> front;
    front.push_back(m_planePane->getFrontRow());
    m_planeContext->display(projView, front);
    m_planeContext->setAlpha(m_planePane->getBackAlpha());
    std::list<psc::mem::active_ptr<psc::gl::Geom2>> back;
    back.push_back(m_planePane->getBackRow());
    m_planeContext->display(projView, back);
    m_planeContext->unuse();
    if (USE_TRANSPARENCY) { // and back to "normal"
        glBlendFunc(GL_ONE, GL_ZERO);
        glDisable(GL_BLEND);
        //glEnable(GL_DEPTH_TEST);
    }


    if (PlaneContext::showSmokeShader) {
        m_smokeContext->use();
        psc::gl::checkError("useSmokeCtx");

        // UV res(0.02f, 0.02f); // fits smoke
        UV res(1.0f, 1.0f);   // fits wave
        m_smokeContext->setResolution(res);
        //std::cout << "t: " << t << std::endl;
        float t = (float)((double)time/3.0E6);
        m_smokeContext->setTime(t);

        m_smokeContext->display(projView);
        m_smokeContext->unuse();
    }
    //gint64 end = g_get_monotonic_time();
    //std::cout << "display " << (end-start) << std::endl;
    //Glib::DateTime dt = Glib::DateTime::create_now_local();
    //std::cout << "Time " << dt.format_iso8601() << std::endl;
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

PlaneGeometry*
GlPlaneView::getPlaneGeometry()
{
    return m_planePane;
}