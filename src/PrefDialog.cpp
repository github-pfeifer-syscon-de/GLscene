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
    builder->get_widget("keepSum", m_keepSum);
    m_keepSum->set_active(m_planeGeometry->isKeepSum());
    m_keepSum->signal_toggled().connect([this] {
        m_planeGeometry->setKeepSum(m_keepSum->get_active());
    });
    builder->get_widget("signalGen", m_signalGen);
    m_signalGen->signal_toggled().connect(
        sigc::mem_fun(*this, &PrefDialog::signalGenToggel));
    builder->get_widget("frequency", m_frequency);
    m_frequency->signal_value_changed().connect(
        sigc::mem_fun(*this, &PrefDialog::signalFrequencyChange));
    builder->get_widget("volume", m_volume);
    m_volume->signal_value_changed().connect(
        sigc::mem_fun(*this, &PrefDialog::signalVolumeChange));
}

void
PrefDialog::signalGenToggel()
{
    m_frequency->set_sensitive(m_signalGen->get_active());
    m_volume->set_sensitive(m_signalGen->get_active());
    m_signalGen->set_sensitive(false);  // avoid rapid changes
    if (m_signalGen->get_active()) {
        if (!m_source) {
            m_source = std::make_shared<psc::snd::SineSource>();
            m_volume->set_value(m_source->getVolume());
        }
        if (!m_out || !m_out->isReady()) {
            auto ctx = m_planeGeometry->getPulseContext();
            psc::snd::PulseFormat fmt;
            m_out = std::make_shared<psc::snd::PulseOut>(ctx, fmt, m_source);
            m_out->setWriteLong(false);
            m_out->addStreamListener(this);
        }
    }
    else {
        m_out->drain();
    }
}

void
PrefDialog::signalFrequencyChange()
{
    if (m_source) {
        m_source->setFrequency(static_cast<float>(m_frequency->get_value()));
    }
}

void
PrefDialog::signalVolumeChange()
{
    if (m_source) {
        m_source->setVolume(static_cast<float>(m_volume->get_value()));
    }
}

double
PrefDialog::getMaxLevel()
{
    return m_maxLevel->get_value();
}

bool
PrefDialog::isKeepSum()
{
    return m_keepSum->get_active();
}

void
PrefDialog::streamNotify(psc::snd::PulseStreamState state)
{
    switch(state) {
    case psc::snd::PulseStreamState::ready:
    case psc::snd::PulseStreamState::disconnected:
    case psc::snd::PulseStreamState::terminated:
        m_signalGen->set_sensitive(true);
        break;
    default:
        break;
    }
    if (state == psc::snd::PulseStreamState::ready) {
    }

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
            auto keyConfig = sceneWindow->getKeyConfig();
            if (ret == Gtk::ResponseType::RESPONSE_OK) {
                sceneWindow->saveConfig();
            }
            else {
                double maxLevel = keyConfig->getDouble(GlSceneWindow::MAIN_SECTION, GlSceneWindow::SCALE_KEY, 1.0);
                planeGeometry->setScale(maxLevel);
                planeGeometry->setKeepSum(keyConfig->getBoolean(GlSceneWindow::MAIN_SECTION, GlSceneWindow::KEEP_SUM_KEY, false));
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