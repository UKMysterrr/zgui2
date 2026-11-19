// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "zgui_impl_sdl2_opengl.hh"
#include "slider.hh"

#include <charconv>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

namespace {
    class TextButton final : public zgui::Button {
    public:
        using zgui::Button::Button;

        void draw() override {
            zgui::core::graphics->draw_text(get_title(), get_absolute_position(),
                                            zgui::core::active_theme.text_primary);
        }
    };

    template <typename T>
    void place(zgui::UIElement& parent, const std::shared_ptr<T>& child, zgui::Point position,
               zgui::Point size) {
        child->set_position(position);
        child->set_size(size);
        parent.add_child(child);
    }

    void build_ui(zgui::WindowManager& manager) {
        auto window = std::make_shared<zgui::Window>("zgui example window");
        window->set_position({8.f, 8.f});
        window->set_size({438.f, 440.f});
        window->set_resizable(false);

        auto first = std::make_shared<zgui::GroupBox>("Centered title");
        first->set_fit_content(false);
        first->set_title_alignment(zgui::TextAlignment::center);
        place(*window, first, {12.f, 26.f}, {194.f, 150.f});
        place(*first, std::make_shared<zgui::CheckBox>("checkbox", zgui::bound(false)), {8.f, 20.f},
              {170.f, 16.f});
        place(*first, std::make_shared<zgui::CheckBox>("checkbox", zgui::bound(true)), {8.f, 38.f},
              {170.f, 16.f});
        zgui_classic::add_slider(*first, "slider with integer", 25, 0, 100, 1, 58.f);
        place(*first,
              std::make_shared<zgui::Button>("button", [] { std::cout << "button clicked\n"; }),
              {8.f, 113.f}, {78.f, 22.f});
        place(*first, std::make_shared<zgui::ToggleButton>("toggle", zgui::bound(true)),
              {100.f, 113.f}, {78.f, 22.f});

        auto second = std::make_shared<zgui::GroupBox>("Default title");
        second->set_fit_content(false);
        place(*window, second, {12.f, 182.f}, {194.f, 244.f});
        place(*second, std::make_shared<zgui::CheckBox>("checkbox", zgui::bound(true)), {8.f, 20.f},
              {170.f, 16.f});
        place(*second, std::make_shared<zgui::Label>("This is sample text :)"), {8.f, 42.f},
              {170.f, 16.f});
        zgui_classic::add_slider(*second, "slider with float", 13.37f, 0.f, 25.f, 0.1f, 65.f);
        place(*second, std::make_shared<zgui::Label>("combobox"), {30.f, 108.f}, {150.f, 16.f});
        place(*second, std::make_shared<zgui::ComboBox>(std::vector<std::string>{"Give", "Take"}),
              {30.f, 126.f}, {130.f, 20.f});
        place(*second, std::make_shared<zgui::Label>("multi combobox"), {30.f, 158.f},
              {150.f, 16.f});
        place(*second,
              std::make_shared<zgui::MultiComboBox>(
                  std::vector<std::string>{"First", "Second", "Third"},
                  zgui::bound(std::vector<bool>{false, false, false})),
              {30.f, 176.f}, {130.f, 20.f});

        auto right = std::make_shared<zgui::GroupBox>();
        right->set_fit_content(false);
        place(*window, right, {214.f, 26.f}, {212.f, 400.f});
        place(*right, std::make_shared<zgui::Label>("key bind"), {12.f, 20.f}, {184.f, 16.f});
        place(*right, std::make_shared<zgui::KeyBind>("", zgui::bound(static_cast<int>(SDLK_LALT))),
              {12.f, 37.f}, {184.f, 22.f});
        auto login = std::make_shared<zgui::GroupBox>("Login form");
        login->set_fit_content(false);
        login->set_title_alignment(zgui::TextAlignment::center);
        place(*right, login, {8.f, 68.f}, {194.f, 130.f});
        place(*login, std::make_shared<zgui::Label>("Username"), {12.f, 20.f}, {160.f, 16.f});
        place(*login, std::make_shared<zgui::TextInput>(zgui::bound(std::string{})), {12.f, 38.f},
              {160.f, 22.f});
        place(*login, std::make_shared<zgui::Label>("Password"), {12.f, 68.f}, {160.f, 16.f});
        auto password = std::make_shared<zgui::TextInput>(zgui::bound(std::string{}));
        password->set_password_mode(true);
        place(*login, password, {12.f, 86.f}, {160.f, 22.f});
        place(*right, std::make_shared<zgui::Label>("listbox"), {12.f, 208.f}, {170.f, 16.f});
        auto list = std::make_shared<zgui::TableView>();
        list->set_columns({{"", 160.f}});
        list->set_zebra_stripes(false);
        for (const char* text : {"zgui", "framework", "by zxvnme", "& contributors"})
            list->add_row(std::vector<std::string>{text});
        place(*right, list, {12.f, 226.f}, {160.f, 100.f});
        auto clickable = std::make_shared<TextButton>("clickable text");
        const std::weak_ptr<zgui::Button> weak_clickable = clickable;
        clickable->set_on_click([weak_clickable] {
            if (const auto button = weak_clickable.lock())
                button->set_title(button->get_title() == "clickable text" ? "clicked"
                                                                          : "clickable text");
        });
        place(*right, clickable, {12.f, 338.f}, {140.f, 22.f});
        manager.add_window(window);
    }
} // namespace

int main(int argc, char** argv) {
    std::size_t frames = 0;
    if (argc != 1) {
        if (argc != 3 || std::string(argv[1]) != "--frames") {
            std::cerr << "usage: zgui_classic_example [--frames N]\n";
            return 2;
        }
        const std::string value(argv[2]);
        std::uint64_t parsed = 0;
        const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
        if (result.ec != std::errc{} || result.ptr != value.data() + value.size() ||
            parsed > std::numeric_limits<std::size_t>::max()) {
            std::cerr << "invalid frame count: " << value << '\n';
            return 2;
        }
        frames = static_cast<std::size_t>(parsed);
    }
    zgui::Theme theme = zgui::core::default_theme;
    theme.accent = {105, 85, 235};
    zgui::core::set_theme(theme);
    return zgui::sdl2_opengl::run_sdl2_opengl_application(
        "zgui example window", 454, 456,
        [](zgui::WindowManager& manager, zgui::sdl2_opengl::OpenGLGraphics&) { build_ui(manager); },
        frames);
}
