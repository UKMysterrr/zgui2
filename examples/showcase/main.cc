// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#include "showcase_ui.hh"
#include "../../backends/sdl2_opengl/zgui_impl_sdl2_opengl.hh"

#include <SDL2/SDL.h>

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace {
    bool parse_arguments(int argc, char** argv, std::size_t& frames, std::string& theme,
                         bool& fuzz) {
        for (int index = 1; index < argc; ++index) {
            const std::string argument(argv[index]);
            if (argument == "--fuzz") {
                fuzz = true;
            } else if (argument == "--frames" && index + 1 < argc) {
                const std::string value(argv[++index]);
                std::uint64_t parsed = 0;
                const auto result =
                    std::from_chars(value.data(), value.data() + value.size(), parsed);
                if (result.ec != std::errc{} || result.ptr != value.data() + value.size() ||
                    parsed > std::numeric_limits<std::size_t>::max()) {
                    std::cerr << "invalid frame count: " << value << '\n';
                    return false;
                }
                frames = static_cast<std::size_t>(parsed);
            } else if (argument == "--theme" && index + 1 < argc) {
                theme = argv[++index];
            } else {
                std::cerr << "usage: zgui_showcase [--frames N] [--theme dark|light|default] "
                             "[--fuzz]\n";
                return false;
            }
        }
        if (theme != "dark" && theme != "light" && theme != "default") {
            std::cerr << "unknown theme: " << theme << '\n';
            return false;
        }
        return true;
    }

    zgui::Theme theme_for(const std::string& name) {
        zgui::Theme theme = zgui::core::default_theme;
        if (name == "light") {
            theme.window_bg = {238, 240, 244};
            theme.window_header = {220, 224, 230};
            theme.groupbox_bg = {246, 247, 249};
            theme.border_outer = {95, 100, 108};
            theme.border_inner = {180, 184, 190};
            theme.accent = {30, 110, 210};
            theme.text_primary = {25, 28, 32};
            theme.text_dim = {90, 96, 104};
            theme.control_bg = {230, 233, 238};
            theme.control_hover = {210, 216, 224};
        }
        return theme;
    }

    void queue_fuzz_events(zgui_showcase::AppState& state, std::size_t frame,
                           zgui::Point viewport) {
        const auto random = [&state]() {
            state.fuzz_state ^= state.fuzz_state << 13;
            state.fuzz_state ^= state.fuzz_state >> 17;
            state.fuzz_state ^= state.fuzz_state << 5;
            return state.fuzz_state;
        };
        const int width = std::max(1, static_cast<int>(viewport.x));
        const int height = std::max(1, static_cast<int>(viewport.y));
        SDL_Event motion{};
        motion.type = SDL_MOUSEMOTION;
        motion.motion.x = static_cast<int>(random() % static_cast<std::uint32_t>(width));
        motion.motion.y = static_cast<int>(random() % static_cast<std::uint32_t>(height));
        if (SDL_PushEvent(&motion) < 0)
            std::cerr << "unable to queue fuzz mouse motion: " << SDL_GetError() << '\n';

        if (frame % 2 == 0) {
            state.fuzz_mouse_down = true;
        } else {
            state.fuzz_mouse_down = false;
        }
        SDL_Event button{};
        button.type = state.fuzz_mouse_down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
        button.button.button = SDL_BUTTON_LEFT;
        button.button.state = state.fuzz_mouse_down ? SDL_PRESSED : SDL_RELEASED;
        button.button.x = motion.motion.x;
        button.button.y = motion.motion.y;
        if (SDL_PushEvent(&button) < 0)
            std::cerr << "unable to queue fuzz mouse button: " << SDL_GetError() << '\n';

        if (frame % 9 == 0) {
            SDL_Event wheel{};
            wheel.type = SDL_MOUSEWHEEL;
            wheel.wheel.y = static_cast<int>(random() % 3u) - 1;
            if (SDL_PushEvent(&wheel) < 0)
                std::cerr << "unable to queue fuzz wheel: " << SDL_GetError() << '\n';
        }
    }
} // namespace

int main(int argc, char** argv) {
    std::size_t frame_limit = 0;
    std::string theme_name = "dark";
    bool fuzz = false;
    if (!parse_arguments(argc, argv, frame_limit, theme_name, fuzz))
        return 2;

    zgui_showcase::AppState state;
    state.theme = theme_for(theme_name);
    zgui::core::set_theme(state.theme);
    return zgui::sdl2_opengl::run_sdl2_opengl_application(
        "zgui C++17 Showcase", 1000, 720,
        [&state](zgui::WindowManager& manager, zgui::sdl2_opengl::OpenGLGraphics&) {
            zgui_showcase::build_showcase_ui(manager, state);
        },
        frame_limit,
        [&state, fuzz](zgui::WindowManager&, zgui::sdl2_opengl::OpenGLGraphics& graphics,
                       zgui::sdl2_opengl::SDL2Input&, std::size_t frame) {
            const std::uint64_t now = SDL_GetPerformanceCounter();
            const std::uint64_t frequency = SDL_GetPerformanceFrequency();
            if (state.previous_counter != 0 && frequency != 0) {
                state.frame_seconds = static_cast<double>(now - state.previous_counter) /
                                      static_cast<double>(frequency);
            }
            state.previous_counter = now;
            const double fps = state.frame_seconds > 0.0 ? 1.0 / state.frame_seconds : 0.0;
            state.status->set_text(
                "zgui " ZGUI_VERSION_STRING " | FPS " + std::to_string(static_cast<int>(fps)) +
                " | widgets " + std::to_string(state.main_window->widget_count()) + " | visible " +
                std::to_string(state.main_window->visible_widget_count()));
            if (state.frame_time) {
                std::ostringstream text;
                text << std::fixed << std::setprecision(2)
                     << "Frame: " << state.frame_seconds * 1000.0 << " ms | widgets "
                     << *state.stress_count.get();
                state.frame_time->set_text(text.str());
            }
            if (fuzz)
                queue_fuzz_events(state, frame, graphics.viewport_size());
        });
}
