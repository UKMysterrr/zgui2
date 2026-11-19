// SPDX-License-Identifier: MIT

#include "zgui.hh"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>
#include <optional>
#include <string>

namespace {

void check(bool condition) {
    if (!condition)
        std::abort();
}

class TestGraphics final : public zgui::Graphics {
public:
    void draw() override {}
    void reset() override {}
    void set_clip_rect(int, int, int, int) override {}
    void reset_clip_rect() override {}
    void draw_text(const std::string&, zgui::Point, zgui::Color) override {}
    float measure_text_width(const std::string& text) const override {
        return static_cast<float>(text.size());
    }
};

class TestInput final : public zgui::Input {
public:
    bool key_pressed(int) const override { return false; }
    bool mouse_over(zgui::Point, zgui::Point) const override { return false; }
    zgui::Point mouse_position() const override { return {}; }
    bool mouse_button_pressed() const override { return false; }
    int mouse_wheel_delta() const override { return 0; }
    bool pop_text_input(char&) override { return false; }
    std::optional<int> poll_pressed_key() override { return std::nullopt; }
    std::string key_name(int) const override { return {}; }
};

void test_invalid_layout_state_is_rejected() {
    zgui::UIElement element;
    element.set_position({10.f, 20.f});
    element.set_position({std::numeric_limits<float>::quiet_NaN(), 20.f});
    check(element.get_position().x == 10.f);
    check(element.get_position().y == 20.f);

    element.set_size({100.f, 50.f});
    element.set_size({-1.f, 50.f});
    check(element.get_size().x == 100.f);
    check(element.get_size().y == 50.f);

    element.set_horizontal_constraints(0.f, 1.f, 0.f, 0.f);
    element.set_horizontal_constraints(std::numeric_limits<float>::quiet_NaN(), 1.f, 0.f, 0.f);
    check(element.get_position().x == 10.f);
}

void test_tab_and_group_widget_counts() {
    auto tabs = std::make_shared<zgui::TabBar>();
    auto first = tabs->add_tab("First");
    first->add_child(std::make_shared<zgui::Label>("A"));
    check(tabs->widget_count() == 3);

    auto group = std::make_shared<zgui::GroupBox>("Group");
    group->set_sub_tabs(std::make_shared<zgui::SubTabBar>());
    check(group->widget_count() == 2);
}

void test_empty_tab_layout_is_valid() {
    zgui::WindowManager manager;
    auto window = std::make_shared<zgui::Window>("Window");
    window->set_size({320.f, 240.f});
    auto tabs = std::make_shared<zgui::TabBar>();
    tabs->set_position({8.f, 28.f});
    tabs->set_size({304.f, 204.f});
    tabs->add_tab("Empty");
    window->add_child(tabs);
    manager.add_window(window);

    std::string error;
    check(manager.validate_layout(&error));
    check(error.empty());
}

void test_window_minimum_size_rejects_invalid_values() {
    zgui::Window window("Window");
    window.set_minimum_size(320.f, 240.f);
    window.set_minimum_size(std::numeric_limits<float>::infinity(), 240.f);
    check(window.get_minimum_size().x == 320.f);
    check(window.get_minimum_size().y == 240.f);
}

} // namespace

int main() {
    auto graphics = std::make_shared<TestGraphics>();
    auto input = std::make_shared<TestInput>();
    zgui::core::initialize(graphics, input);

    test_invalid_layout_state_is_rejected();
    test_tab_and_group_widget_counts();
    test_empty_tab_layout_is_valid();
    test_window_minimum_size_rejects_invalid_values();

    zgui::core::shutdown();
    return 0;
}
