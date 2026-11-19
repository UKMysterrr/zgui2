// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "../support/mock_graphics.hh"
#include "../support/test_context.hh"
#include "../support/test_harness.hh"

#include <memory>
#include <limits>
#include <cmath>
#include <vector>

namespace {
    class DrawUpdateProbe final : public zgui::UIElement {
    public:
        int draws = 0;
        int updates = 0;

        void draw() override {
            ++draws;
        }

        void update() override {
            ++updates;
        }
    };
} // namespace

TEST(draw_and_update_helpers_accept_references_and_pointers) {
    DrawUpdateProbe element;
    zgui::draw_and_update(element);
    zgui::draw_and_update(&element);
    CHECK_EQ(element.draws, 2);
    CHECK_EQ(element.updates, 2);
}

TEST(widget_index_operations_reject_invalid_values) {
    zgui_test::Harness harness;
    zgui::ComboBox combo;
    CHECK(!combo.set_selected_index(0));
    CHECK_EQ(combo.get_selected_index(), 0u);
    combo.set_items({"first", "second"});
    CHECK(combo.set_selected_index(1));
    CHECK(!combo.set_selected_index(2));
    CHECK_EQ(combo.get_selected_index(), 1u);

    zgui::TabBar tabs;
    CHECK(!tabs.set_active_tab(0));
    tabs.add_tab("only");
    CHECK(!tabs.set_active_tab(1));
    CHECK_EQ(tabs.get_active_tab(), 0u);
    CHECK_EQ(tabs.tab_title(1), std::string{});
    CHECK_EQ(tabs.tab_widget_count(1), 0u);
    CHECK_EQ(tabs.tab_visible_widget_count(1), 0u);

    zgui::TableView table;
    CHECK(!table.select_row(0));
    CHECK(!table.set_cell(0, 0, {"invalid", std::nullopt}));
    table.set_columns({{"first", 40.f}});
    table.add_row(std::vector<std::string>{"value"});
    CHECK(table.select_row(0));
    CHECK(!table.select_row(1));
    CHECK(!table.set_cell(0, 1, {"invalid", std::nullopt}));
    table.clear_rows();
    CHECK(!table.selected_row().has_value());
}

TEST(fit_text_handles_small_widths_and_caps_truncation_tracking) {
    zgui_test::MockGraphics graphics;
    graphics.set_character_width(5.f);
    CHECK_EQ(graphics.fit_text("abc", 15.f), "abc");
    CHECK_EQ(graphics.fit_text("abc", 10.f), "..");
    CHECK_EQ(graphics.fit_text("abc", 9.f), "");
    CHECK_EQ(graphics.fit_text("abc", 0.f), "");

    zgui::core::text_truncations.clear();
    zgui::core::collect_text_truncations = true;
    CHECK_EQ(graphics.fit_text("fit", 15.f), "fit");
    CHECK(zgui::core::text_truncations.empty());
    for (int index = 0; index < 1100; ++index)
        graphics.fit_text("too long", 5.f);
    zgui::core::collect_text_truncations = false;
    CHECK_EQ(zgui::core::text_truncations.size(), 1024u);
}

TEST(slider_ranges_clamp_and_handle_degenerate_and_extreme_values) {
    zgui_test::Harness harness;
    harness.input->press();
    int integer_value = 200;
    zgui::Slider<int> integer_slider("integer", &integer_value, 100, 0);
    CHECK_EQ(integer_value, 100);
    integer_slider.set_size({100.f, 30.f});
    int changes = 0;
    integer_slider.set_on_change([&changes]() { ++changes; });
    harness.input->set_mouse({99.9f, 10.f});
    CHECK(integer_slider.handle_mouse_press({50.f, 10.f}));
    CHECK_EQ(integer_value, 100);
    CHECK_EQ(changes, 0);
    harness.input->set_mouse({0.f, 10.f});
    integer_slider.update();
    CHECK_EQ(integer_value, 0);
    CHECK_EQ(changes, 1);
    harness.input->set_mouse({150.f, 10.f});
    integer_slider.update();
    CHECK_EQ(integer_value, 100);
    CHECK_EQ(changes, 2);

    int fixed_value = 9;
    zgui::Slider<int> fixed_slider("fixed", &fixed_value, 3, 3);
    CHECK_EQ(fixed_value, 3);
    fixed_slider.set_size({100.f, 30.f});
    harness.input->set_mouse({50.f, 10.f});
    CHECK(fixed_slider.handle_mouse_press({50.f, 10.f}));
    CHECK_EQ(fixed_value, 3);

    float float_value = 0.f;
    zgui::Slider<float> float_slider("float", &float_value, -1.f, 1.f);
    float_slider.set_size({100.f, 30.f});
    harness.input->set_mouse({50.f, 10.f});
    CHECK(float_slider.handle_mouse_press({50.f, 10.f}));
    CHECK_NEAR(float_value, 0.f, 0.0001f);

    float extreme_value = 0.f;
    const float extreme = std::numeric_limits<float>::max() / 2.f;
    zgui::Slider<float> extreme_slider("extreme", &extreme_value, -extreme, extreme);
    extreme_slider.set_size({100.f, 30.f});
    harness.input->set_mouse({99.f, 10.f});
    CHECK(extreme_slider.handle_mouse_press({50.f, 10.f}));
    CHECK(std::isfinite(extreme_value));

    unsigned char byte_value = 0;
    zgui::Slider<unsigned char> byte_slider("byte", &byte_value, 0, 255);
    byte_slider.set_size({100.f, 30.f});
    harness.input->set_mouse({150.f, 10.f});
    CHECK(byte_slider.handle_mouse_press({50.f, 10.f}));
    CHECK_EQ(byte_value, static_cast<unsigned char>(255));

    double double_value = 0.0;
    zgui::Slider<double> double_slider("double", &double_value, -1.0e9, 1.0e9);
    double_slider.set_size({100.f, 30.f});
    harness.input->set_mouse({75.f, 10.f});
    CHECK(double_slider.handle_mouse_press({50.f, 10.f}));
    CHECK(std::isfinite(double_value));
}

TEST(color_picker_hsv_round_trips_common_colors) {
    zgui_test::Harness harness;
    struct Sample {
        zgui::Color color;
        zgui::Point saturation_value_position;
    };
    const std::vector<Sample> samples{
        {{255, 0, 0, 0}, {169.5f, 28.f}},
        {{0, 255, 0, 255}, {169.5f, 28.f}},
        {{0, 0, 255, 128}, {169.5f, 28.f}},
        {{128, 128, 128, 0}, {10.f, 28.f + (1.f - 128.f / 255.f) * 128.f}},
        {{255, 255, 255, 255}, {10.f, 28.f}},
        {{0, 0, 0, 255}, {10.f, 155.99f}},
    };
    for (const Sample& sample : samples) {
        zgui::Color color = sample.color;
        zgui::ColorPicker picker(&color);
        CHECK(picker.handle_mouse_press(sample.saturation_value_position));
        CHECK(std::abs(static_cast<int>(color.r) - sample.color.r) <= 1);
        CHECK(std::abs(static_cast<int>(color.g) - sample.color.g) <= 1);
        CHECK(std::abs(static_cast<int>(color.b) - sample.color.b) <= 1);
        CHECK_EQ(color.a, sample.color.a);
    }

    const auto color_from_hsv = [](float hue, float saturation, float value, unsigned char alpha) {
        const float chroma = value * saturation;
        const float sector = hue / 60.f;
        const float intermediate = chroma * (1.f - std::abs(std::fmod(sector, 2.f) - 1.f));
        float red = 0.f;
        float green = 0.f;
        float blue = 0.f;
        if (sector < 1.f) {
            red = chroma;
            green = intermediate;
        } else if (sector < 2.f) {
            red = intermediate;
            green = chroma;
        } else if (sector < 3.f) {
            green = chroma;
            blue = intermediate;
        } else if (sector < 4.f) {
            green = intermediate;
            blue = chroma;
        } else if (sector < 5.f) {
            red = intermediate;
            blue = chroma;
        } else {
            red = chroma;
            blue = intermediate;
        }
        const float offset = value - chroma;
        return zgui::Color(static_cast<unsigned char>(std::lround((red + offset) * 255.f)),
                           static_cast<unsigned char>(std::lround((green + offset) * 255.f)),
                           static_cast<unsigned char>(std::lround((blue + offset) * 255.f)), alpha);
    };
    for (float hue = 0.f; hue < 360.f; hue += 60.f) {
        for (float saturation : {0.f, 0.25f, 0.5f, 0.75f}) {
            for (float value : {0.25f, 0.5f, 0.75f, 1.f}) {
                for (unsigned char alpha :
                     {static_cast<unsigned char>(0), static_cast<unsigned char>(255)}) {
                    const zgui::Color expected = color_from_hsv(hue, saturation, value, alpha);
                    zgui::Color actual = expected;
                    zgui::ColorPicker picker(&actual);
                    const zgui::Point position(10.f + saturation * 160.f,
                                               28.f + (1.f - value) * 128.f);
                    CHECK(picker.handle_mouse_press(position));
                    CHECK(std::abs(static_cast<int>(actual.r) - expected.r) <= 1);
                    CHECK(std::abs(static_cast<int>(actual.g) - expected.g) <= 1);
                    CHECK(std::abs(static_cast<int>(actual.b) - expected.b) <= 1);
                    CHECK_EQ(actual.a, alpha);
                }
            }
        }
    }
}

#ifdef NDEBUG
TEST(null_pointer_bindings_use_internal_fallbacks_without_exceptions) {
    zgui_test::Harness harness;
    zgui::CheckBox check_box("check", nullptr);
    check_box.set_value(nullptr);

    zgui::Slider<int> slider("slider", nullptr, 0, 10);
    CHECK_EQ(slider.get_value(), 0);

    zgui::ComboBox combo;
    combo.bind_selected_index(nullptr);

    zgui::MultiComboBox multi_combo({"one"}, nullptr);
    CHECK_EQ(multi_combo.values().size(), 1u);

    zgui::TextInput text_input(nullptr);
    text_input.set_value(nullptr);
    zgui::KeyBind key_bind("key", nullptr);
    CHECK_EQ(key_bind.get_key(), 0);

    zgui::ColorPicker picker(nullptr);
    zgui::ColorSwatch swatch("swatch", nullptr);
    zgui::ColorButton button("button", nullptr);
    zgui::Window window("window");
    window.set_status_checkbox("disabled", nullptr);

    zgui::TableView table;
    table.set_row_height(0.f);
    table.set_row_height(std::numeric_limits<float>::quiet_NaN());
    table.set_columns({{"nan", std::numeric_limits<float>::quiet_NaN(), true,
                        std::numeric_limits<float>::infinity()}});
    table.set_size({100.f, 60.f});
    table.draw();
    CHECK_EQ(harness.graphics->clip_depth(), 0u);
}
#endif
