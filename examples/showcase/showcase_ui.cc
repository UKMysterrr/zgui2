// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "showcase_ui.hh"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {
    void build_basic_controls(zgui::UIElement& page, zgui_showcase::AppState& state) {
        zgui::StackLayout layout = zgui::StackLayout::page(page);
        layout.add_row(std::make_shared<zgui::Label>("Buttons, toggles, progress, and separators"));
        layout.add_row(
            std::make_shared<zgui::Button>(
                "Click to update progress",
                [&state] { *state.progress.get() = std::min(1.f, *state.progress.get() + 0.1f); }),
            24.f);
        layout.add_row(std::make_shared<zgui::CheckBox>("Enabled", state.enabled), 20.f);
        layout.add_row(std::make_shared<zgui::ToggleButton>("Pinned", state.pinned), 24.f);
        layout.add_row(std::make_shared<zgui::ProgressBar>(state.progress, "65%"), 20.f);
        layout.add_row(
            std::make_shared<zgui::Separator>(zgui::SeparatorOrientation::horizontal, "Controls"),
            18.f);
        layout.add_row(std::make_shared<zgui::Separator>(zgui::SeparatorOrientation::vertical),
                       24.f);
    }

    void build_inputs_and_binding(zgui::UIElement& page, zgui_showcase::AppState& state) {
        zgui::StackLayout layout = zgui::StackLayout::page(page);
        layout.add_row(std::make_shared<zgui::Label>("Text input, key capture, and bound sliders"));
        layout.add_row(std::make_shared<zgui::TextInput>(state.input), 22.f);
        layout.add_row(std::make_shared<zgui::Slider<float>>("Scale", state.scale, 0.5f, 2.f),
                       28.f);
        layout.add_row(std::make_shared<zgui::KeyBind>("Shortcut", state.key), 22.f);
        layout.add_row(std::make_shared<zgui::Label>(
                           "Narrow text is truncated; unsupported characters use ASCII fallback."),
                       20.f);
        auto narrow = std::make_shared<zgui::Label>("Aligned and deliberately truncated text");
        narrow->set_alignment(zgui::TextAlignment::center);
        layout.add_row(narrow, 18.f);
    }

    void build_selection_widgets(zgui::UIElement& page, zgui_showcase::AppState& state) {
        zgui::StackLayout layout = zgui::StackLayout::page(page);
        auto combo = std::make_shared<zgui::ComboBox>(
            std::vector<std::string>{"Windowed", "Borderless", "Fullscreen"});
        combo->bind_selected_index(state.choice);
        layout.add_row(combo, 22.f);
        layout.add_row(std::make_shared<zgui::MultiComboBox>(
                           std::vector<std::string>{"Grid", "Guides", "Labels"}, state.flags),
                       22.f);
        auto radios = std::make_shared<zgui::RadioGroup>(
            std::vector<std::string>{"Small", "Medium", "Large"}, state.radio_choice);
        radios->set_size({220.f, 54.f});
        layout.add_row(radios, 54.f);
        auto scroll = std::make_shared<zgui::ScrollArea>();
        scroll->set_size({700.f, 250.f});
        for (int row = 0; row < 24; ++row) {
            auto label = std::make_shared<zgui::Label>("Scrollable row " + std::to_string(row));
            label->set_position({8.f, static_cast<float>(row) * 22.f});
            label->set_size({260.f, 18.f});
            scroll->add_child(label);
        }
        layout.add_row(scroll, 250.f);
    }

    void build_color_tools(zgui::UIElement& page, zgui_showcase::AppState& state) {
        auto swatch = std::make_shared<zgui::ColorSwatch>("Accent", state.viewport_color);
        swatch->set_size({220.f, 52.f});
        swatch->set_position({8.f, 8.f});
        page.add_child(swatch);
        auto picker = std::make_shared<zgui::ColorPicker>(state.viewport_color);
        picker->set_position({8.f, 72.f});
        picker->set_size({264.f, 212.f});
        page.add_child(picker);
        auto info = std::make_shared<zgui::Label>("Edit the swatch or drag the picker planes.");
        info->set_position({290.f, 12.f});
        info->set_size({400.f, 20.f});
        page.add_child(info);
    }

    void populate_table(zgui_showcase::AppState& state) {
        state.table->clear_rows();
        for (std::size_t row = 0; row < state.table_rows; ++row) {
            const std::string number = std::to_string(row);
            const std::string name = state.sort_descending
                                         ? "Item " + std::to_string(state.table_rows - row - 1)
                                         : "Item " + number;
            state.table->add_row({number, name, row % 2 == 0 ? "ready" : "pending"});
        }
        state.table->set_cell(0, 2, {"selected", zgui::Color(90, 210, 140), false});
    }

    void build_tables(zgui::UIElement& page, zgui_showcase::AppState& state) {
        auto sort = std::make_shared<zgui::Button>("Reverse sort", [&state] {
            state.sort_descending = !state.sort_descending;
            populate_table(state);
        });
        sort->set_position({8.f, 8.f});
        sort->set_size({120.f, 22.f});
        page.add_child(sort);
        auto columns = std::make_shared<zgui::MultiComboBox>(
            std::vector<std::string>{"ID", "Name", "Status"}, state.table_columns, [&state] {
                for (std::size_t column = 0; column < state.table_columns.get()->size(); ++column)
                    state.table->set_column_visible(column, (*state.table_columns.get())[column]);
            });
        columns->set_position({145.f, 9.f});
        columns->set_size({180.f, 22.f});
        page.add_child(columns);
        state.table_detail = std::make_shared<zgui::Label>("Select a row for details.");
        state.table_detail->set_position({340.f, 10.f});
        state.table_detail->set_size({380.f, 20.f});
        page.add_child(state.table_detail);
        state.table = std::make_shared<zgui::TableView>();
        state.table->set_position({8.f, 38.f});
        state.table->set_size({710.f, 380.f});
        state.table->set_columns({{"ID", 80.f}, {"Name", 300.f}, {"Status", 150.f}});
        state.table->set_on_selection_changed([&state](std::size_t row) {
            state.table_detail->set_text("Selected row " + std::to_string(row));
        });
        page.add_child(state.table);
        populate_table(state);
    }

    void build_layout_and_anchors(zgui::UIElement& page, zgui_showcase::AppState& state) {
        auto anchored = std::make_shared<zgui::Label>("Fractional constraint: 10% to 90%");
        anchored->set_position({20.f, 12.f});
        anchored->set_size({520.f, 20.f});
        anchored->set_horizontal_constraints(0.1f, 0.9f, 0.f, 0.f);
        page.add_child(anchored);
        auto group = std::make_shared<zgui::GroupBox>("Nested group and sub-tabs");
        group->set_position({12.f, 48.f});
        group->set_size({700.f, 340.f});
        group->set_fit_content(false);
        auto tabs = std::make_shared<zgui::SubTabBar>();
        auto anchors_page = tabs->add_tab("Anchors");
        auto anchor_label = std::make_shared<zgui::Label>("Anchored sub-tab page.");
        anchor_label->set_position({8.f, 8.f});
        anchor_label->set_size({240.f, 20.f});
        anchors_page->add_child(anchor_label);
        auto second = tabs->add_tab("Stack rows");
        auto label = std::make_shared<zgui::Label>("This page is laid out with StackLayout.");
        label->set_position({8.f, 8.f});
        label->set_size({320.f, 20.f});
        second->add_child(label);
        group->set_sub_tabs(tabs);
        page.add_child(group);
        zgui::StackLayout rows(page, 400.f, 8.f, 2.f);
        rows.add_two_column_row(
            std::make_shared<zgui::CheckBox>("Two-column row", state.two_column),
            std::make_shared<zgui::ComboBox>(std::vector<std::string>{"First", "Second"}));
        auto first = std::make_shared<zgui::ColorButton>("One", state.layout_color_one);
        auto second_color = std::make_shared<zgui::ColorButton>("Two", state.layout_color_two);
        auto third = std::make_shared<zgui::ColorButton>("Three", state.layout_color_three);
        rows.add_three_column_row(first, second_color, third, 44.f);
    }

    void build_windows_and_overlays(zgui::UIElement& page, zgui_showcase::AppState& state,
                                    zgui::WindowManager& manager) {
        auto spawn = std::make_shared<zgui::Button>("Spawn / close window", [&state, &manager] {
            if (state.spawned_window) {
                manager.remove_window(state.spawned_window);
                state.spawned_window.reset();
                return;
            }
            auto window = std::make_shared<zgui::Window>("Secondary");
            window->set_position({560.f, 80.f});
            window->set_size({220.f, 150.f});
            window->set_always_on_top(*state.pinned.get());
            auto label = std::make_shared<zgui::Label>("Always-on-top demo");
            label->set_position({8.f, 26.f});
            label->set_size({180.f, 18.f});
            window->add_child(label);
            state.spawned_window = window;
            manager.add_window(window);
        });
        spawn->set_position({12.f, 12.f});
        spawn->set_size({180.f, 24.f});
        page.add_child(spawn);
        auto topmost =
            std::make_shared<zgui::ToggleButton>("Always on top", state.pinned, [&state] {
                if (state.spawned_window)
                    state.spawned_window->set_always_on_top(*state.pinned.get());
            });
        topmost->set_position({210.f, 12.f});
        topmost->set_size({150.f, 24.f});
        page.add_child(topmost);
        auto frameless = std::make_shared<zgui::Window>("Frameless child");
        frameless->set_frameless(true);
        frameless->set_position({12.f, 52.f});
        frameless->set_size({240.f, 120.f});
        frameless->set_resizable(false);
        frameless->add_child(std::make_shared<zgui::Label>("No title or frame"));
        page.add_child(frameless);
        auto edge_combo =
            std::make_shared<zgui::ComboBox>(std::vector<std::string>{"Near edge", "Popup flips"});
        edge_combo->set_position({570.f, 300.f});
        edge_combo->set_size({140.f, 22.f});
        page.add_child(edge_combo);
        auto tooltip = std::make_shared<zgui::Tooltip>("A delayed, clamped tooltip");
        tooltip->set_position({12.f, 190.f});
        tooltip->set_size({200.f, 26.f});
        page.add_child(tooltip);
    }

    void build_viewport_demo(zgui::UIElement& page, zgui_showcase::AppState& state) {
        auto viewport = std::make_shared<zgui::Viewport>();
        viewport->set_position({12.f, 12.f});
        viewport->set_size({680.f, 380.f});
        viewport->set_on_input([&state](const zgui::ViewportInput& input) {
            if (input.double_clicked) {
                state.viewport_angle = state.viewport_pan_x = state.viewport_pan_y = 0.f;
                *state.scale.get() = 1.f;
            } else {
                if (input.rotating)
                    state.viewport_angle += input.mouse_delta.x * 0.01f;
                if (input.panning) {
                    state.viewport_pan_x += input.mouse_delta.x;
                    state.viewport_pan_y += input.mouse_delta.y;
                }
                if (input.wheel_delta != 0)
                    *state.scale.get() = std::clamp(
                        *state.scale.get() + static_cast<float>(input.wheel_delta) * 0.05f, 0.5f,
                        2.f);
            }
        });
        viewport->set_on_render([&state](int x, int y, int width, int height) {
            auto& graphics = *zgui::core::graphics;
            const float size = 40.f * *state.scale.get();
            const float cx =
                static_cast<float>(x) + static_cast<float>(width) / 2.f + state.viewport_pan_x;
            const float cy =
                static_cast<float>(y) + static_cast<float>(height) / 2.f + state.viewport_pan_y;
            const float c = std::cos(state.viewport_angle);
            const float s = std::sin(state.viewport_angle);
            const auto point = [cx, cy, c, s, size](float px, float py) {
                return zgui::Point(cx + (px * c - py * s) * size, cy + (px * s + py * c) * size);
            };
            const zgui::Point a = point(-1.f, -1.f);
            const zgui::Point b = point(1.f, -1.f);
            const zgui::Point c1 = point(1.f, 1.f);
            const zgui::Point d = point(-1.f, 1.f);
            graphics.draw_line(a, b, state.viewport_color);
            graphics.draw_line(b, c1, state.viewport_color);
            graphics.draw_line(c1, d, state.viewport_color);
            graphics.draw_line(d, a, state.viewport_color);
            graphics.draw_line(a, c1, state.viewport_color);
            graphics.draw_line(b, d, state.viewport_color);
        });
        page.add_child(viewport);
    }

    void build_theme_editor(zgui::UIElement& page, zgui_showcase::AppState& state) {
        zgui::StackLayout layout = zgui::StackLayout::page(page, 8.f, 8.f, 6.f);
        const auto update = [&state] { zgui::core::set_theme(state.theme); };
        const auto button = [&update](const std::string& name, zgui::Color& color) {
            return std::make_shared<zgui::ColorButton>(name, color, update);
        };
        std::vector<std::shared_ptr<zgui::ColorButton>> colors{
            button("Window", state.theme.window_bg),
            button("Header", state.theme.window_header),
            button("Group", state.theme.groupbox_bg),
            button("Outer border", state.theme.border_outer),
            button("Inner border", state.theme.border_inner),
            button("Accent", state.theme.accent),
            button("Text", state.theme.text_primary),
            button("Dim text", state.theme.text_dim),
            button("Control", state.theme.control_bg),
            button("Hover", state.theme.control_hover)};
        for (std::size_t index = 0; index + 2 < colors.size(); index += 3)
            layout.add_three_column_row(colors[index], colors[index + 1], colors[index + 2], 52.f);
        for (std::size_t index = colors.size() / 3 * 3; index < colors.size(); ++index)
            layout.add_row(colors[index], 52.f);
        layout.add_row(std::make_shared<zgui::Button>("Reset theme",
                                                      [&state] {
                                                          state.theme = zgui::core::default_theme;
                                                          zgui::core::set_theme(state.theme);
                                                      }),
                       24.f);
        layout.add_row(std::make_shared<zgui::Button>(
                           "Copy as C++",
                           [&state] {
                               const auto print = [](const zgui::Color& color) {
                                   std::cout << "{" << static_cast<int>(color.r) << ", "
                                             << static_cast<int>(color.g) << ", "
                                             << static_cast<int>(color.b) << "}";
                               };
                               std::cout << "Theme{";
                               print(state.theme.window_bg);
                               std::cout << ", ";
                               print(state.theme.window_header);
                               std::cout << ", ";
                               print(state.theme.groupbox_bg);
                               std::cout << ", ";
                               print(state.theme.border_outer);
                               std::cout << ", ";
                               print(state.theme.border_inner);
                               std::cout << ", ";
                               print(state.theme.accent);
                               std::cout << ", ";
                               print(state.theme.text_primary);
                               std::cout << ", ";
                               print(state.theme.text_dim);
                               std::cout << ", ";
                               print(state.theme.control_bg);
                               std::cout << ", ";
                               print(state.theme.control_hover);
                               std::cout << "}\n";
                           }),
                       24.f);
    }

    void build_stress_test(zgui::UIElement& page, zgui_showcase::AppState& state) {
        state.frame_time = std::make_shared<zgui::Label>("Frame time: waiting for first frame");
        state.frame_time->set_position({8.f, 8.f});
        state.frame_time->set_size({360.f, 20.f});
        page.add_child(state.frame_time);
        state.stress_area = std::make_shared<zgui::ScrollArea>();
        state.stress_area->set_position({8.f, 64.f});
        state.stress_area->set_size({710.f, 350.f});
        page.add_child(state.stress_area);
        state.stress_labels.reserve(2000);
        for (int index = 0; index < 2000; ++index) {
            auto label = std::make_shared<zgui::Label>("Widget " + std::to_string(index));
            label->set_position(
                {static_cast<float>((index % 2) * 340), static_cast<float>((index / 2) * 20)});
            label->set_size({320.f, 18.f});
            state.stress_labels.push_back(label);
        }
        const auto sync = [&state] {
            const std::size_t desired = static_cast<std::size_t>(*state.stress_count.get());
            while (state.attached_stress_labels < desired) {
                state.stress_area->add_child(state.stress_labels[state.attached_stress_labels]);
                ++state.attached_stress_labels;
            }
            while (state.attached_stress_labels > desired) {
                --state.attached_stress_labels;
                state.stress_area->remove_child(
                    state.stress_labels[state.attached_stress_labels].get());
            }
        };
        sync();
        auto count =
            std::make_shared<zgui::Slider<int>>("Widgets", state.stress_count, 1, 2000, sync);
        count->set_position({8.f, 34.f});
        count->set_size({320.f, 26.f});
        page.add_child(count);
    }
} // namespace

std::shared_ptr<zgui::Window> zgui_showcase::build_showcase_ui(zgui::WindowManager& manager,
                                                               AppState& state) {
    state.theme = zgui::core::active_theme;
    state.main_window = std::make_shared<zgui::Window>("zgui C++17 Showcase");
    state.main_window->set_position({20.f, 20.f});
    state.main_window->set_size({760.f, 560.f});
    state.main_window->set_status_bar_visible(true);
    state.tabs = std::make_shared<zgui::TabBar>();
    state.tabs->set_position({8.f, 22.f});
    state.tabs->set_size({744.f, 470.f});
    state.tabs->set_anchors(true, true, true, true);

    const auto add_page = [&state](const std::string& name) { return state.tabs->add_tab(name); };
    build_basic_controls(*add_page("Basics"), state);
    build_inputs_and_binding(*add_page("Input"), state);
    build_selection_widgets(*add_page("Selection"), state);
    build_color_tools(*add_page("Colors"), state);
    build_tables(*add_page("Tables"), state);
    build_layout_and_anchors(*add_page("Layout"), state);
    build_windows_and_overlays(*add_page("Windows"), state, manager);
    build_viewport_demo(*add_page("Viewport"), state);
    build_theme_editor(*add_page("Theme"), state);
    build_stress_test(*add_page("Stress"), state);

    state.status = std::make_shared<zgui::Label>("zgui " ZGUI_VERSION_STRING);
    state.status->set_position({8.f, 520.f});
    state.status->set_size({700.f, 18.f});
    state.status->set_anchors(true, true, false, true);
    state.main_window->add_child(state.tabs);
    state.main_window->add_child(state.status);
    manager.add_window(state.main_window);
    return state.main_window;
}
