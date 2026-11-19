// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#ifndef ZGUI_CLASSIC_EXAMPLE_SLIDER_HH
#define ZGUI_CLASSIC_EXAMPLE_SLIDER_HH

#include "zgui.hh"

namespace zgui_classic {
    class StepperButton final : public zgui::Button {
    public:
        using zgui::Button::Button;

        void draw() override {
            const auto position = get_absolute_position();
            const auto size = get_size();
            const auto& theme = zgui::core::active_theme;
            auto& graphics = *zgui::core::graphics;
            graphics.fill_rect(position, size, theme.border_outer);
            graphics.fill_rect({position.x + 1.f, position.y + 1.f}, {size.x - 2.f, size.y - 2.f},
                               contains_point(zgui::core::input->mouse_position())
                                   ? theme.control_hover
                                   : theme.control_bg);
            graphics.draw_text(
                get_title(),
                {position.x + (size.x - graphics.measure_text_width(get_title())) / 2.f,
                 position.y + (size.y - 12.f) / 2.f},
                theme.text_primary);
        }
    };

    template <typename T>
    std::shared_ptr<zgui::Slider<T>> add_slider(zgui::GroupBox& group, const std::string& title,
                                                T initial, T minimum, T maximum, T step, float y) {
        constexpr float slider_x = 30.f;
        constexpr float slider_width = 130.f;
        constexpr float button_size = 14.f;
        constexpr float gap = 2.f;
        const float button_y = y + zgui::spacing::kSliderLabelHeight +
                               zgui::spacing::kSliderTrackHeight / 2.f - button_size / 2.f;

        auto slider =
            std::make_shared<zgui::Slider<T>>(title, zgui::bound(initial), minimum, maximum);
        slider->set_position({slider_x, y});
        slider->set_size({slider_width, 30.f});
        group.add_child(slider);

        auto minus = std::make_shared<StepperButton>(
            "-", [slider, step] { slider->set_value(slider->get_value() - step); });
        minus->set_position({slider_x - gap - button_size, button_y});
        minus->set_size({button_size, button_size});
        group.add_child(minus);
        auto plus = std::make_shared<StepperButton>(
            "+", [slider, step] { slider->set_value(slider->get_value() + step); });
        plus->set_position({slider_x + slider_width + gap, button_y});
        plus->set_size({button_size, button_size});
        group.add_child(plus);
        return slider;
    }
} // namespace zgui_classic

#endif
