// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "../support/test_context.hh"
#include "../support/test_harness.hh"

TEST(table_selection_callback_and_clear_rows) {
    zgui_test::Harness harness;
    zgui::TableView table;
    table.set_position({10.f, 10.f});
    table.set_size({200.f, 100.f});
    table.set_columns({{"first", 80.f}, {"second", 80.f}});
    table.add_row(std::vector<std::string>{"one", "value"});
    table.add_row(std::vector<std::string>{"two", "value"});
    table.add_row(std::vector<std::string>{"three", "value"});

    int callback_count = 0;
    std::size_t selected = 99;
    table.set_on_selection_changed([&](std::size_t row) {
        ++callback_count;
        selected = row;
    });
    CHECK(table.handle_mouse_press({20.f, 33.f}));
    CHECK_EQ(table.selected_row(), std::optional<std::size_t>(0));
    CHECK_EQ(selected, 0u);
    CHECK_EQ(callback_count, 1);
    CHECK(table.select_row(0));
    CHECK_EQ(callback_count, 1);

    table.clear_rows();
    CHECK_EQ(table.row_count(), 0u);
    CHECK(!table.selected_row().has_value());
}

TEST(table_wheel_scroll_clamps_and_selects_scrolled_rows) {
    zgui_test::Harness harness;
    zgui::TableView table;
    table.set_position({10.f, 10.f});
    table.set_size({200.f, 100.f});
    table.set_columns({{"first", 80.f}});
    for (int row = 0; row < 10; ++row)
        table.add_row(std::vector<std::string>{std::to_string(row)});

    harness.input->set_mouse({20.f, 40.f});
    harness.input->scroll(-1000);
    CHECK(table.handles_mouse_wheel({20.f, 40.f}));
    table.update();
    CHECK_EQ(table.visible_widget_count(), 9u);
    CHECK(!table.handles_mouse_wheel({20.f, 40.f}));
    CHECK(table.handle_mouse_press({20.f, 33.f}));
    CHECK_EQ(table.selected_row(), std::optional<std::size_t>(6));

    harness.input->end_frame();
    harness.input->scroll(1000);
    CHECK(table.handles_mouse_wheel({20.f, 40.f}));
    table.update();
    CHECK(!table.handles_mouse_wheel({20.f, 40.f}));
    CHECK(table.handle_mouse_press({20.f, 33.f}));
    CHECK_EQ(table.selected_row(), std::optional<std::size_t>(0));

    harness.input->end_frame();
    harness.input->scroll(-1000);
    table.update();
    table.clear_rows();
    table.add_row(std::vector<std::string>{"fresh"});
    CHECK(table.handle_mouse_press({20.f, 33.f}));
    CHECK_EQ(table.selected_row(), std::optional<std::size_t>(0));
}

TEST(table_weight_columns_resize_and_visibility) {
    zgui_test::Harness harness;
    zgui::TableView table;
    table.set_position({10.f, 10.f});
    table.set_size({200.f, 100.f});
    table.set_columns({{"A", 40.f, true, 1.f}, {"B", 40.f, true, 1.f}});
    table.add_row(std::vector<std::string>{"left", "right"});

    table.draw();
    float second_column_x = -1.f;
    for (const auto& command : harness.graphics->commands()) {
        if (command.type == zgui_test::GraphicsCommandType::Text && command.text == "B")
            second_column_x = command.position.x;
    }
    CHECK_EQ(second_column_x, 109.f);

    CHECK(table.handle_mouse_press({107.f, 15.f}));
    harness.input->set_mouse({127.f, 15.f});
    harness.input->press();
    table.update();
    harness.input->release();
    table.update();
    harness.graphics->clear_commands();
    table.draw();
    second_column_x = -1.f;
    for (const auto& command : harness.graphics->commands()) {
        if (command.type == zgui_test::GraphicsCommandType::Text && command.text == "B")
            second_column_x = command.position.x;
    }
    CHECK_EQ(second_column_x, 129.f);

    table.set_column_visible(0, false);
    harness.graphics->clear_commands();
    table.draw();
    second_column_x = -1.f;
    for (const auto& command : harness.graphics->commands()) {
        if (command.type == zgui_test::GraphicsCommandType::Text && command.text == "B")
            second_column_x = command.position.x;
    }
    CHECK_EQ(second_column_x, 16.f);
}

TEST(table_draw_work_is_bounded_by_visible_rows) {
    zgui_test::Harness harness;
    zgui::TableView table;
    table.set_size({200.f, 100.f});
    table.set_columns({{"first", 80.f}, {"second", 80.f}});
    for (int row = 0; row < 10000; ++row)
        table.add_row(std::vector<std::string>{"first", "second"});

    table.draw();
    std::size_t text_commands = 0;
    for (const auto& command : harness.graphics->commands())
        if (command.type == zgui_test::GraphicsCommandType::Text)
            ++text_commands;
    CHECK(text_commands <= 10u);
}

TEST(table_row_style_cells_and_scrollbar_interaction) {
    zgui_test::Harness harness;
    zgui::TableView table;
    table.set_position({10.f, 10.f});
    table.set_size({200.f, 100.f});
    table.set_columns({{"first", 80.f}, {"second", 80.f}});
    table.set_zebra_stripes(false);
    table.add_row(zgui::TableView::Row{{"background", std::nullopt}, {"value", std::nullopt}},
                  zgui::Color(10, 20, 30));
    table.add_row(std::vector<std::string>{"ordinary", "cell"});
    const zgui::Color custom(40, 50, 60);
    CHECK(table.set_cell(1, 1, {"custom", custom}));
    CHECK(!table.set_cell(2, 0, {"invalid", std::nullopt}));
    CHECK(!table.set_cell(1, 2, {"invalid", std::nullopt}));

    table.draw();
    bool found_custom_cell = false;
    for (const auto& command : harness.graphics->commands()) {
        if (command.type == zgui_test::GraphicsCommandType::Text && command.text == "custom") {
            found_custom_cell = true;
            CHECK_EQ(command.color.r, custom.r);
            CHECK_EQ(command.color.g, custom.g);
        }
    }
    CHECK(found_custom_cell);
    for (int row = 0; row < 20; ++row)
        table.add_row(std::vector<std::string>{"more", "rows"});
    CHECK(table.handles_mouse_wheel({20.f, 40.f}));
    CHECK(table.handle_mouse_press({198.f, 60.f}));
    harness.input->set_mouse({198.f, 90.f});
    harness.input->press();
    table.update();
    harness.input->release();
    table.update();
    table.set_row_height(12.f);
    CHECK(table.visible_widget_count() < table.widget_count());
}

TEST(table_draw_handles_zero_rows_columns_and_clamped_widths) {
    zgui_test::Harness harness;
    zgui::TableView table;
    table.set_size({160.f, 80.f});

    table.draw();
    CHECK_EQ(harness.graphics->clip_depth(), 0u);

    table.set_columns({{"zero", 0.f}, {"negative", -10.f}});
    table.draw();
    CHECK_EQ(harness.graphics->clip_depth(), 0u);

    table.add_row(std::vector<std::string>{"value"});
    table.set_columns({});
    CHECK(!table.set_cell(0, 0, {"invalid", std::nullopt}));
    table.draw();
    CHECK_EQ(harness.graphics->clip_depth(), 0u);
}
