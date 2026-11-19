// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "../support/mock_graphics.hh"
#include "../support/test_harness.hh"

TEST(fill_rect_emits_six_ordered_triangle_vertices) {
    zgui_test::MockGraphics graphics;
    const zgui::Color color(1, 2, 3, 4);
    graphics.fill_rect(zgui::Point(10.f, 20.f), zgui::Point(30.f, 40.f), color);
    CHECK_EQ(graphics.commands().size(), 1u);
    const auto& command = graphics.commands().front();
    CHECK_EQ(command.topology, zgui::VertexTopology::TRIANGLE_LIST);
    CHECK_EQ(command.vertices.size(), 6u);
    CHECK_EQ(command.vertices[0].position.x, 10.f);
    CHECK_EQ(command.vertices[0].position.y, 20.f);
    CHECK_EQ(command.vertices[1].position.x, 40.f);
    CHECK_EQ(command.vertices[2].position.y, 60.f);
    CHECK_EQ(command.vertices[5].position.x, 40.f);
    CHECK_EQ(command.vertices[5].position.y, 60.f);
    CHECK_EQ(command.vertices[0].color.a, 4);
}

TEST(line_and_rect_use_line_list_geometry) {
    zgui_test::MockGraphics graphics;
    graphics.draw_line(zgui::Point(1.f, 2.f), zgui::Point(3.f, 4.f), zgui::Color(5, 6, 7));
    CHECK_EQ(graphics.commands().size(), 1u);
    CHECK_EQ(graphics.commands()[0].topology, zgui::VertexTopology::LINE_LIST);
    CHECK_EQ(graphics.commands()[0].vertices.size(), 2u);

    graphics.draw_rect(zgui::Point(0.f, 0.f), zgui::Point(10.f, 20.f), zgui::Color{});
    CHECK_EQ(graphics.commands().size(), 5u);
    for (std::size_t index = 1; index < graphics.commands().size(); ++index) {
        CHECK_EQ(graphics.commands()[index].topology, zgui::VertexTopology::LINE_LIST);
        CHECK_EQ(graphics.commands()[index].vertices.size(), 2u);
    }
}

TEST(mixed_topologies_and_frame_layers_preserve_submission_order) {
    zgui_test::MockGraphics graphics;
    const zgui::Color fill(20, 30, 40);
    graphics.fill_rect({1.f, 2.f}, {10.f, 12.f}, fill);
    graphics.draw_line({0.f, 0.f}, {5.f, 5.f}, fill);
    graphics.fill_rect({3.f, 4.f}, {6.f, 8.f}, fill);
    CHECK_EQ(graphics.commands().size(), 3u);
    CHECK_EQ(graphics.commands()[0].topology, zgui::VertexTopology::TRIANGLE_LIST);
    CHECK_EQ(graphics.commands()[1].topology, zgui::VertexTopology::LINE_LIST);
    CHECK_EQ(graphics.commands()[2].topology, zgui::VertexTopology::TRIANGLE_LIST);
    CHECK_EQ(graphics.commands()[0].vertices[0].position.x, 1.f);
    CHECK_EQ(graphics.commands()[1].vertices[0].position.x, 0.f);
    CHECK_EQ(graphics.commands()[2].vertices[0].position.x, 3.f);

    graphics.clear_commands();
    graphics.draw_frame({10.f, 20.f}, {12.f, 14.f}, fill);
    CHECK_EQ(graphics.commands().size(), 3u);
    CHECK_EQ(graphics.commands()[0].vertices[0].color.r, zgui::core::active_theme.border_outer.r);
    CHECK_EQ(graphics.commands()[1].vertices[0].color.r, zgui::core::active_theme.border_inner.r);
    CHECK_EQ(graphics.commands()[2].vertices[0].color.r, fill.r);
}

TEST(graphics_helpers_dispatch_identically_through_base_and_mock) {
    zgui_test::MockGraphics concrete;
    zgui::Graphics& base = concrete;
    concrete.draw_line(zgui::Point(1.f, 2.f), zgui::Point(3.f, 4.f), zgui::Color(9, 8, 7));
    const auto expected = concrete.commands();
    concrete.clear_commands();
    base.draw_line(zgui::Point(1.f, 2.f), zgui::Point(3.f, 4.f), zgui::Color(9, 8, 7));
    CHECK_EQ(concrete.commands().size(), expected.size());
    CHECK_EQ(concrete.commands()[0].topology, expected[0].topology);
    CHECK_EQ(concrete.commands()[0].vertices[0].position.x, expected[0].vertices[0].position.x);
    CHECK_EQ(concrete.commands()[0].vertices[1].position.y, expected[0].vertices[1].position.y);
}

TEST(clip_stack_intersects_and_handles_empty_regions) {
    zgui_test::MockGraphics graphics;
    graphics.push_clip_rect(10, 20, 100, 80);
    CHECK_EQ(graphics.clip_depth(), 1u);
    graphics.push_clip_rect(50, 0, 100, 100);
    CHECK_EQ(graphics.clip_depth(), 2u);
    auto clip = graphics.last_clip();
    CHECK(clip.has_value());
    CHECK_EQ(clip->x, 50);
    CHECK_EQ(clip->y, 20);
    CHECK_EQ(clip->width, 60);
    CHECK_EQ(clip->height, 80);

    graphics.push_clip_rect(200, 200, 5, 5);
    clip = graphics.last_clip();
    CHECK(clip.has_value());
    CHECK_EQ(clip->width, 0);
    CHECK_EQ(clip->height, 0);
    graphics.pop_clip_rect();
    CHECK_EQ(graphics.clip_depth(), 2u);
    graphics.pop_clip_rect();
    CHECK_EQ(graphics.clip_depth(), 1u);
    clip = graphics.last_clip();
    CHECK(clip.has_value());
    CHECK_EQ(clip->x, 10);
    CHECK_EQ(clip->width, 100);
    graphics.pop_clip_rect();
    CHECK_EQ(graphics.clip_depth(), 0u);
    CHECK(!graphics.last_clip().has_value());
    graphics.pop_clip_rect();
    CHECK_EQ(graphics.clip_depth(), 0u);

    graphics.push_clip_rect(0, 0, -4, 10);
    CHECK_EQ(graphics.clip_depth(), 1u);
    clip = graphics.last_clip();
    CHECK(clip.has_value());
    CHECK_EQ(clip->width, 0);

    graphics.pop_clip_rect();
    graphics.push_clip_rect(4, 5, 0, 0);
    clip = graphics.last_clip();
    CHECK(clip.has_value());
    CHECK_EQ(clip->width, 0);
    CHECK_EQ(clip->height, 0);
}

TEST(gradient_rect_preserves_corner_colors) {
    zgui_test::MockGraphics graphics;
    const zgui::Color top_left(1, 2, 3);
    const zgui::Color top_right(4, 5, 6);
    const zgui::Color bottom_right(7, 8, 9);
    const zgui::Color bottom_left(10, 11, 12);
    graphics.draw_gradient_rect(zgui::Point{}, zgui::Point(4.f, 6.f), top_left, top_right,
                                bottom_right, bottom_left);
    const auto& vertices = graphics.commands().front().vertices;
    CHECK_EQ(vertices[0].color.r, top_left.r);
    CHECK_EQ(vertices[1].color.r, top_right.r);
    CHECK_EQ(vertices[2].color.r, bottom_right.r);
    CHECK_EQ(vertices[5].color.r, bottom_left.r);
}
