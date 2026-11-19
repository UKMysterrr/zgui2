// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#ifndef ZGUI_MOCK_INPUT_HH
#define ZGUI_MOCK_INPUT_HH

#include "../../zgui.hh"

#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace zgui_test {

    class MockInput final : public zgui::Input {
    public:
        void begin_frame();
        void end_frame();
        void set_mouse(zgui::Point position);
        void press(MouseButton button = MouseButton::left);
        void release(MouseButton button = MouseButton::left);
        void scroll(int delta);
        void type(const std::string& text);
        void press_key(int key);
        void set_key_down(int key, bool down);
        void double_click();
        void escape();
        void set_cursor_keys(bool left, bool right, bool home, bool end, bool delete_key);

        bool key_pressed(int key) const override;
        bool mouse_over(zgui::Point position, zgui::Point size) const override;
        zgui::Point mouse_position() const override;
        bool mouse_button_pressed() const override;
        bool mouse_button_down(MouseButton button) const override;
        zgui::Point mouse_delta() const override;
        bool mouse_double_clicked() const override;
        bool escape_pressed() const override;
        int mouse_wheel_delta() const override;
        bool pop_text_input(char& character) override;
        std::optional<int> poll_pressed_key() override;
        std::string key_name(int key) const override;
        bool text_cursor_left_pressed() const override;
        bool text_cursor_right_pressed() const override;
        bool text_cursor_home_pressed() const override;
        bool text_cursor_end_pressed() const override;
        bool text_delete_pressed() const override;

    private:
        zgui::Point mouse_position_{};
        zgui::Point mouse_delta_{};
        bool left_down_ = false;
        bool middle_down_ = false;
        bool right_down_ = false;
        bool double_clicked_ = false;
        bool escape_pressed_ = false;
        bool cursor_left_ = false;
        bool cursor_right_ = false;
        bool cursor_home_ = false;
        bool cursor_end_ = false;
        bool cursor_delete_ = false;
        int wheel_delta_ = 0;
        std::unordered_set<int> keys_down_;
        std::unordered_set<int> transient_keys_;
        std::deque<char> text_input_;
        std::deque<int> pressed_keys_;
        std::unordered_map<int, std::string> key_names_;
    };

} // namespace zgui_test

#endif // ZGUI_MOCK_INPUT_HH
