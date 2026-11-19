// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "../support/test_harness.hh"
#include "../../zgui.hh"

#include <type_traits>

static_assert(!std::is_copy_constructible<zgui::UIElement>::value,
              "UI elements must not copy their parent/child links");
static_assert(!std::is_copy_constructible<zgui::Graphics>::value,
              "graphics backends must not copy frame state");
static_assert(!std::is_copy_constructible<zgui::WindowManager>::value,
              "window managers must not copy their owned windows and overlays");

TEST(primitive_defaults) {
    const zgui::Color color;
    const zgui::Point point;
    const zgui::Vertex vertex;
    const zgui::Rect rect;
    CHECK_EQ(color.r, 0);
    CHECK_EQ(color.g, 0);
    CHECK_EQ(color.b, 0);
    CHECK_EQ(color.a, 255);
    CHECK_EQ(point.x, 0.f);
    CHECK_EQ(point.y, 0.f);
    CHECK_EQ(vertex.position.x, 0.f);
    CHECK_EQ(vertex.coordinates.y, 0.f);
    CHECK_EQ(vertex.color.a, 255);
    CHECK_EQ(rect.position.x, 0.f);
    CHECK_EQ(rect.size.y, 0.f);
}

TEST(rect_contains_boundary_and_negative_size) {
    const zgui::Rect rect{zgui::Point(10.f, 20.f), zgui::Point(30.f, 40.f)};
    CHECK(rect.contains(zgui::Point(10.f, 20.f)));
    CHECK(rect.contains(zgui::Point(39.99f, 59.99f)));
    CHECK(!rect.contains(zgui::Point(40.f, 30.f)));
    CHECK(!rect.contains(zgui::Point(30.f, 60.f)));

    const zgui::Rect negative{zgui::Point(10.f, 20.f), zgui::Point(-5.f, -5.f)};
    CHECK(!negative.contains(zgui::Point(10.f, 20.f)));
}
