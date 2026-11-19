// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "../support/test_context.hh"
#include "../support/test_harness.hh"

TEST(group_box_fit_content_and_subtab_layout) {
    zgui_test::Harness harness;
    zgui::GroupBox group("settings");
    group.set_position({10.f, 10.f});
    group.set_size({300.f, 120.f});
    auto tabs = std::make_shared<zgui::SubTabBar>();
    auto page = tabs->add_tab("general");
    auto label = std::make_shared<zgui::Label>("setting");
    label->set_position({8.f, 8.f});
    label->set_size({100.f, 14.f});
    page->add_child(label);
    group.set_sub_tabs(tabs);
    group.set_fit_content(false);
    group.set_size({300.f, 160.f});

    CHECK_EQ(group.get_sub_tabs(), tabs);
    CHECK_EQ(tabs->get_position().x, zgui::spacing::kSubTabStripInset);
    CHECK_EQ(tabs->get_position().y, zgui::spacing::kGroupSubTabTop);
    CHECK_EQ(tabs->get_size().x, 300.f - zgui::spacing::kSubTabStripInset * 2.f);
    CHECK(group.content_right() >=
          zgui::spacing::kSubTabStripInset + label->get_position().x + label->get_size().x);
    CHECK(group.widget_count() >= tabs->widget_count());
    CHECK(group.visible_widget_count() >= tabs->visible_widget_count());

    group.set_fit_content(true);
    group.fit_to_content();
    CHECK(group.get_size().y >= zgui::spacing::kGroupSubTabTop + zgui::spacing::kSubTabBarHeight +
                                    zgui::spacing::kMinimumSubTabPageHeight +
                                    zgui::spacing::kSubTabBarBottomGap);
    harness.graphics->clear_commands();
    group.draw();
    CHECK_EQ(harness.graphics->clip_depth(), 0u);
}

TEST(group_stack_layout_places_three_color_buttons) {
    zgui_test::Harness harness;
    zgui::GroupBox group("colors");
    group.set_size({200.f, 100.f});
    auto first = std::make_shared<zgui::ColorButton>("first", zgui::bound(zgui::Color{}));
    auto second = std::make_shared<zgui::ColorButton>("second", zgui::bound(zgui::Color{}));
    auto third = std::make_shared<zgui::ColorButton>("third", zgui::bound(zgui::Color{}));
    zgui::StackLayout layout(group, 20.f, 8.f, 2.f);

    layout.add_three_column_row(first, second, third, 52.f);
    CHECK_EQ(first->get_position().x, 8.f);
    CHECK_NEAR(first->get_size().x, 60.f, 0.001f);
    CHECK_NEAR(second->get_position().x, 70.f, 0.001f);
    CHECK_NEAR(third->get_position().x, 132.f, 0.001f);
    CHECK_EQ(layout.cursor_y(), 74.f);
    CHECK(group.get_size().y >= 78.f);
}

TEST(color_swatch_horizontal_vertical_and_picker_popup) {
    zgui_test::Harness harness;
    zgui::Color color(10, 20, 30, 255);
    int changes = 0;
    zgui::ColorSwatch swatch("swatch", color, [&changes]() { ++changes; });
    swatch.set_position({10.f, 10.f});
    swatch.set_size({200.f, 30.f});
    swatch.draw();
    CHECK(!harness.graphics->texts().empty());
    CHECK(swatch.handle_mouse_press({15.f, 15.f}));
    CHECK(!harness.manager.has_overlays());

    swatch.set_vertical_layout(true);
    swatch.set_size({100.f, 80.f});
    harness.graphics->clear_commands();
    swatch.draw();
    CHECK(swatch.handle_mouse_press({60.f, 80.f}));
    CHECK(harness.manager.has_overlays());

    harness.input->escape();
    harness.manager.update();
    CHECK(!harness.manager.has_overlays());

    zgui::ColorButton button("button", color, [&changes]() { ++changes; });
    button.set_position({10.f, 100.f});
    button.set_size({120.f, 50.f});
    CHECK(button.handle_mouse_press({20.f, 120.f}));
    CHECK(harness.manager.has_overlays());
    CHECK_EQ(changes, 0);
    harness.input->escape();
    harness.manager.update();
    CHECK(!harness.manager.has_overlays());
}
