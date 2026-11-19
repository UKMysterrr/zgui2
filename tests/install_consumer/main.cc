// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include <zgui.hh>

int main() {
    zgui::UIElement element;
    element.set_position({1.f, 2.f});
    element.set_size({3.f, 4.f});
    const zgui::Point position = element.get_absolute_position();
    return position.x == 1.f && position.y == 2.f ? 0 : 1;
}
