// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "../support/test_context.hh"
#include "../support/test_harness.hh"

TEST(scroll_area_clips_children_and_clamps_scroll_position) {
    zgui_test::Harness harness;
    zgui::ScrollArea area;
    area.set_position({10.f, 20.f});
    area.set_size({100.f, 80.f});
    auto child = std::make_shared<zgui::Label>("content");
    child->set_position({0.f, 150.f});
    child->set_size({80.f, 20.f});
    area.add_child(child);

    area.set_scroll_offset({40.f, 1000.f});
    CHECK_EQ(area.scroll_offset().x, 0.f);
    CHECK_EQ(area.scroll_offset().y, 92.f);
    area.set_scroll_offset({0.f, 0.f});
    area.draw();

    bool found_viewport_clip = false;
    for (const auto& command : harness.graphics->commands()) {
        if (command.type == zgui_test::GraphicsCommandType::SetClip && command.x == 11 &&
            command.y == 21 && command.width == 88 && command.height == 78) {
            found_viewport_clip = true;
        }
    }
    CHECK(found_viewport_clip);
    CHECK_EQ(harness.graphics->clip_depth(), 0u);
}

TEST(scroll_area_wheel_scroll_clamps_and_hands_off_at_edges) {
    zgui_test::Harness harness;
    zgui::ScrollArea area;
    area.set_size({100.f, 80.f});
    auto child = std::make_shared<zgui::Label>("content");
    child->set_position({0.f, 140.f});
    child->set_size({80.f, 20.f});
    area.add_child(child);
    harness.input->set_mouse({20.f, 20.f});

    harness.input->scroll(-1000);
    area.update();
    CHECK_EQ(area.scroll_offset().y, 82.f);
    CHECK(!area.handles_mouse_wheel({20.f, 20.f}));

    harness.input->end_frame();
    harness.input->scroll(1);
    CHECK(area.handles_mouse_wheel({20.f, 20.f}));
    area.update();
    CHECK_EQ(area.scroll_offset().y, 58.f);

    harness.input->end_frame();
    harness.input->scroll(1000);
    area.update();
    CHECK_EQ(area.scroll_offset().y, 0.f);
    CHECK(!area.handles_mouse_wheel({20.f, 20.f}));
}

TEST(scroll_area_vertical_scrollbar_can_be_dragged) {
    zgui_test::Harness harness;
    zgui::ScrollArea area;
    area.set_size({100.f, 80.f});
    auto child = std::make_shared<zgui::Label>("content");
    child->set_position({0.f, 180.f});
    child->set_size({80.f, 20.f});
    area.add_child(child);

    harness.input->set_mouse({94.f, 77.f});
    harness.input->press();
    CHECK(area.handle_mouse_press({94.f, 77.f}));
    area.update();
    CHECK_EQ(area.scroll_offset().y, 122.f);

    harness.input->release();
    area.update();
}

TEST(scroll_area_horizontal_scrollbar_is_optional_and_draggable) {
    zgui_test::Harness harness;
    zgui::ScrollArea area;
    area.set_size({100.f, 80.f});
    auto child = std::make_shared<zgui::Label>("content");
    child->set_position({180.f, 0.f});
    child->set_size({20.f, 20.f});
    area.add_child(child);

    CHECK_EQ(area.child_clip_rect().size.x, 98.f);
    CHECK_EQ(area.scroll_offset().x, 0.f);
    area.set_horizontal_scrolling(true);
    CHECK(area.horizontal_scrolling());
    CHECK_EQ(area.child_clip_rect().size.x, 98.f);

    harness.input->set_mouse({97.f, 74.f});
    harness.input->press();
    CHECK(area.handle_mouse_press({97.f, 74.f}));
    area.update();
    CHECK_EQ(area.scroll_offset().x, 102.f);

    harness.input->release();
    area.update();
    area.set_horizontal_scrolling(false);
    CHECK_EQ(area.scroll_offset().x, 0.f);
}

TEST(scroll_area_nested_wheel_scrolls_inner_then_outer) {
    zgui_test::Harness harness;
    zgui::ScrollArea outer;
    outer.set_size({120.f, 100.f});
    auto inner = std::make_shared<zgui::ScrollArea>();
    inner->set_position({0.f, 0.f});
    inner->set_size({80.f, 60.f});
    auto inner_content = std::make_shared<zgui::Label>("inner");
    inner_content->set_position({0.f, 100.f});
    inner_content->set_size({60.f, 20.f});
    inner->add_child(inner_content);
    outer.add_child(inner);
    auto outer_content = std::make_shared<zgui::Label>("outer");
    outer_content->set_position({0.f, 150.f});
    outer_content->set_size({80.f, 20.f});
    outer.add_child(outer_content);

    harness.input->set_mouse({20.f, 20.f});
    harness.input->scroll(-1);
    outer.update();
    CHECK_EQ(inner->scroll_offset().y, 24.f);
    CHECK_EQ(outer.scroll_offset().y, 0.f);

    inner->set_scroll_offset({0.f, 1000.f});
    harness.input->end_frame();
    harness.input->scroll(-1);
    outer.update();
    CHECK_EQ(inner->scroll_offset().y, 62.f);
    CHECK_EQ(outer.scroll_offset().y, 24.f);
}
