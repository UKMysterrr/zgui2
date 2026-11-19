// Copyright (c) 2019, zxvnme
// Copyright (c) 2026 UKMysterrr
// SPDX-License-Identifier: MIT

#include "zgui_impl_sdl2_opengl.hh"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <string>

namespace {
    using namespace zgui;

    constexpr std::array<const char*, 8> workspaces{
        "Overview", "Controls", "Canvas", "Colors", "Layout", "Inspector", "Shortcuts", "About"};

    struct State {
        int amount = 25;
        float scale = 1.f;
        bool enabled = true;
        bool pinned = false;
        bool guides = false;
        std::size_t mode = 0;
        std::vector<bool> options{true, true, false};
        int key = SDLK_LALT;
        std::string username;
        std::string password;
        std::size_t quality = 1;
        std::size_t theme = 0;
        float progress = 0.25f;
        float yaw = 0.45f;
        float pitch = -0.35f;
        std::chrono::steady_clock::time_point last_frame = std::chrono::steady_clock::now();
        std::optional<std::size_t> selection;
        std::shared_ptr<Window> inspector;
        std::shared_ptr<KeyBind> key_bind;
        std::shared_ptr<TableView> table;
        std::shared_ptr<Label> detail;
    };

    Theme make_theme(std::size_t index, Color accent) {
        Theme theme = core::default_theme;
        theme.accent = accent;
        if (index == 1) {
            theme.window_bg = {238, 240, 244};
            theme.window_header = {220, 224, 230};
            theme.groupbox_bg = {246, 247, 249};
            theme.border_outer = {95, 100, 108};
            theme.border_inner = {180, 184, 190};
            theme.text_primary = {25, 28, 32};
            theme.text_dim = {90, 96, 104};
            theme.control_bg = {230, 233, 238};
            theme.control_hover = {210, 216, 224};
        }
        return theme;
    }

    void reset_defaults(State& state) {
        const State defaults;
        state.amount = defaults.amount;
        state.scale = defaults.scale;
        state.enabled = defaults.enabled;
        state.pinned = defaults.pinned;
        state.guides = defaults.guides;
        state.mode = defaults.mode;
        state.options = defaults.options;
        state.key = defaults.key;
        state.key_bind->cancel_capture();
        state.username = defaults.username;
        state.password = defaults.password;
        state.quality = defaults.quality;
        state.theme = defaults.theme;
        state.progress = defaults.progress;
        state.yaw = defaults.yaw;
        state.pitch = defaults.pitch;
        state.last_frame = std::chrono::steady_clock::now();
        state.selection = defaults.selection;
        state.inspector->set_always_on_top(false);
        state.table->clear_selection();
        core::set_theme(make_theme(state.theme, {105, 85, 235}));
    }

    template <typename T>
    std::shared_ptr<T> place(UIElement& parent, std::shared_ptr<T> child, Point position,
                             Point size) {
        child->set_position(position);
        child->set_size(size);
        parent.add_child(child);
        return child;
    }

    class ClickableLabel final : public Label {
        std::function<void()> action_;
        std::function<bool()> selected_;

    public:
        ClickableLabel(std::string text, std::function<void()> action,
                       std::function<bool()> selected = {})
            : Label(std::move(text)), action_(std::move(action)), selected_(std::move(selected)) {}

        void draw() override {
            if (selected_ && selected_())
                set_color(core::active_theme.accent);
            else
                clear_color_override();
            Label::draw();
        }

        bool handle_mouse_press(Point point) override {
            if (!contains_point(point))
                return false;
            action_();
            return true;
        }
    };

    template <typename T>
    void add_slider(GroupBox& group, const std::string& title, T& value, T minimum, T maximum,
                    T step, float y) {
        auto slider = place(group, std::make_shared<Slider<T>>(title, value, minimum, maximum),
                            {32.f, y}, {166.f, 28.f});
        place(group,
              std::make_shared<Button>("-", [slider, step] {
                  slider->set_value(slider->get_value() - step);
              }),
              {10.f, y + 13.f}, {18.f, 18.f});
        place(group,
              std::make_shared<Button>("+", [slider, step] {
                  slider->set_value(slider->get_value() + step);
              }),
              {202.f, y + 13.f}, {18.f, 18.f});
    }

    void build_classic(UIElement& page, State& state) {
        auto left = place(page, std::make_shared<GroupBox>("Centered title"), {8.f, 8.f},
                          {232.f, 466.f});
        left->set_fit_content(false);
        left->set_title_alignment(TextAlignment::center);
        place(*left, std::make_shared<CheckBox>("Enable live view", state.enabled), {10.f, 25.f},
              {205.f, 18.f});
        place(*left, std::make_shared<CheckBox>("Show guides", state.guides), {10.f, 47.f},
              {205.f, 18.f});
        add_slider(*left, "Amount", state.amount, 0, 100, 1, 76.f);
        add_slider(*left, "Canvas scale", state.scale, 0.5f, 2.f, 0.1f, 123.f);
        place(*left, std::make_shared<Button>("Reset view", [&state] {
                  state.amount = 25;
                  state.scale = 1.f;
                  state.yaw = 0.45f;
                  state.pitch = -0.35f;
              }),
              {10.f, 174.f}, {95.f, 24.f});
        place(*left, std::make_shared<ToggleButton>("Pin inspector", state.pinned, [&state] {
                  state.inspector->set_always_on_top(state.pinned);
              }), {115.f, 174.f},
              {105.f, 24.f});
        place(*left, std::make_shared<Label>("Display mode"), {10.f, 217.f}, {200.f, 16.f});
        auto combo = place(*left,
                           std::make_shared<ComboBox>(
                               std::vector<std::string>{"Windowed", "Borderless", "Fullscreen"}),
                           {10.f, 235.f}, {210.f, 22.f});
        combo->bind_selected_index(state.mode);
        place(*left, std::make_shared<Label>("Visible layers"), {10.f, 272.f}, {200.f, 16.f});
        place(*left,
              std::make_shared<MultiComboBox>(
                  std::vector<std::string>{"Wireframe", "Guides", "Bounding Box"}, state.options),
              {10.f, 290.f}, {210.f, 22.f});
        place(*left, std::make_shared<Label>("Shortcut"), {10.f, 327.f}, {200.f, 16.f});
        state.key_bind =
            place(*left, std::make_shared<KeyBind>("", state.key), {10.f, 345.f}, {210.f, 22.f});

        auto right = place(page, std::make_shared<GroupBox>("Default title"), {248.f, 8.f},
                           {232.f, 466.f});
        right->set_fit_content(false);
        auto login = place(*right, std::make_shared<GroupBox>("Login form"), {10.f, 22.f},
                           {210.f, 130.f});
        login->set_fit_content(false);
        login->set_title_alignment(TextAlignment::center);
        place(*login, std::make_shared<Label>("Username"), {10.f, 21.f}, {185.f, 16.f});
        place(*login, std::make_shared<TextInput>(state.username), {10.f, 39.f}, {185.f, 22.f});
        place(*login, std::make_shared<Label>("Password"), {10.f, 68.f}, {185.f, 16.f});
        auto password = place(*login, std::make_shared<TextInput>(state.password), {10.f, 86.f},
                              {185.f, 22.f});
        password->set_password_mode(true);
        place(*right, std::make_shared<Label>("Select a workspace"), {10.f, 166.f},
              {210.f, 16.f});
        auto scroll = place(*right, std::make_shared<ScrollArea>(), {10.f, 185.f}, {210.f, 150.f});
        for (std::size_t i = 0; i < workspaces.size(); ++i) {
            place(*scroll,
                  std::make_shared<ClickableLabel>(workspaces[i], [&state, i] {
                      if (state.selection == i)
                          state.selection.reset();
                      else
                          state.selection = i;
                  }, [&state, i] { return state.selection == i; }),
                  {8.f, 5.f + static_cast<float>(i) * 22.f}, {175.f, 18.f});
        }
        place(*right, std::make_shared<Label>("Selection appears in the inspector."), {10.f, 379.f},
              {210.f, 18.f});
    }

    void build_extended(UIElement& page, State& state) {
        auto group = place(page, std::make_shared<GroupBox>("Live canvas"), {8.f, 8.f},
                           {472.f, 330.f});
        group->set_fit_content(false);
        place(*group, std::make_shared<Label>("Choose the menu and cube accent color."),
              {12.f, 25.f}, {440.f, 18.f});
        place(*group, std::make_shared<ColorSwatch>("Accent", core::active_theme.accent), {12.f, 50.f},
              {250.f, 24.f});
        place(*group, std::make_shared<Label>("Pick opens an HSV / alpha editor."), {12.f, 82.f},
              {400.f, 18.f});
        place(*group, std::make_shared<Label>("Progress follows the Amount slider."),
              {12.f, 116.f}, {400.f, 18.f});
        place(*group, std::make_shared<ProgressBar>(state.progress), {12.f, 138.f}, {300.f, 20.f});
        place(*group, std::make_shared<Label>("Render detail"), {12.f, 177.f}, {220.f, 18.f});
        place(*group,
              std::make_shared<RadioGroup>(
                  std::vector<std::string>{"Low", "Balanced", "High"}, state.quality),
              {12.f, 198.f}, {220.f, 54.f});
        place(*group, std::make_shared<Label>("Hover here for a tip."), {12.f, 278.f},
              {220.f, 18.f});
        place(*group, std::make_shared<Tooltip>("Drag to rotate, scroll to zoom, double-click to reset."),
              {12.f, 278.f}, {220.f, 18.f});
    }

    void build_inspector(WindowManager& manager, State& state) {
        auto window = std::make_shared<Window>("Inspector & Live View");
        window->set_position({530.f, 36.f});
        window->set_minimum_size(410.f, 490.f);
        window->set_size({450.f, 570.f});
        state.inspector = window;
        place(*window, std::make_shared<Label>("Theme"), {12.f, 32.f}, {75.f, 18.f});
        place(*window,
              std::make_shared<RadioGroup>(
                  std::vector<std::string>{"Dark", "Light"}, state.theme,
                  [&state](std::size_t) {
                      core::set_theme(make_theme(state.theme, core::active_theme.accent));
                  }),
              {90.f, 28.f}, {100.f, 36.f});
        place(*window, std::make_shared<Button>("Reset Defaults", [&state] {
                  reset_defaults(state);
              }),
              {280.f, 32.f}, {150.f, 24.f})->set_anchors(false, true, true, false);
        place(*window, std::make_shared<Label>("Drag column edges; click a row."), {12.f, 72.f},
              {400.f, 18.f});
        state.table = place(*window, std::make_shared<TableView>(), {12.f, 94.f}, {426.f, 170.f});
        state.table->set_anchors(true, true, true, false);
        state.table->set_columns({{"Property", 130.f}, {"Live value", 130.f, true, 1.f}});
        for (const char* name : {"Amount", "Scale", "Workspace", "Mode", "Enabled",
                                 "An intentionally long property name for truncation"}) {
            state.table->add_row(std::vector<std::string>{name, ""});
        }
        state.detail = place(*window, std::make_shared<Label>("Select a row to inspect it."),
                             {12.f, 272.f}, {400.f, 18.f});
        state.table->set_on_selection_changed([&state](std::size_t row) {
            state.detail->set_text(row == TableView::no_selection
                                       ? "Select a row to inspect it."
                                       : "Selected row " + std::to_string(row + 1));
        });
        place(*window, std::make_shared<Label>("Live view: drag to rotate; scroll to zoom."),
              {12.f, 300.f}, {410.f, 18.f});
        auto viewport = place(*window, std::make_shared<Viewport>(), {12.f, 322.f}, {426.f, 230.f});
        viewport->set_anchors(true, true, true, true);
        viewport->set_on_input([&state](const ViewportInput& input) {
            if (input.double_clicked) {
                state.yaw = 0.45f;
                state.pitch = -0.35f;
                state.scale = 1.f;
            } else {
                if (input.rotating) {
                    state.yaw += input.mouse_delta.x * 0.01f;
                    state.pitch = std::clamp(state.pitch + input.mouse_delta.y * 0.01f, -1.3f, 1.3f);
                }
                state.scale = std::clamp(state.scale + static_cast<float>(input.wheel_delta) * 0.05f,
                                         0.5f, 2.f);
            }
        });
        viewport->set_on_render([&state](int x, int y, int width, int height) {
            auto& graphics = *core::graphics;
            if (!state.enabled)
                return;
            struct Vec3 { float x, y, z; };
            const float cx = static_cast<float>(x) + static_cast<float>(width) / 2.f;
            const float cy = static_cast<float>(y) + static_cast<float>(height) / 2.f;
            const float focal = static_cast<float>(std::min(width, height)) * 1.6f * state.scale;
            const float yaw_cos = std::cos(state.yaw), yaw_sin = std::sin(state.yaw);
            const float pitch_cos = std::cos(state.pitch), pitch_sin = std::sin(state.pitch);
            const auto project = [&](Vec3 point) {
                const float rotated_x = yaw_cos * point.x + yaw_sin * point.z;
                const float rotated_z = -yaw_sin * point.x + yaw_cos * point.z;
                const float rotated_y = pitch_cos * point.y - pitch_sin * rotated_z;
                const float depth = 6.f + pitch_sin * point.y + pitch_cos * rotated_z;
                return Point(cx + focal * rotated_x / depth, cy - focal * rotated_y / depth);
            };
            const auto line = [&](Vec3 a, Vec3 b, Color color) {
                graphics.draw_line(project(a), project(b), color);
            };
            if (state.guides && state.options[1]) {
                const int subdivisions =
                    2 + state.amount * static_cast<int>(state.quality + 1) / 20;
                for (int i = 0; i <= subdivisions; ++i) {
                    const float offset = -2.f + 4.f * static_cast<float>(i) /
                                                    static_cast<float>(subdivisions);
                    line({offset, -1.25f, -2.f}, {offset, -1.25f, 2.f},
                         core::active_theme.border_inner);
                    line({-2.f, -1.25f, offset}, {2.f, -1.25f, offset},
                         core::active_theme.border_inner);
                }
                const Vec3 origin{0.f, -1.25f, 0.f};
                line(origin, {2.f, -1.25f, 0.f}, {225, 75, 75});
                line(origin, {0.f, 1.25f, 0.f}, {75, 190, 100});
                line(origin, {0.f, -1.25f, 2.f}, {90, 145, 240});
            }
            const std::array<Vec3, 8> vertices{{
                {-1.f, -1.f, -1.f}, {1.f, -1.f, -1.f}, {1.f, 1.f, -1.f},
                {-1.f, 1.f, -1.f}, {-1.f, -1.f, 1.f}, {1.f, -1.f, 1.f},
                {1.f, 1.f, 1.f}, {-1.f, 1.f, 1.f}}};
            const std::array<std::array<std::size_t, 2>, 12> edges{{
                {{0, 1}}, {{1, 2}}, {{2, 3}}, {{3, 0}},
                {{4, 5}}, {{5, 6}}, {{6, 7}}, {{7, 4}},
                {{0, 4}}, {{1, 5}}, {{2, 6}}, {{3, 7}}}};
            const auto box = [&](float size, Color color) {
                for (const auto& edge : edges) {
                    const Vec3 a = vertices[edge[0]], b = vertices[edge[1]];
                    line({a.x * size, a.y * size, a.z * size},
                         {b.x * size, b.y * size, b.z * size}, color);
                }
            };
            if (state.options[0])
                box(1.f, core::active_theme.accent);
            if (state.options[2])
                box(1.2f, core::active_theme.text_dim);
        });
        manager.add_window(window);
    }

    void build_ui(WindowManager& manager, State& state) {
        auto window = std::make_shared<Window>("zgui 2.1 — Controls");
        window->set_position({16.f, 16.f});
        window->set_minimum_size(504.f, 540.f);
        window->set_size({504.f, 540.f});
        auto tabs = place(*window, std::make_shared<TabBar>(), {8.f, 28.f}, {488.f, 502.f});
        tabs->set_anchors(true, true, true, true);
        auto classic = tabs->add_tab("Classic");
        auto extended = tabs->add_tab("Extended");
        build_classic(*classic, state);
        build_extended(*extended, state);
        manager.add_window(window);
        build_inspector(manager, state);
    }

    bool parse_arguments(int argc, char** argv, std::size_t& frames, std::size_t& theme) {
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--frames" && i + 1 < argc) {
                const std::string value = argv[++i];
                std::uint64_t parsed = 0;
                const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
                if (result.ec != std::errc{} || result.ptr != value.data() + value.size() ||
                    parsed == 0 || parsed > std::numeric_limits<std::size_t>::max()) {
                    std::cerr << "invalid frame count: " << value << '\n';
                    return false;
                }
                frames = static_cast<std::size_t>(parsed);
            } else if (arg == "--theme" && i + 1 < argc) {
                const std::string value = argv[++i];
                if (value != "dark" && value != "light") {
                    std::cerr << "invalid theme: " << value << '\n';
                    return false;
                }
                theme = value == "light" ? 1u : 0u;
            } else {
                std::cerr << "usage: zgui_example [--frames N] [--theme dark|light]\n";
                return false;
            }
        }
        return true;
    }
} // namespace

int main(int argc, char** argv) {
    std::size_t frames = 0;
    State state;
    if (!parse_arguments(argc, argv, frames, state.theme))
        return 2;
    core::set_theme(make_theme(state.theme, {105, 85, 235}));
    return sdl2_opengl::run_sdl2_opengl_application(
        "zgui example", 1000, 640,
        [&state](WindowManager& manager, sdl2_opengl::OpenGLGraphics&) {
            build_ui(manager, state);
        },
        frames,
        [&state](WindowManager&, sdl2_opengl::OpenGLGraphics&, sdl2_opengl::SDL2Input&,
                 std::size_t) {
            const auto now = std::chrono::steady_clock::now();
            const float elapsed =
                std::min(0.1f, std::chrono::duration<float>(now - state.last_frame).count());
            state.last_frame = now;
            if (state.enabled)
                state.yaw = std::fmod(
                    state.yaw + elapsed * (0.1f + 0.005f * static_cast<float>(state.amount)),
                    6.2831853f);
            state.progress = static_cast<float>(state.amount) / 100.f;
            const std::vector<std::string> values{
                std::to_string(state.amount), std::to_string(state.scale),
                state.selection ? workspaces[*state.selection] : "None",
                std::vector<std::string>{"Windowed", "Borderless", "Fullscreen"}[state.mode],
                state.enabled ? "Yes" : "No", "Resize this column to see the full text"};
            for (std::size_t i = 0; i < values.size(); ++i)
                state.table->set_cell(i, 1, {values[i], std::nullopt, false});
        });
}
