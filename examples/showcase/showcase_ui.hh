// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#ifndef ZGUI_SHOWCASE_UI_HH
#define ZGUI_SHOWCASE_UI_HH

#include "../../zgui.hh"

#include <cstdint>
#include <memory>
#include <vector>

namespace zgui_showcase {

    struct AppState {
        zgui::Bound<bool> enabled = zgui::bound(true);
        zgui::Bound<bool> pinned = zgui::bound(false);
        zgui::Bound<std::vector<bool>> table_columns =
            zgui::bound(std::vector<bool>{true, true, true});
        zgui::Bound<bool> two_column = zgui::bound(false);
        zgui::Bound<float> progress = zgui::bound(0.65f);
        zgui::Bound<float> scale = zgui::bound(1.f);
        zgui::Bound<int> stress_count = zgui::bound(120);
        zgui::Bound<std::string> input = zgui::bound(std::string("Edit this text"));
        zgui::Bound<std::size_t> choice = zgui::bound(std::size_t{0});
        zgui::Bound<std::size_t> radio_choice = zgui::bound(std::size_t{0});
        zgui::Bound<std::vector<bool>> flags = zgui::bound(std::vector<bool>{true, false, true});
        zgui::Bound<int> key = zgui::bound(0);
        zgui::Theme theme = zgui::core::default_theme;
        zgui::Color viewport_color{100, 180, 240};
        zgui::Bound<zgui::Color> layout_color_one = zgui::bound(zgui::Color{220, 90, 90});
        zgui::Bound<zgui::Color> layout_color_two = zgui::bound(zgui::Color{90, 220, 90});
        zgui::Bound<zgui::Color> layout_color_three = zgui::bound(zgui::Color{90, 90, 220});
        std::shared_ptr<zgui::Window> main_window;
        std::shared_ptr<zgui::TabBar> tabs;
        std::shared_ptr<zgui::Label> status;
        std::shared_ptr<zgui::Label> table_detail;
        std::shared_ptr<zgui::Label> frame_time;
        std::shared_ptr<zgui::TableView> table;
        std::shared_ptr<zgui::ScrollArea> stress_area;
        std::vector<std::shared_ptr<zgui::Label>> stress_labels;
        std::shared_ptr<zgui::Window> spawned_window;
        std::size_t attached_stress_labels = 0;
        std::size_t table_rows = 10000;
        bool sort_descending = false;
        bool fuzz_mouse_down = false;
        float viewport_angle = 0.f;
        float viewport_pan_x = 0.f;
        float viewport_pan_y = 0.f;
        double frame_seconds = 0.0;
        std::uint64_t previous_counter = 0;
        std::uint32_t fuzz_state = 0x4d595df4u;
    };

    /// Builds every tutorial page using only zgui's public APIs.
    std::shared_ptr<zgui::Window> build_showcase_ui(zgui::WindowManager& manager, AppState& state);

} // namespace zgui_showcase

#endif // ZGUI_SHOWCASE_UI_HH
