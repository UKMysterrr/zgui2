// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "mock_input.hh"

#include <algorithm>

void zgui_test::MockInput::begin_frame() {}

void zgui_test::MockInput::end_frame() {
    transient_keys_.clear();
    text_input_.clear();
    pressed_keys_.clear();
    wheel_delta_ = 0;
    double_clicked_ = false;
    escape_pressed_ = false;
    cursor_left_ = false;
    cursor_right_ = false;
    cursor_home_ = false;
    cursor_end_ = false;
    cursor_delete_ = false;
    mouse_delta_ = zgui::Point{};
}

void zgui_test::MockInput::set_mouse(zgui::Point position) {
    mouse_delta_ = zgui::Point(position.x - mouse_position_.x, position.y - mouse_position_.y);
    mouse_position_ = position;
}

void zgui_test::MockInput::press(MouseButton button) {
    switch (button) {
    case MouseButton::left:
        left_down_ = true;
        break;
    case MouseButton::middle:
        middle_down_ = true;
        break;
    case MouseButton::right:
        right_down_ = true;
        break;
    }
}

void zgui_test::MockInput::release(MouseButton button) {
    switch (button) {
    case MouseButton::left:
        left_down_ = false;
        break;
    case MouseButton::middle:
        middle_down_ = false;
        break;
    case MouseButton::right:
        right_down_ = false;
        break;
    }
}

void zgui_test::MockInput::scroll(int delta) {
    wheel_delta_ += delta;
}

void zgui_test::MockInput::type(const std::string& text) {
    text_input_.insert(text_input_.end(), text.begin(), text.end());
}

void zgui_test::MockInput::press_key(int key) {
    transient_keys_.insert(key);
    pressed_keys_.push_back(key);
}

void zgui_test::MockInput::set_key_down(int key, bool down) {
    if (down)
        keys_down_.insert(key);
    else
        keys_down_.erase(key);
}

void zgui_test::MockInput::double_click() {
    double_clicked_ = true;
}

void zgui_test::MockInput::escape() {
    escape_pressed_ = true;
}

void zgui_test::MockInput::set_cursor_keys(bool left, bool right, bool home, bool end,
                                           bool delete_key) {
    cursor_left_ = left;
    cursor_right_ = right;
    cursor_home_ = home;
    cursor_end_ = end;
    cursor_delete_ = delete_key;
}

bool zgui_test::MockInput::key_pressed(int key) const {
    return keys_down_.find(key) != keys_down_.end() ||
           transient_keys_.find(key) != transient_keys_.end();
}

bool zgui_test::MockInput::mouse_over(zgui::Point position, zgui::Point size) const {
    return mouse_position_.x >= position.x && mouse_position_.y >= position.y &&
           mouse_position_.x < position.x + size.x && mouse_position_.y < position.y + size.y;
}

zgui::Point zgui_test::MockInput::mouse_position() const {
    return mouse_position_;
}

bool zgui_test::MockInput::mouse_button_pressed() const {
    return left_down_;
}

bool zgui_test::MockInput::mouse_button_down(MouseButton button) const {
    switch (button) {
    case MouseButton::left:
        return left_down_;
    case MouseButton::middle:
        return middle_down_;
    case MouseButton::right:
        return right_down_;
    }
    return false;
}

zgui::Point zgui_test::MockInput::mouse_delta() const {
    return mouse_delta_;
}

bool zgui_test::MockInput::mouse_double_clicked() const {
    return double_clicked_;
}

bool zgui_test::MockInput::escape_pressed() const {
    return escape_pressed_;
}

int zgui_test::MockInput::mouse_wheel_delta() const {
    return wheel_delta_;
}

bool zgui_test::MockInput::pop_text_input(char& character) {
    if (text_input_.empty())
        return false;
    character = text_input_.front();
    text_input_.pop_front();
    return true;
}

std::optional<int> zgui_test::MockInput::poll_pressed_key() {
    if (pressed_keys_.empty())
        return std::nullopt;
    const int key = pressed_keys_.front();
    pressed_keys_.pop_front();
    return key;
}

std::string zgui_test::MockInput::key_name(int key) const {
    const auto found = key_names_.find(key);
    return found == key_names_.end() ? std::to_string(key) : found->second;
}

bool zgui_test::MockInput::text_cursor_left_pressed() const {
    return cursor_left_;
}

bool zgui_test::MockInput::text_cursor_right_pressed() const {
    return cursor_right_;
}

bool zgui_test::MockInput::text_cursor_home_pressed() const {
    return cursor_home_;
}

bool zgui_test::MockInput::text_cursor_end_pressed() const {
    return cursor_end_;
}

bool zgui_test::MockInput::text_delete_pressed() const {
    return cursor_delete_;
}
