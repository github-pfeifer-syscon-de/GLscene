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


#include "PrefDialog.hpp"
#include "GlSceneWindow.hpp"
#include "PlaneGeometry.hpp"

PrefDialog::PrefDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, GlSceneWindow* sceneWindow)
: Gtk::Dialog(cobject)
, m_sceneWindow{sceneWindow}
{
    builder->get_widget("maxLevel", m_maxLevel);
    m_maxLevel->set_value(m_sceneWindow->getPlaneGeometry()->getScale());
    m_maxLevel->signal_value_changed().connect([this] {
        m_sceneWindow->getPlaneGeometry()->setScale(m_maxLevel->get_value());
    });
    builder->get_widget("keepSum", m_keepSum);
    m_keepSum->set_active(m_sceneWindow->getPlaneGeometry()->isKeepSum());
    m_keepSum->signal_toggled().connect([this] {
        m_sceneWindow->getPlaneGeometry()->setKeepSum(m_keepSum->get_active());
    });
    builder->get_widget("signalGen", m_signalGen);
    m_signalGen->signal_toggled().connect(
        sigc::mem_fun(*this, &PrefDialog::signalGenToggel));
    builder->get_widget("frequency", m_frequency);
    m_frequency->signal_value_changed().connect([this] {
        if (m_source) {
            m_source->setFrequency(static_cast<float>(m_frequency->get_value()));
        }
    });
    builder->get_widget("volume", m_volume);
    m_volume->signal_value_changed().connect([this] {
        if (m_source) {
            m_source->setVolume(static_cast<float>(m_volume->get_value()));
        }
    });
    builder->get_widget("freqUsage", m_freqUsage);
    m_freqUsage->set_value(m_sceneWindow->getPlaneGeometry()->getAudioUsageRate());
    m_freqUsage->signal_value_changed().connect([this] {
        m_sceneWindow->getPlaneGeometry()->setAudioUsageRate(m_freqUsage->get_value());
    });
    builder->get_widget("freqMode", m_freqMode);
    m_freqMode->append(GlPlaneView::FREQ_LINEAR, "Linear");
    m_freqMode->append(GlPlaneView::FREQ_LOGARITHMIC, "Logarithmic");
    m_freqMode->set_active_id(m_sceneWindow->getPlaneGeometry()->getScaleMode());
    m_freqMode->signal_changed().connect([this] {
        m_sceneWindow->getPlaneGeometry()->setScaleMode(m_freqMode->get_active_id());
    });
    builder->get_widget("movement", m_movement);
    m_movement->append(GlPlaneView::MOVE_FORWARD, "Forward");
    m_movement->append(GlPlaneView::MOVE_BACKWARD, "Backward");
    m_movement->set_active_id(m_sceneWindow->getPlaneView()->getMovement());
    m_movement->signal_changed().connect([this] {
        m_sceneWindow->getPlaneView()->setMovement(m_movement->get_active_id());
    });
    // Model section
    builder->get_widget("fileModel", m_fileChooser);
    auto file = m_sceneWindow->getPlaneView()->getModelFile();
    if (file) {
        m_fileChooser->set_file(file);
    }
    auto fileFilter = Gtk::FileFilter::create();
    fileFilter->add_pattern("*.obj");
    fileFilter->set_name("Obj");
    m_fileChooser->set_filter(fileFilter);
    m_fileChooser->signal_file_set().connect([this] {
        auto file = m_fileChooser->get_file();
        m_sceneWindow->getPlaneView()->setModelFile(file);
        m_displayModel->set_active(file.operator bool() && file->query_exists());
    });
    builder->get_widget("displayModel", m_displayModel);
    m_displayModel->set_active(file.operator bool() );
    m_displayModel->signal_toggled().connect([this] {
        m_sceneWindow->getPlaneView()->setModelFile(
                    m_displayModel->get_active()
                            ? m_fileChooser->get_file()
                            : Glib::RefPtr<Gio::File>());
    });
    builder->get_widget("modelAnimSpeed", m_modelAnimSpeed);
    m_modelAnimSpeed->set_value(m_sceneWindow->getPlaneView()->getModelAnimSpeed());
    m_modelAnimSpeed->signal_value_changed().connect( [this] {
        m_sceneWindow->getPlaneView()->setModelAnimSpeed(m_modelAnimSpeed->get_value());
    });
    builder->get_widget("showShader", m_showShader);
    m_showShader->set_active(m_sceneWindow->getPlaneView()->isShowShader());
    m_showShader->signal_toggled().connect( [this] {
        m_sceneWindow->getPlaneView()->setShowShader(m_showShader->get_active());
    });
}

void
PrefDialog::signalGenToggel()
{
    m_frequency->set_sensitive(m_signalGen->get_active());
    m_volume->set_sensitive(m_signalGen->get_active());
    m_signalGen->set_sensitive(false);  // avoid rapid changes
    if (m_signalGen->get_active()) {
        if (!m_source) {
            m_source = std::make_shared<psc::snd::AudioGenerator>();
            m_source->setShape(psc::snd::AudioShape::Sine);
            m_volume->set_value(m_source->getVolume());
        }
        if (!m_out || !m_out->isReady()) {
            auto ctx = m_sceneWindow->getPlaneGeometry()->getPulseContext();
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
}


void
PrefDialog::show(GlSceneWindow* sceneWindow)
{
    auto refBuilder = Gtk::Builder::create();
    try {
        auto appl = sceneWindow->getApplicaiton();
        refBuilder->add_from_resource(appl->get_resource_base_path() + "/prefs-dlg.ui");
        PrefDialog* prefDialog;
        refBuilder->get_widget_derived("pref-dlg", prefDialog, sceneWindow);
        if (prefDialog) {
            prefDialog->set_transient_for(*sceneWindow);
            int ret = prefDialog->run();
            if (ret == Gtk::ResponseType::RESPONSE_OK) {
                sceneWindow->saveConfig();
            }
            else {
                sceneWindow->restoreConfig();
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
