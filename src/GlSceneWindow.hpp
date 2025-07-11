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

#include <KeyConfig.hpp>

#include "GlPlaneView.hpp"


class GlSceneApp;
class GlPlaneView;
class PlaneGeometry;

class GlSceneWindow : public Gtk::ApplicationWindow {
public:
    GlSceneWindow(GlSceneApp* application);
    virtual ~GlSceneWindow() ;

    void on_action_preferences();
    Gtk::Application* getApplicaiton();
    PlaneGeometry* getPlaneGeometry();
    std::shared_ptr<KeyConfig> getKeyConfig();
    void saveConfig();
    void restoreConfig();
    static constexpr auto MAIN_SECTION{"main"};
    static constexpr auto SCALE_KEY{"scaleY"};
    static constexpr auto KEEP_SUM_KEY{"keepSum"};
    static constexpr auto FREQ_USE_KEY{"frequUsage"};
    static constexpr auto FREQ_SCALE_MODE_KEY{"frequScaleMode"};
private:
    GlSceneApp* m_application;
    GlPlaneView* m_planView{nullptr};
    std::shared_ptr<KeyConfig> m_keyConfig;
};

