// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "../support/test_context.hh"
#include "../support/test_harness.hh"

TEST(label_alignment_color_and_truncation) {
    zgui_test::Harness harness;
    harness.graphics->set_character_width(5.f);

    zgui::Label label("abcd", zgui::TextAlignment::center);
    label.set_position({10.f, 20.f});
    label.set_size({50.f, 16.f});
    label.draw();
    CHECK_EQ(harness.graphics->commands().back().position.x, 25.f);

    label.set_alignment(zgui::TextAlignment::right);
    harness.graphics->clear_commands();
    label.draw();
    CHECK_EQ(harness.graphics->commands().back().position.x, 40.f);

    const zgui::Color override_color(4, 5, 6);
    label.set_color(override_color);
    harness.graphics->clear_commands();
    label.draw();
    CHECK_EQ(harness.graphics->commands().back().color.r, override_color.r);
    label.clear_color_override();
    harness.graphics->clear_commands();
    label.draw();
    CHECK_EQ(harness.graphics->commands().back().color.r, zgui::core::active_theme.text_primary.r);

    label.set_size({10.f, 16.f});
    harness.graphics->clear_commands();
    label.draw();
    CHECK_EQ(harness.graphics->commands().back().text, "..");
}

TEST(separator_draws_horizontal_labeled_and_vertical_rules) {
    zgui_test::Harness harness;
    zgui::Separator horizontal(zgui::SeparatorOrientation::horizontal, "OR");
    horizontal.set_position({10.f, 20.f});
    horizontal.set_size({100.f, 16.f});
    horizontal.draw();
    CHECK_EQ(harness.graphics->texts().size(), 1u);
    CHECK_EQ(harness.graphics->texts().front(), "OR");
    CHECK_EQ(harness.graphics->commands().size(), 3u);
    CHECK_EQ(harness.graphics->commands()[0].topology, zgui::VertexTopology::LINE_LIST);
    CHECK_EQ(harness.graphics->commands()[0].vertices[0].position.y, 28.f);
    CHECK_EQ(harness.graphics->commands()[2].topology, zgui::VertexTopology::LINE_LIST);

    zgui::Separator vertical(zgui::SeparatorOrientation::vertical);
    vertical.set_position({4.f, 5.f});
    vertical.set_size({12.f, 40.f});
    harness.graphics->clear_commands();
    vertical.draw();
    CHECK(harness.graphics->texts().empty());
    CHECK_EQ(harness.graphics->commands().size(), 1u);
    CHECK_EQ(harness.graphics->commands()[0].vertices[0].position.x, 10.f);
}

TEST(button_requires_inside_release_and_calls_replaced_callback_once) {
    zgui_test::Harness harness;
    int first_callback_count = 0;
    int second_callback_count = 0;
    zgui::Button button("button", [&first_callback_count]() { ++first_callback_count; });
    button.set_size({60.f, 20.f});

    CHECK(button.handle_mouse_press({10.f, 10.f}));
    harness.input->press();
    button.update();
    CHECK_EQ(first_callback_count, 0);
    harness.input->set_mouse({80.f, 10.f});
    harness.input->release();
    button.update();
    CHECK_EQ(first_callback_count, 0);

    button.set_on_click([&second_callback_count]() { ++second_callback_count; });
    harness.input->set_mouse({10.f, 10.f});
    CHECK(button.handle_mouse_press({10.f, 10.f}));
    harness.input->press();
    button.update();
    harness.input->release();
    button.update();
    CHECK_EQ(first_callback_count, 0);
    CHECK_EQ(second_callback_count, 1);
}

TEST(checkbox_toggles_once_and_draws_external_value_changes) {
    zgui_test::Harness harness;
    bool value = false;
    int changes = 0;
    zgui::CheckBox check_box("check", value, [&changes]() { ++changes; });
    check_box.set_size({100.f, 20.f});
    CHECK(check_box.handle_mouse_press({2.f, 2.f}));
    CHECK(value);
    CHECK_EQ(changes, 1);
    value = false;
    harness.graphics->clear_commands();
    check_box.draw();
    const std::size_t unchecked_fill_count = harness.graphics->count_fill_rects();
    value = true;
    harness.graphics->clear_commands();
    check_box.draw();
    CHECK(harness.graphics->count_fill_rects() > unchecked_fill_count);
}

TEST(toggle_button_toggles_on_inside_release_and_notifies_changes) {
    zgui_test::Harness harness;
    bool value = false;
    int changes = 0;
    zgui::ToggleButton toggle("pin", value, [&changes]() { ++changes; });
    toggle.set_size({80.f, 20.f});

    CHECK(toggle.handle_mouse_press({10.f, 10.f}));
    harness.input->press();
    toggle.update();
    CHECK(!value);
    harness.input->release();
    toggle.update();
    CHECK(value);
    CHECK_EQ(changes, 1);

    CHECK(toggle.handle_mouse_press({10.f, 10.f}));
    harness.input->press();
    toggle.update();
    harness.input->set_mouse({90.f, 10.f});
    harness.input->release();
    toggle.update();
    CHECK(value);
    CHECK_EQ(changes, 1);

    toggle.set_value(false);
    toggle.set_value(false);
    CHECK(!toggle.get_value());
    CHECK_EQ(changes, 2);
}

TEST(radio_group_selection_clamps_invalid_input_and_owns_bound_index) {
    zgui_test::Harness harness;
    std::size_t selected = 0;
    int changes = 0;
    zgui::RadioGroup radios({"first", "second", "third"}, selected,
                            [&changes](std::size_t) { ++changes; });
    radios.set_size({120.f, 54.f});
    CHECK(radios.handle_mouse_press({20.f, 20.f}));
    CHECK_EQ(selected, 1u);
    CHECK_EQ(changes, 1);
    CHECK(!radios.set_selected_index(3));
    CHECK_EQ(radios.get_selected_index(), 1u);

    auto owned_radios = [&]() {
        const auto bound_index = zgui::bound(std::size_t{2});
        return std::make_shared<zgui::RadioGroup>(std::vector<std::string>{"one", "two", "three"},
                                                  bound_index);
    }();
    CHECK_EQ(owned_radios->get_selected_index(), 2u);
    owned_radios->set_size({120.f, 54.f});
    owned_radios->draw();
    CHECK(!harness.graphics->commands().empty());
}

TEST(progress_bar_clamps_bound_values_and_draws_fraction_and_text) {
    zgui_test::Harness harness;
    float value = -1.f;
    zgui::ProgressBar progress(value);
    progress.set_size({100.f, 20.f});
    CHECK_EQ(progress.get_value(), 0.f);
    progress.set_value(1.5f);
    CHECK_EQ(value, 1.f);

    value = 0.5f;
    progress.set_text("Half");
    progress.draw();
    CHECK_EQ(harness.graphics->texts().front(), "Half");
    bool found_half_fill = false;
    for (const auto& command : harness.graphics->commands()) {
        for (const auto& vertex : command.vertices) {
            if (vertex.color.r == zgui::core::active_theme.accent.r &&
                vertex.color.g == zgui::core::active_theme.accent.g &&
                vertex.color.b == zgui::core::active_theme.accent.b && vertex.position.x == 1.f) {
                for (const auto& far_vertex : command.vertices) {
                    if (far_vertex.position.x == 50.f)
                        found_half_fill = true;
                }
            }
        }
    }
    CHECK(found_half_fill);

    auto owned_progress = [&]() {
        const auto bound_value = zgui::bound(0.75f);
        return std::make_shared<zgui::ProgressBar>(bound_value);
    }();
    CHECK_EQ(owned_progress->get_value(), 0.75f);
    progress.set_value(-1.f);
    CHECK_EQ(progress.get_value(), 0.f);
}

TEST(tooltip_delay_overlay_exclusivity_and_viewport_clamping) {
    zgui_test::Harness harness;
    harness.graphics->set_viewport_size({100.f, 80.f});
    auto window = std::make_shared<zgui::Window>("Main");
    window->set_size({100.f, 80.f});
    auto tooltip = std::make_shared<zgui::Tooltip>("Tip");
    tooltip->set_position({70.f, 55.f});
    tooltip->set_size({20.f, 15.f});
    auto competing_tooltip = std::make_shared<zgui::Tooltip>("Other");
    competing_tooltip->set_position({70.f, 55.f});
    competing_tooltip->set_size({20.f, 15.f});
    window->add_child(tooltip);
    window->add_child(competing_tooltip);
    harness.manager.add_window(window);
    harness.input->set_mouse({75.f, 60.f});

    harness.manager.update();
    CHECK(!harness.manager.has_overlays());
    tooltip->set_delay(std::chrono::milliseconds::zero());
    harness.manager.update();
    CHECK(harness.manager.has_overlays());
    harness.manager.draw();

    const auto& texts = harness.graphics->commands();
    CHECK_EQ(harness.graphics->texts().back(), "Tip");
    bool found_clamped_text = false;
    for (const auto& command : texts) {
        if (command.type == zgui_test::GraphicsCommandType::Text && command.text == "Tip") {
            found_clamped_text = command.position.x <= 100.f && command.position.y < 55.f;
        }
    }
    CHECK(found_clamped_text);

    harness.input->set_mouse({95.f, 79.f});
    harness.manager.update();
    CHECK(!harness.manager.has_overlays());
}

TEST(text_input_cursor_editing_and_focus) {
    zgui_test::Harness harness;
    std::string value = "ab";
    int changes = 0;
    zgui::TextInput input(value, [&changes]() { ++changes; });
    zgui::core::focused_text_input = &input;

    harness.input->set_cursor_keys(true, false, false, false, false);
    harness.input->type("X");
    input.update();
    CHECK_EQ(value, "aXb");
    CHECK_EQ(changes, 1);

    harness.input->set_cursor_keys(false, false, true, false, false);
    harness.input->type("Y");
    input.update();
    CHECK_EQ(value, "YaXb");

    harness.input->set_cursor_keys(false, false, false, false, true);
    input.update();
    CHECK_EQ(value, "YXb");

    zgui::core::focused_text_input = nullptr;
    harness.input->type("ignored");
    input.update();
    CHECK_EQ(value, "YXb");
}

TEST(text_input_focus_cursor_controls_and_backspace) {
    zgui_test::Harness harness;
    std::string first_value = "abc";
    std::string second_value = "12";
    int first_changes = 0;
    int second_changes = 0;
    zgui::TextInput first(first_value, [&first_changes]() { ++first_changes; });
    zgui::TextInput second(second_value, [&second_changes]() { ++second_changes; });
    first.set_size({100.f, 20.f});
    second.set_size({100.f, 20.f});

    CHECK(first.handle_mouse_press({35.f, 10.f}));
    CHECK_EQ(zgui::core::focused_text_input, &first);
    harness.input->set_cursor_keys(true, false, false, false, false);
    harness.input->type("X");
    first.update();
    CHECK_EQ(first_value, "abXc");
    CHECK_EQ(first_changes, 1);

    harness.input->set_cursor_keys(false, false, false, true, false);
    harness.input->type("Y");
    first.update();
    CHECK_EQ(first_value, "abXcY");
    harness.input->type("\b");
    first.update();
    CHECK_EQ(first_value, "abXc");
    CHECK_EQ(first_changes, 3);

    CHECK(first.handle_mouse_press({3.f, 10.f}));
    harness.input->set_cursor_keys(false, true, false, false, false);
    harness.input->type("Q");
    first.update();
    CHECK_EQ(first_value, "aQbXc");
    CHECK_EQ(first_changes, 4);

    CHECK(second.handle_mouse_press({20.f, 10.f}));
    CHECK_EQ(zgui::core::focused_text_input, &second);
    harness.input->type("Z");
    first.update();
    second.update();
    CHECK_EQ(first_value, "aQbXc");
    CHECK_EQ(second_value, "12Z");
    CHECK_EQ(second_changes, 1);

    harness.input->set_mouse({200.f, 200.f});
    harness.input->press();
    harness.manager.update();
    CHECK(zgui::core::focused_text_input == nullptr);
    harness.input->release();
    harness.manager.update();
}

TEST(key_bind_listens_for_one_key_and_escape_cancels) {
    zgui_test::Harness harness;
    int key = 0;
    int changes = 0;
    zgui::KeyBind key_bind("key", key, [&changes]() { ++changes; });
    key_bind.set_size({100.f, 20.f});

    CHECK(key_bind.handle_mouse_press({5.f, 5.f}));
    harness.input->press_key(42);
    key_bind.update();
    CHECK_EQ(key, 42);
    CHECK_EQ(changes, 1);

    CHECK(key_bind.handle_mouse_press({5.f, 5.f}));
    harness.input->escape();
    key_bind.update();
    harness.input->press_key(7);
    key_bind.update();
    CHECK_EQ(key, 42);
    CHECK_EQ(changes, 1);
    harness.graphics->clear_commands();
    key_bind.draw();
    bool found_key_name = false;
    for (const auto& text : harness.graphics->texts())
        found_key_name = found_key_name || text == "[42]";
    CHECK(found_key_name);
}

TEST(combo_box_popup_flips_above_near_bottom_edge) {
    zgui_test::Harness harness;
    harness.graphics->set_viewport_size({200.f, 100.f});
    zgui::core::window_manager = &harness.manager;
    zgui::ComboBox combo({"first", "second"});
    combo.set_position({10.f, 80.f});
    combo.set_size({40.f, 20.f});

    CHECK(combo.handle_mouse_press({12.f, 82.f}));
    harness.input->set_mouse({20.f, 71.f});
    harness.input->press();
    harness.manager.update();
    CHECK_EQ(combo.get_selected_index(), 1u);
}

TEST(combo_box_binding_selection_and_empty_items) {
    zgui_test::Harness harness;
    std::size_t selected = 1;
    zgui::ComboBox combo({"first", "second"}, 0);
    int changes = 0;
    combo.set_on_change([&changes](std::size_t) { ++changes; });
    combo.bind_selected_index(selected);
    CHECK_EQ(combo.get_selected_index(), 1u);
    CHECK_EQ(selected, 1u);

    CHECK(combo.set_selected_index(0));
    CHECK_EQ(selected, 0u);
    CHECK_EQ(changes, 1);
    selected = 1;
    CHECK_EQ(combo.get_selected_index(), 1u);
    CHECK_EQ(combo.get_selected_item(), "second");

    combo.set_items({});
    CHECK_EQ(combo.get_selected_index(), 0u);
    CHECK(combo.get_selected_item().empty());
    CHECK(!combo.set_selected_index(0));
}

TEST(combo_box_popup_selection_callback_and_escape_or_outside_dismissal) {
    zgui_test::Harness harness;
    zgui::ComboBox combo({"first", "second"});
    combo.set_position({0.f, 0.f});
    combo.set_size({100.f, 20.f});
    int selected = -1;
    combo.set_on_change([&selected](std::size_t index) { selected = static_cast<int>(index); });

    CHECK(combo.handle_mouse_press({5.f, 5.f}));
    harness.input->set_mouse({10.f, 45.f});
    harness.input->press();
    harness.manager.update();
    CHECK_EQ(selected, 1);
    CHECK(!harness.manager.has_overlays());
    harness.input->release();
    harness.manager.update();

    CHECK(combo.handle_mouse_press({5.f, 5.f}));
    harness.input->escape();
    harness.manager.update();
    CHECK(!harness.manager.has_overlays());
    harness.input->end_frame();

    CHECK(combo.handle_mouse_press({5.f, 5.f}));
    harness.input->set_mouse({300.f, 300.f});
    harness.input->press();
    harness.manager.update();
    CHECK(!harness.manager.has_overlays());
    CHECK_EQ(combo.get_selected_index(), 1u);
}

TEST(multi_combo_resizes_values_toggles_popup_and_draws_summary) {
    zgui_test::Harness harness;
    std::vector<bool> values{true};
    int changes = 0;
    zgui::MultiComboBox combo({"first"}, values, [&changes]() { ++changes; });
    combo.set_items({"first", "second", "third"});
    CHECK_EQ(combo.values().size(), 3u);
    CHECK(combo.values()[0]);
    CHECK(!combo.values()[1]);

    combo.set_position({0.f, 0.f});
    combo.set_size({200.f, 20.f});
    CHECK(combo.handle_mouse_press({5.f, 5.f}));
    harness.input->set_mouse({10.f, 45.f});
    harness.input->press();
    harness.manager.update();
    CHECK(combo.values()[1]);
    CHECK_EQ(changes, 1);
    CHECK(harness.manager.has_overlays());

    combo.set_items({"first", "second"});
    CHECK_EQ(combo.values().size(), 2u);
    harness.graphics->clear_commands();
    combo.draw();
    bool found_summary = false;
    for (const auto& text : harness.graphics->texts())
        found_summary = found_summary || text == "first, second";
    CHECK(found_summary);
    harness.graphics->set_character_width(5.f);
    combo.set_size({30.f, 20.f});
    harness.graphics->clear_commands();
    combo.draw();
    CHECK(harness.graphics->texts().back() != "first, second");
    combo.set_items({"first"});
    CHECK_EQ(combo.values().size(), 1u);
}

TEST(viewport_requires_inside_press_and_keeps_captured_drag_outside) {
    zgui_test::Harness harness;
    zgui::Viewport viewport;
    viewport.set_position({10.f, 10.f});
    viewport.set_size({20.f, 20.f});
    std::vector<zgui::ViewportInput> events;
    viewport.set_on_input([&events](const zgui::ViewportInput& event) { events.push_back(event); });

    harness.input->set_mouse({0.f, 0.f});
    harness.input->press();
    viewport.update();
    harness.input->set_mouse({12.f, 12.f});
    viewport.update();
    CHECK(events.empty());
    harness.input->release();
    viewport.update();

    CHECK(viewport.handle_mouse_press({12.f, 12.f}));
    harness.input->press();
    viewport.update();
    CHECK_EQ(events.size(), 1u);
    CHECK(events.back().rotating);

    harness.input->set_mouse({40.f, 40.f});
    viewport.update();
    CHECK_EQ(events.size(), 2u);
    CHECK(events.back().rotating);

    harness.input->release();
    viewport.update();
    CHECK(viewport.handle_mouse_press({12.f, 12.f}));
    harness.input->press(zgui::Input::MouseButton::middle);
    viewport.update();
    CHECK_EQ(events.size(), 3u);
    CHECK(events.back().panning);
    CHECK(!events.back().rotating);
    harness.input->release(zgui::Input::MouseButton::middle);
    viewport.update();
    harness.input->set_mouse({12.f, 12.f});
    harness.input->scroll(2);
    harness.input->double_click();
    viewport.update();
    CHECK_EQ(events.size(), 4u);
    CHECK_EQ(events.back().wheel_delta, 2);
    CHECK(events.back().double_clicked);
}

TEST(viewport_render_callback_receives_clipped_screen_rectangle) {
    zgui_test::Harness harness;
    zgui::UIElement parent;
    parent.set_position({10.f, 15.f});
    zgui::Viewport viewport;
    viewport.set_parent(&parent);
    viewport.set_position({5.f, 7.f});
    viewport.set_size({50.f, 30.f});
    zgui::Rect rendered;
    viewport.set_on_render([&rendered](int x, int y, int width, int height) {
        rendered = {{static_cast<float>(x), static_cast<float>(y)},
                    {static_cast<float>(width), static_cast<float>(height)}};
    });

    viewport.draw();
    CHECK_EQ(rendered.position.x, 17.f);
    CHECK_EQ(rendered.position.y, 24.f);
    CHECK_EQ(rendered.size.x, 46.f);
    CHECK_EQ(rendered.size.y, 26.f);
    CHECK_EQ(harness.graphics->clip_depth(), 0u);
}

TEST(window_manager_reorders_windows_when_always_on_top_changes) {
    zgui_test::Harness harness;
    auto regular = std::make_shared<zgui::Window>("regular");
    auto topmost = std::make_shared<zgui::Window>("topmost");
    topmost->set_always_on_top(true);
    harness.manager.add_window(regular);
    harness.manager.add_window(topmost);

    regular->set_always_on_top(true);
    topmost->set_always_on_top(false);
    harness.manager.update();
    CHECK_EQ(harness.manager.windows().back(), regular);

    regular->set_always_on_top(false);
    topmost->set_always_on_top(true);
    harness.manager.draw();
    CHECK_EQ(harness.manager.windows().back(), topmost);
}

TEST(color_picker_hue_alpha_and_captured_drag_edges) {
    zgui_test::Harness harness;
    zgui::Color color(255, 0, 0, 255);
    int changes = 0;
    zgui::ColorPicker picker(color, [&changes]() { ++changes; });
    picker.set_position({10.f, 10.f});

    CHECK(picker.handle_mouse_press({189.f, 10.f + 28.f + 128.f / 3.f}));
    CHECK(color.g >= 254);
    CHECK(color.r <= 1);
    CHECK_EQ(changes, 1);

    CHECK(picker.handle_mouse_press({10.f + 10.f + 80.f, 10.f + 170.f}));
    CHECK(color.a >= 126 && color.a <= 129);
    CHECK_EQ(changes, 2);

    CHECK(picker.handle_mouse_press({170.f, 38.f}));
    harness.input->set_mouse({-100.f, -100.f});
    harness.input->press();
    picker.update();
    CHECK_EQ(color.r, 255);
    CHECK_EQ(color.g, 255);
    CHECK_EQ(color.b, 255);
    CHECK_EQ(color.a, 128);
    harness.input->release();
    picker.update();
}
