// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "../../examples/showcase/showcase_ui.hh"
#include "../support/test_context.hh"
#include "../support/test_harness.hh"

#include <iostream>

TEST(public_showcase_builds_and_runs_headlessly) {
    zgui_test::Harness harness;
    zgui_showcase::AppState state;
    const auto window = zgui_showcase::build_showcase_ui(harness.manager, state);
    CHECK_EQ(state.table->row_count(), 10000u);

    std::string error;
    for (int frame = 0; frame < 120; ++frame) {
        harness.frame();
        CHECK_EQ(harness.graphics->clip_depth(), 0u);
        if (!harness.manager.validate_layout(*window, &error)) {
            std::cerr << "showcase layout error: " << error << '\n';
            CHECK(false);
            break;
        }
    }
}
