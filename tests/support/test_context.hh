// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#ifndef ZGUI_TEST_CONTEXT_HH
#define ZGUI_TEST_CONTEXT_HH

#include "mock_graphics.hh"
#include "mock_input.hh"

#include <memory>

namespace zgui_test {

    class Harness {
    public:
        Harness();
        ~Harness();

        void frame();

        std::shared_ptr<MockGraphics> graphics;
        std::shared_ptr<MockInput> input;
        zgui::WindowManager manager;
    };

} // namespace zgui_test

#endif // ZGUI_TEST_CONTEXT_HH
