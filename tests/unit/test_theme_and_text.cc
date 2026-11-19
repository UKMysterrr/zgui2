// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "../support/mock_graphics.hh"
#include "../support/mock_input.hh"
#include "../support/test_context.hh"
#include "../support/test_harness.hh"

#include <memory>
#include <vector>

TEST(core_initialization_shutdown_and_theme_changes_are_visible) {
    auto graphics = std::make_shared<zgui_test::MockGraphics>();
    auto input = std::make_shared<zgui_test::MockInput>();
    zgui::core::initialize(graphics, input);
    CHECK(zgui::core::graphics == graphics);
    CHECK(zgui::core::input == input);

    zgui::Label label("theme");
    label.set_size({100.f, 20.f});
    zgui::Theme changed = zgui::core::default_theme;
    changed.text_primary = zgui::Color(12, 34, 56);
    zgui::core::set_theme(changed);
    label.draw();
    CHECK_EQ(graphics->commands().back().color.r, 12);
    CHECK_EQ(graphics->commands().back().color.g, 34);
    CHECK_EQ(graphics->commands().back().color.b, 56);

    zgui::core::reset_theme();
    label.draw();
    CHECK_EQ(graphics->commands().back().color.r, zgui::core::default_theme.text_primary.r);
    zgui::core::shutdown();
    CHECK(!zgui::core::graphics);
    CHECK(!zgui::core::input);
}

TEST(text_input_accepts_printable_ascii_only) {
    zgui_test::Harness harness;
    std::string value;
    int changes = 0;
    zgui::TextInput input(value, [&changes]() { ++changes; });
    zgui::core::focused_text_input = &input;

    std::string text = "A";
    text.append("\x00\x1f\x7f\x80\xff", 5);
    text.append("\xc3\xa9", 2);
    text += "Z";
    harness.input->type(text);
    input.update();

    CHECK_EQ(value, "AZ");
    CHECK_EQ(changes, 1);
}

TEST(bound_values_are_owned_by_widgets_and_color_popups) {
    zgui_test::Harness harness;

    auto check_value = zgui::bound(false);
    zgui::CheckBox check_box("check", check_value);
    check_box.set_size({100.f, 20.f});
    CHECK(check_box.handle_mouse_press({1.f, 1.f}));
    CHECK(*check_value.get());

    auto slider_value = zgui::bound(2);
    zgui::Slider<int> slider("slider", slider_value, 0, 10);
    slider.set_value(7);
    CHECK_EQ(*slider_value.get(), 7);

    auto selected = zgui::bound(std::size_t{0});
    zgui::ComboBox combo({"first", "second"});
    combo.bind_selected_index(selected);
    CHECK(combo.set_selected_index(1));
    CHECK_EQ(*selected.get(), 1u);

    auto multi_values = zgui::bound(std::vector<bool>{false});
    zgui::MultiComboBox multi({"one", "two"}, multi_values);
    CHECK_EQ(multi.values().size(), 2u);

    auto text_value = zgui::bound(std::string{});
    zgui::TextInput text_input(text_value);
    zgui::core::focused_text_input = &text_input;
    harness.input->type("x");
    text_input.update();
    CHECK_EQ(*text_value.get(), "x");

    auto key_value = zgui::bound(0);
    zgui::KeyBind key_bind("key", key_value);
    key_bind.set_key(42);
    CHECK_EQ(*key_value.get(), 42);

    auto color_value = zgui::bound(zgui::Color(10, 20, 30));
    zgui::ColorPicker picker(color_value);
    CHECK(picker.handle_mouse_press({169.5f, 28.f}));
    CHECK(color_value.get()->r != 10);

    zgui::Window window("window");
    window.set_size({100.f, 100.f});
    window.set_status_bar_visible(true);
    auto status_value = zgui::bound(false);
    window.set_status_checkbox("status", status_value);
    CHECK(window.handle_mouse_press({10.f, 90.f}));
    CHECK(*status_value.get());

    auto popup_value = zgui::bound(zgui::Color(20, 40, 60));
    std::weak_ptr<zgui::Color> weak_popup_value = popup_value.owner();
    auto swatch = std::make_shared<zgui::ColorSwatch>("color", popup_value);
    swatch->set_size({100.f, 20.f});
    popup_value = zgui::Bound<zgui::Color>(std::shared_ptr<zgui::Color>{});
    zgui::core::window_manager = &harness.manager;
    CHECK(swatch->handle_mouse_press({60.f, 5.f}));
    swatch.reset();
    CHECK(!weak_popup_value.expired());
    harness.manager.draw();
    CHECK(!weak_popup_value.expired());

    auto button_color = zgui::bound(zgui::Color(1, 2, 3));
    zgui::ColorButton button("button", button_color);
    button.set_size({100.f, 40.f});
    CHECK(button.handle_mouse_press({10.f, 20.f}));
    CHECK(harness.manager.has_overlays());
}
