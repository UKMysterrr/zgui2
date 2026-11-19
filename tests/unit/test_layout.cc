// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "../support/test_context.hh"
#include "../support/test_harness.hh"

TEST(stack_layout_places_rows_and_advances_cursor) {
    zgui_test::Harness harness;
    zgui::UIElement parent;
    parent.set_size({200.f, 100.f});
    zgui::Label first("first");
    zgui::Label second("second");
    zgui::StackLayout layout(parent, 5.f, 10.f, 3.f);

    layout.add_row(first, 20.f);
    layout.add_row(second, 14.f);
    CHECK_EQ(first.get_position().x, 10.f);
    CHECK_EQ(first.get_position().y, 5.f);
    CHECK_EQ(first.get_size().x, 180.f);
    CHECK_EQ(first.get_size().y, 20.f);
    CHECK_EQ(second.get_position().y, 28.f);
    CHECK_EQ(layout.cursor_y(), 45.f);

    parent.set_size({300.f, 100.f});
    CHECK_EQ(first.get_size().x, 280.f);
    CHECK_EQ(second.get_size().x, 280.f);
}

TEST(stack_layout_two_column_rows_respect_fraction_and_gap) {
    zgui_test::Harness harness;
    zgui::UIElement parent;
    parent.set_size({200.f, 100.f});
    bool checked = false;
    auto check_box = std::make_shared<zgui::CheckBox>("check", checked);
    auto combo_box = std::make_shared<zgui::ComboBox>(std::vector<std::string>{"one", "two"});
    zgui::StackLayout layout(parent, 4.f, 10.f, 5.f);

    layout.add_two_column_row(check_box, combo_box, 0.4f, 20.f);
    CHECK_EQ(check_box->get_position().x, 10.f);
    CHECK_EQ(check_box->get_size().x, 70.f);
    CHECK_EQ(combo_box->get_position().x, 85.f);
    CHECK_EQ(combo_box->get_size().x, 105.f);
    CHECK_EQ(combo_box->get_position().y, 4.f);
    CHECK_EQ(layout.cursor_y(), 29.f);
}

TEST(page_layout_uses_shared_page_spacing_defaults) {
    zgui_test::Harness harness;
    zgui::UIElement page;
    page.set_size({100.f, 100.f});
    zgui::Label row("row");
    zgui::StackLayout layout = zgui::StackLayout::page(page);
    layout.add_row(row);

    CHECK_EQ(row.get_position().y, zgui::spacing::kPageStackStartY);
    CHECK_EQ(layout.cursor_y(), zgui::spacing::kPageStackStartY + zgui::spacing::kDefaultRowHeight +
                                    zgui::spacing::kPageRowGap);
}

TEST(parent_constraints_recompute_without_resize_drift) {
    zgui_test::Harness harness;
    zgui::UIElement parent;
    parent.set_size({100.f, 80.f});
    zgui::UIElement child;
    child.set_position({10.f, 10.f});
    child.set_size({20.f, 20.f});
    parent.add_child(&child);
    child.set_horizontal_constraints(0.25f, 0.75f, 5.f, -5.f);
    child.set_vertical_constraints(0.2f, 0.8f, 4.f, -6.f);

    for (int iteration = 0; iteration < 1000; ++iteration) {
        const float width = 100.f + static_cast<float>(iteration % 31);
        const float height = 80.f + static_cast<float>((iteration * 7) % 29);
        parent.set_size({width, height});
        CHECK_NEAR(child.get_position().x, width * 0.25f + 5.f, 0.0001f);
        CHECK_NEAR(child.get_size().x, width * 0.5f - 10.f, 0.0001f);
        CHECK_NEAR(child.get_position().y, height * 0.2f + 4.f, 0.0001f);
        CHECK_NEAR(child.get_size().y, height * 0.6f - 10.f, 0.0001f);
    }
}

TEST(parent_anchors_stretch_and_keep_opposite_edges_fixed) {
    zgui_test::Harness harness;
    zgui::UIElement parent;
    parent.set_size({100.f, 100.f});
    zgui::UIElement child;
    child.set_position({10.f, 20.f});
    child.set_size({30.f, 10.f});
    parent.add_child(&child);
    child.set_anchors(true, true, false, true);

    parent.set_size({120.f, 130.f});
    CHECK_EQ(child.get_position().x, 10.f);
    CHECK_EQ(child.get_size().x, 50.f);
    CHECK_EQ(child.get_position().y, 50.f);
    CHECK_EQ(child.get_size().y, 10.f);
}
