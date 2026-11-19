// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "../../backends/sdl2_opengl/zgui_impl_sdl2_opengl.hh"
#include "../support/test_harness.hh"

TEST(opengl_text_measurement_uses_ascii_fallback_without_a_context) {
    zgui::sdl2_opengl::OpenGLGraphics graphics;
    const float question_width = graphics.measure_text_width("?");
    const unsigned char unsupported_bytes[] = {0x00, 0x1f, 0x7f, 0x80, 0xff};
    for (unsigned char byte : unsupported_bytes) {
        const char text_byte = static_cast<char>(byte);
        CHECK_EQ(graphics.measure_text_width(std::string(1, text_byte)), question_width);
    }

    const std::string utf8("\xc3\xa9", 2);
    CHECK_EQ(graphics.measure_text_width(utf8), graphics.measure_text_width("??"));
}
