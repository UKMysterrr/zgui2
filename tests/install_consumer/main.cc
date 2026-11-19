// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include <zgui.hh>
#include <test_context.hh>

#ifdef ZGUI_CONSUMER_BACKEND
#include <zgui_impl_sdl2_opengl.hh>
#endif

int main() {
    zgui_test::Harness harness;
#ifdef ZGUI_CONSUMER_BACKEND
    zgui::sdl2_opengl::SDL2Input backend_input;
    if (backend_input.key_name(static_cast<int>(SDLK_LALT)) != "LMENU")
        return 1;
#endif
    zgui::UIElement element;
    element.set_position({1.f, 2.f});
    element.set_size({3.f, 4.f});
    const zgui::Point position = element.get_absolute_position();
    return position.x == 1.f && position.y == 2.f ? 0 : 1;
}
