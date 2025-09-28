/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2025 RPf 
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


#include "SpectrumPlot.hpp"
#include "GlSceneWindow.hpp"

SpectrumPlot::SpectrumPlot(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, GlSceneWindow* sceneWindow)
: psc::ui::Plot(cobject, builder)
{
    builder->get_widget("active", m_active);
    m_active->set_active(m_drawing->isActive());
    m_active->signal_clicked().connect([this] {
        m_drawing->setActive(m_active->get_active());
    });
    builder->get_widget("scale", m_scale);
    m_scale->set_value(sceneWindow->getPlaneGeometry()->getScale());
    m_scale->signal_value_changed().connect([this, sceneWindow] {
        sceneWindow->getPlaneGeometry()->setScale(m_scale->get_value());
    });
}

void
SpectrumPlot::show(GlSceneWindow* sceneWindow, const std::shared_ptr<psc::ui::PlotView>& func)
{
    auto refBuilder = Gtk::Builder::create();
    try {
        auto appl = sceneWindow->get_application();
        refBuilder->add_from_resource(appl->get_resource_base_path() + "/plot-dlg.ui");
        SpectrumPlot* plot;
        refBuilder->get_widget_derived("plotDlg", plot, sceneWindow);
        if (plot) {
            plot->set_transient_for(*sceneWindow);
            std::vector<std::shared_ptr<psc::ui::PlotView>> views;
            views.push_back(func);
            plot->plot(views);
            plot->run();
            delete plot;
        }
        else {
            std::cerr << "Plot::show no plot-dlg" << std::endl;
        }
    }
    catch (const Glib::Error& ex) {
        std::cerr << "Plot::show error loading plot-dlg.ui " << ex.what() << std::endl;
    }
    return;
}
