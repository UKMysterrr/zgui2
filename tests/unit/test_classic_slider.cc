// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "../../examples/classic_example/slider.hh"
#include "../support/test_context.hh"
#include "../support/test_harness.hh"

TEST(classic_steppers_align_with_tracks_and_change_both_values) {
    zgui_test::Harness harness;
    zgui::GroupBox group;
    group.set_fit_content(false);
    group.set_size({194.f, 180.f});
    const auto integer = zgui_classic::add_slider(group, "integer", 25, 0, 100, 1, 25.f);
    const auto floating = zgui_classic::add_slider(group, "float", 13.37f, 0.f, 25.f, 0.1f, 95.f);

    group.draw();
    for (const float y : {25.f, 95.f}) {
        const float track_y = y + zgui::spacing::kSliderLabelHeight;
        const float button_y = track_y + zgui::spacing::kSliderTrackHeight / 2.f - 7.f;
        bool track = false;
        bool minus = false;
        bool plus = false;
        bool minus_fill = false;
        bool plus_fill = false;
        bool minus_text = false;
        bool plus_text = false;
        for (const auto& command : harness.graphics->commands()) {
            if (command.type == zgui_test::GraphicsCommandType::Geometry &&
                command.vertices.size() == 6) {
                const auto& top_left = command.vertices[0];
                const auto& bottom_right = command.vertices[5];
                if (top_left.position.x == 30.f && top_left.position.y == track_y &&
                    bottom_right.position.x == 160.f &&
                    bottom_right.position.y == track_y + zgui::spacing::kSliderTrackHeight)
                    track = true;
                if (top_left.position.y == button_y && bottom_right.position.y == button_y + 14.f &&
                    top_left.color.r == zgui::core::active_theme.border_outer.r &&
                    top_left.color.g == zgui::core::active_theme.border_outer.g &&
                    top_left.color.b == zgui::core::active_theme.border_outer.b) {
                    if (top_left.position.x == 14.f && bottom_right.position.x == 28.f)
                        minus = true;
                    if (top_left.position.x == 162.f && bottom_right.position.x == 176.f)
                        plus = true;
                }
                if (top_left.position.y == button_y + 1.f &&
                    bottom_right.position.y == button_y + 13.f) {
                    if (top_left.position.x == 15.f && bottom_right.position.x == 27.f)
                        minus_fill = true;
                    if (top_left.position.x == 163.f && bottom_right.position.x == 175.f)
                        plus_fill = true;
                }
            }
            if (command.type == zgui_test::GraphicsCommandType::Text &&
                command.position.y == button_y + 1.f) {
                if (command.text == "-" && command.position.x == 17.f)
                    minus_text = true;
                if (command.text == "+" && command.position.x == 165.f)
                    plus_text = true;
            }
        }
        CHECK(track);
        CHECK(minus);
        CHECK(plus);
        CHECK(minus_fill);
        CHECK(plus_fill);
        CHECK(minus_text);
        CHECK(plus_text);
    }

    for (const zgui::Point position : {zgui::Point{20.f, 37.f}, zgui::Point{168.f, 107.f}}) {
        harness.input->set_mouse(position);
        harness.input->press();
        CHECK(group.handle_mouse_press(position));
        group.update();
        harness.input->release();
        group.update();
    }
    CHECK_EQ(integer->get_value(), 24);
    CHECK_NEAR(floating->get_value(), 13.47f, 0.001f);
}
