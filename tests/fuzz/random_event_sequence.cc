// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "../support/test_context.hh"
#include "../support/test_harness.hh"

#include <cmath>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

TEST(random_event_sequence_runs_100000_frames) {
    constexpr std::uint32_t seed = 0x5eed1234u;
    std::mt19937 random(seed);
    std::uniform_int_distribution<int> x_position(-40, 940);
    std::uniform_int_distribution<int> y_position(-40, 1040);
    std::uniform_int_distribution<int> event(0, 99);
    std::uniform_int_distribution<int> wheel(-5, 5);
    std::uniform_int_distribution<int> key(1, 255);
    std::uniform_int_distribution<int> character(0, 255);

    zgui_test::Harness harness;
    harness.graphics->set_viewport_size({900.f, 1000.f});
    auto window = std::make_shared<zgui::Window>("fuzz");
    window->set_size({900.f, 1000.f});
    harness.manager.add_window(window);

    auto label = std::make_shared<zgui::Label>("label");
    label->set_position({10.f, 20.f});
    label->set_size({120.f, 18.f});
    window->add_child(label);
    auto button = std::make_shared<zgui::Button>("button");
    button->set_position({10.f, 45.f});
    button->set_size({120.f, 20.f});
    window->add_child(button);
    bool checked = false;
    auto check_box = std::make_shared<zgui::CheckBox>("check", checked);
    check_box->set_position({10.f, 70.f});
    check_box->set_size({120.f, 20.f});
    window->add_child(check_box);
    float slider_value = 0.5f;
    auto slider = std::make_shared<zgui::Slider<float>>("slider", slider_value, 0.f, 1.f);
    slider->set_position({10.f, 95.f});
    slider->set_size({180.f, 34.f});
    window->add_child(slider);
    auto combo = std::make_shared<zgui::ComboBox>(std::vector<std::string>{"one", "two", "three"});
    combo->set_position({10.f, 135.f});
    combo->set_size({180.f, 20.f});
    window->add_child(combo);
    std::vector<bool> multi_values(3, false);
    auto multi_combo = std::make_shared<zgui::MultiComboBox>(
        std::vector<std::string>{"one", "two", "three"}, multi_values);
    multi_combo->set_position({10.f, 160.f});
    multi_combo->set_size({180.f, 20.f});
    window->add_child(multi_combo);
    std::string text_value;
    auto text_input = std::make_shared<zgui::TextInput>(text_value);
    text_input->set_position({10.f, 185.f});
    text_input->set_size({180.f, 20.f});
    window->add_child(text_input);
    int bound_key = 0;
    auto key_bind = std::make_shared<zgui::KeyBind>("key", bound_key);
    key_bind->set_position({10.f, 210.f});
    key_bind->set_size({180.f, 20.f});
    window->add_child(key_bind);
    zgui::Color color(255, 0, 0, 255);
    auto color_picker = std::make_shared<zgui::ColorPicker>(color);
    color_picker->set_position({220.f, 20.f});
    window->add_child(color_picker);
    auto swatch = std::make_shared<zgui::ColorSwatch>("swatch", color);
    swatch->set_position({10.f, 240.f});
    swatch->set_size({180.f, 24.f});
    window->add_child(swatch);
    auto color_button = std::make_shared<zgui::ColorButton>("color", color);
    color_button->set_position({10.f, 270.f});
    color_button->set_size({180.f, 40.f});
    window->add_child(color_button);
    auto tabs = std::make_shared<zgui::TabBar>();
    tabs->set_position({10.f, 320.f});
    tabs->set_size({240.f, 180.f});
    auto tab_page = tabs->add_tab("page");
    auto tab_label = std::make_shared<zgui::Label>("tab content");
    tab_label->set_position({4.f, 4.f});
    tab_label->set_size({120.f, 18.f});
    tab_page->add_child(tab_label);
    tabs->set_size({240.f, 180.f});
    window->add_child(tabs);
    auto group = std::make_shared<zgui::GroupBox>("group");
    group->set_position({270.f, 320.f});
    group->set_size({240.f, 180.f});
    auto group_label = std::make_shared<zgui::Label>("group content");
    group_label->set_position({8.f, 26.f});
    group_label->set_size({120.f, 18.f});
    group->add_child(group_label);
    window->add_child(group);
    auto table = std::make_shared<zgui::TableView>();
    table->set_position({10.f, 520.f});
    table->set_size({240.f, 180.f});
    table->set_columns({{"first", 80.f}, {"second", 80.f}});
    for (int row = 0; row < 20; ++row)
        table->add_row(std::vector<std::string>{"first", "second"});
    window->add_child(table);
    auto viewport = std::make_shared<zgui::Viewport>();
    viewport->set_position({270.f, 520.f});
    viewport->set_size({240.f, 180.f});
    viewport->set_on_render([](int, int, int, int) {});
    viewport->set_on_input([](const zgui::ViewportInput&) {});
    window->add_child(viewport);

    bool left_down = false;
    bool middle_down = false;
    bool right_down = false;
    std::string failure;
    for (int frame = 0; frame < 100000; ++frame) {
        harness.input->set_mouse(
            {static_cast<float>(x_position(random)), static_cast<float>(y_position(random))});
        const int action = event(random);
        if (action < 12) {
            left_down = !left_down;
            if (left_down)
                harness.input->press();
            else
                harness.input->release();
        } else if (action < 17) {
            middle_down = !middle_down;
            if (middle_down)
                harness.input->press(zgui::Input::MouseButton::middle);
            else
                harness.input->release(zgui::Input::MouseButton::middle);
        } else if (action < 22) {
            right_down = !right_down;
            if (right_down)
                harness.input->press(zgui::Input::MouseButton::right);
            else
                harness.input->release(zgui::Input::MouseButton::right);
        } else if (action < 32) {
            harness.input->scroll(wheel(random));
        } else if (action < 42) {
            harness.input->type(std::string(1, static_cast<char>(character(random))));
        } else if (action < 50) {
            harness.input->press_key(key(random));
        } else if (action < 53) {
            harness.input->escape();
        } else if (action < 56) {
            harness.input->double_click();
        }

        if (frame % 1000 == 0) {
            window->set_size(
                {static_cast<float>(850 + event(random)), static_cast<float>(950 + event(random))});
            window->set_position(
                {static_cast<float>(event(random)), static_cast<float>(event(random))});
        }
        harness.graphics->clear_commands();
        harness.input->begin_frame();
        harness.manager.update();
        if (frame % 5 == 0)
            harness.manager.draw();
        harness.graphics->flush();
        harness.input->end_frame();
        if (harness.graphics->clip_depth() != 0u) {
            failure = "clip stack is unbalanced";
            break;
        }
        for (const auto& command : harness.graphics->commands()) {
            if (command.type == zgui_test::GraphicsCommandType::SetClip &&
                (command.width < 0 || command.height < 0)) {
                failure = "negative clip dimensions";
                break;
            }
            if (command.type == zgui_test::GraphicsCommandType::Text &&
                (!std::isfinite(command.position.x) || !std::isfinite(command.position.y))) {
                failure = "non-finite text position";
                break;
            }
            for (const auto& vertex : command.vertices) {
                if (!std::isfinite(vertex.position.x) || !std::isfinite(vertex.position.y)) {
                    failure = "non-finite geometry position";
                    break;
                }
            }
            if (!failure.empty())
                break;
        }
        if (!failure.empty())
            break;
    }

    if (!failure.empty())
        std::cerr << "random_event_sequence seed=" << seed << ": " << failure << '\n';
    CHECK(failure.empty());

    while (harness.manager.has_overlays()) {
        harness.input->escape();
        harness.manager.update();
        harness.input->end_frame();
    }
    harness.manager.remove_window(window);
    window.reset();
}
