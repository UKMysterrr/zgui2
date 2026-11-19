// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "../support/test_context.hh"
#include "../support/test_harness.hh"

TEST(tab_bar_with_no_pages_draws_and_updates_safely) {
    zgui_test::Harness harness;
    zgui::TabBar tabs;
    tabs.set_size({80.f, 40.f});
    CHECK_EQ(tabs.tab_count(), 0u);
    CHECK(!tabs.set_active_tab(0));
    tabs.update();
    tabs.draw();
    CHECK_EQ(harness.graphics->clip_depth(), 0u);
}

TEST(tab_bar_switches_pages_and_reports_widget_counts) {
    zgui_test::Harness harness;
    zgui::TabBar tabs;
    tabs.set_position({10.f, 20.f});
    tabs.set_size({200.f, 100.f});
    auto first_page = tabs.add_tab("first");
    auto second_page = tabs.add_tab("second");
    auto first = std::make_shared<zgui::Label>("first page");
    auto second = std::make_shared<zgui::Label>("second page");
    first_page->add_child(first);
    second_page->add_child(second);
    first->set_position({4.f, 4.f});
    first->set_size({40.f, 12.f});
    second->set_position({4.f, 4.f});
    second->set_size({40.f, 12.f});
    tabs.set_size({200.f, 100.f});

    CHECK_EQ(tabs.tab_count(), 2u);
    CHECK_EQ(tabs.tab_widget_count(0), 2u);
    CHECK_EQ(tabs.tab_visible_widget_count(0), 2u);
    CHECK_EQ(tabs.tab_visible_widget_count(1), 0u);
    CHECK(tabs.handle_mouse_press({70.f, 25.f}));
    CHECK_EQ(tabs.get_active_tab(), 1u);
    CHECK_EQ(tabs.tab_visible_widget_count(1), 2u);
    CHECK_EQ(tabs.tab_title(1), "second");
    CHECK(tabs.child_clip_rect().position.y > tabs.get_absolute_position().y);

    harness.graphics->clear_commands();
    tabs.draw();
    bool found_page_clip = false;
    for (const auto& command : harness.graphics->commands()) {
        if (command.type == zgui_test::GraphicsCommandType::SetClip && command.x == 10 &&
            command.y ==
                static_cast<int>(tabs.get_absolute_position().y + zgui::spacing::kTabBarHeight) &&
            command.width == 200 &&
            command.height == static_cast<int>(100.f - zgui::spacing::kTabBarHeight)) {
            found_page_clip = true;
        }
    }
    CHECK(found_page_clip);
}
TEST(tab_bar_page_wheel_scroll_resets_when_switching_tabs) {
    zgui_test::Harness harness;
    zgui::TabBar tabs;
    tabs.set_size({200.f, 100.f});
    auto page = tabs.add_tab("page");
    auto child = std::make_shared<zgui::Label>("far row");
    child->set_position({0.f, 200.f});
    child->set_size({50.f, 12.f});
    page->add_child(child);
    tabs.set_size({200.f, 100.f});
    const float initial_y = child->get_absolute_position().y;

    harness.input->set_mouse({10.f, 50.f});
    harness.input->scroll(-1);
    CHECK(tabs.handles_mouse_wheel({10.f, 50.f}));
    tabs.update();
    CHECK_EQ(child->get_absolute_position().y, initial_y - 24.f);

    harness.input->end_frame();
    harness.input->scroll(-1000);
    tabs.update();
    CHECK(!tabs.handles_mouse_wheel({10.f, 50.f}));
    harness.input->end_frame();
    harness.input->scroll(1000);
    CHECK(tabs.handles_mouse_wheel({10.f, 50.f}));
    tabs.update();
    CHECK(!tabs.handles_mouse_wheel({10.f, 50.f}));

    tabs.add_tab("other");
    tabs.set_active_tab(1);
    CHECK(tabs.set_active_tab(0));
    CHECK_EQ(child->get_absolute_position().y, initial_y);
}

TEST(tab_bar_narrow_layout_keeps_all_tabs_clickable) {
    zgui_test::Harness harness;
    zgui::TabBar tabs;
    tabs.set_size({30.f, 80.f});
    for (int index = 0; index < 10; ++index)
        tabs.add_tab("Long title " + std::to_string(index));

    CHECK(tabs.handle_mouse_press({29.f, 5.f}));
    CHECK_EQ(tabs.get_active_tab(), 9u);
    tabs.draw();
    CHECK_EQ(harness.graphics->clip_depth(), 0u);
}
