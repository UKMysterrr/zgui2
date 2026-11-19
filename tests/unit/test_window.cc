// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "../support/test_context.hh"
#include "../support/test_harness.hh"

TEST(window_manager_validates_layout_and_reports_failure_kind) {
    zgui_test::Harness harness;
    auto window = std::make_shared<zgui::Window>("window");
    window->set_size({200.f, 120.f});
    auto first = std::make_shared<zgui::UIElement>();
    first->set_title("first");
    first->set_position({2.f, 20.f});
    first->set_size({20.f, 10.f});
    window->add_child(first);
    harness.manager.add_window(window);

    std::string error;
    CHECK(harness.manager.validate_layout(&error));
    CHECK(error.empty());

    auto overlapping = std::make_shared<zgui::UIElement>();
    overlapping->set_title("overlapping");
    overlapping->set_position({10.f, 25.f});
    overlapping->set_size({20.f, 10.f});
    window->add_child(overlapping);
    CHECK(!harness.manager.validate_layout(&error));
    CHECK(error.find("overlap") != std::string::npos);
    window->remove_child(overlapping.get());

    auto outside = std::make_shared<zgui::UIElement>();
    outside->set_title("outside");
    outside->set_position({190.f, 20.f});
    outside->set_size({20.f, 10.f});
    window->add_child(outside);
    CHECK(!harness.manager.validate_layout(&error));
    CHECK(error.find("exceeds parent") != std::string::npos);
    window->remove_child(outside.get());

    auto negative = std::make_shared<zgui::UIElement>();
    negative->set_title("negative");
    negative->set_position({30.f, 20.f});
    negative->set_size({-1.f, 10.f});
    window->add_child(negative);
    CHECK(!harness.manager.validate_layout(&error));
    CHECK(error.find("invalid size") != std::string::npos);
}

TEST(window_drag_resize_minimum_size_and_status_checkbox) {
    zgui_test::Harness harness;
    zgui::Window window("window");
    window.set_position({10.f, 10.f});
    window.set_size({200.f, 120.f});

    CHECK(window.handle_mouse_press({20.f, 15.f}));
    harness.input->set_mouse({40.f, 50.f});
    harness.input->press();
    window.update();
    CHECK_EQ(window.get_position().x, 30.f);
    CHECK_EQ(window.get_position().y, 45.f);
    harness.input->release();
    window.update();

    CHECK(window.handle_mouse_press({225.f, 160.f}));
    harness.input->set_mouse({255.f, 180.f});
    harness.input->press();
    window.update();
    CHECK_EQ(window.get_size().x, 230.f);
    CHECK_EQ(window.get_size().y, 140.f);
    harness.input->release();
    window.update();

    bool enabled = false;
    window.set_status_bar_visible(true);
    window.set_status_checkbox("enabled", enabled);
    CHECK(window.handle_mouse_press({40.f, 178.f}));
    CHECK(enabled);

    window.set_visible(false);
    harness.graphics->clear_commands();
    window.draw();
    CHECK(harness.graphics->commands().empty());
    CHECK(!window.handle_mouse_press({40.f, 50.f}));
}

TEST(window_minimum_size_from_content_includes_child_bounds) {
    zgui_test::Harness harness;
    zgui::Window window("window");
    auto child = std::make_shared<zgui::UIElement>();
    child->set_position({20.f, 30.f});
    child->set_size({100.f, 40.f});
    window.add_child(child);
    window.set_minimum_size_from_content();

    CHECK_EQ(window.get_minimum_size().x, 128.f);
    CHECK_EQ(window.get_minimum_size().y, 72.f);
}

TEST(window_modes_status_indicators_and_resizable_state) {
    zgui_test::Harness harness;
    zgui::Window window("window");
    window.set_size({200.f, 120.f});
    window.set_resizable(false);
    CHECK(!window.is_resizable());
    window.set_title_bar_visible(false);
    window.set_status_bar_visible(true);
    window.set_status_checkbox("enabled", zgui::bound(true));
    window.set_status_indicators({zgui::Color(255, 0, 0), zgui::Color(0, 255, 0)});
    harness.graphics->clear_commands();
    window.draw();
    CHECK(!harness.graphics->texts().empty());

    window.set_frameless(true);
    CHECK(!window.is_resizable());
    CHECK_EQ(window.get_minimum_size().x, 1.f);
    CHECK_EQ(window.get_minimum_size().y, 1.f);
}

TEST(window_manager_z_order_and_outside_overlay_swallow_clicks) {
    zgui_test::Harness harness;
    auto lower = std::make_shared<zgui::Window>("lower");
    auto upper = std::make_shared<zgui::Window>("upper");
    lower->set_size({160.f, 100.f});
    upper->set_size({160.f, 100.f});
    lower->set_position({0.f, 0.f});
    upper->set_position({50.f, 20.f});
    harness.manager.add_window(lower);
    harness.manager.add_window(upper);

    harness.input->set_mouse({20.f, 30.f});
    harness.input->press();
    harness.manager.update();
    CHECK_EQ(harness.manager.windows().back(), lower);
    harness.input->release();
    harness.manager.update();

    int clicks = 0;
    auto button = std::make_shared<zgui::Button>("under", [&clicks]() { ++clicks; });
    button->set_position({10.f, 30.f});
    button->set_size({80.f, 20.f});
    lower->add_child(button);
    auto overlay = std::make_shared<zgui::UIElement>();
    overlay->set_position({300.f, 300.f});
    overlay->set_size({30.f, 30.f});
    harness.manager.show_overlay(overlay);

    harness.input->set_mouse({20.f, 35.f});
    harness.input->press();
    harness.manager.update();
    CHECK(!harness.manager.has_overlays());
    harness.input->release();
    harness.manager.update();
    CHECK_EQ(clicks, 0);
}
