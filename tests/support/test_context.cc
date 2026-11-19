// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "test_context.hh"

zgui_test::Harness::Harness()
    : graphics(std::make_shared<MockGraphics>()), input(std::make_shared<MockInput>()) {
    zgui::core::graphics = graphics;
    zgui::core::input = input;
    zgui::core::window_manager = &manager;
}

zgui_test::Harness::~Harness() {
    zgui::core::focused_text_input = nullptr;
    zgui::core::window_manager = nullptr;
    zgui::core::graphics.reset();
    zgui::core::input.reset();
}

void zgui_test::Harness::frame() {
    input->begin_frame();
    manager.update();
    manager.draw();
    graphics->flush();
    input->end_frame();
}
