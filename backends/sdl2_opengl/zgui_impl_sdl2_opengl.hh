// Copyright (c) 2019, zxvnme
// Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
// SPDX-License-Identifier: MIT

#ifndef ZGUI_IMPL_SDL2_OPENGL_HH
#define ZGUI_IMPL_SDL2_OPENGL_HH

#include "../../zgui.hh"

#include <GL/gl.h>
#include <SDL2/SDL.h>

#include <deque>
#include <string>
#include <unordered_set>
#include <vector>

namespace zgui::sdl2_opengl {

    /// OpenGL 2.1 renderer that saves/restores selected host state per frame.
    class OpenGLGraphics : public Graphics {
    private:
        struct Batch {
            VertexTopology topology;
            std::vector<Vertex> vertices;
        };

        using UseProgramProc = void (*)(GLuint);

        std::vector<Batch> batches_;
        VertexTopology pending_topology_ = VertexTopology::TRIANGLE_LIST;
        GLuint font_texture_ = 0;
        int viewport_width_ = 0;
        int viewport_height_ = 0;
        GLint previous_matrix_mode_ = GL_MODELVIEW;
        GLint previous_program_ = 0;
        UseProgramProc use_program_ = nullptr;
        bool frame_active_ = false;

        void initialize_font();
        void flush_geometry();
        void set_topology(VertexTopology topology) override;
        void append_geometry(const std::vector<Vertex>& vertices, std::size_t count) override;

    public:
        ~OpenGLGraphics() override = default;

        /// Begins drawing to a positive-size viewport in pixels.
        void begin_frame(int viewport_width, int viewport_height);
        /// Flushes queued drawing and restores saved host matrix/program state.
        void end_frame();
        /// Deletes the cached font texture while an OpenGL context is current.
        void release_font_texture();
        /// Releases backend-owned resources while an OpenGL context is current.
        void release_resources();
        /// Checks the built-in font's glyph spacing and optionally returns a diagnostic.
        bool font_spacing_valid(std::string* error = nullptr) const;
        /// Returns the active viewport dimensions in pixels.
        Point viewport_size() const override;

        void draw() override;
        void reset() override;
        void set_clip_rect(int x, int y, int w, int h) override;
        void reset_clip_rect() override;
        void draw_text(const std::string& text, Point position, Color color) override;
        float measure_text_width(const std::string& text) const override;
    };

    /// Converts SDL2 events into zgui's frame-based input interface.
    class SDL2Input : public Input {
    private:
        Point mouse_position_{0.f, 0.f};
        Point mouse_delta_{0.f, 0.f};
        bool left_button_pressed_ = false;
        bool middle_button_pressed_ = false;
        bool right_button_pressed_ = false;
        bool mouse_double_clicked_ = false;
        bool escape_pressed_ = false;
        bool have_last_click_ = false;
        Point last_click_position_{0.f, 0.f};
        Uint32 last_click_time_ = 0;
        int mouse_wheel_delta_ = 0;
        std::unordered_set<int> keys_down_;
        std::deque<char> text_input_;
        std::deque<int> pressed_keys_;

    public:
        SDL2Input();

        /// Clears transient deltas/events while preserving held state.
        void begin_frame();
        /// Updates input state from one SDL event.
        void process_event(const SDL_Event& event);
        bool key_pressed(int key) const override;
        bool mouse_over(Point position, Point size) const override;
        Point mouse_position() const override;
        bool mouse_button_pressed() const override;
        bool mouse_button_down(MouseButton button) const override;
        Point mouse_delta() const override;
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
    };

    /// Setup callback invoked once after the SDL window and backend are ready.
    using ApplicationSetup = std::function<void(WindowManager&, OpenGLGraphics&)>;
    /// Per-frame application callback invoked after SDL events are processed.
    using ApplicationFrame =
        std::function<void(WindowManager&, OpenGLGraphics&, SDL2Input&, std::size_t)>;
    /// Runs an SDL2/OpenGL window and dispatches frames until quit or Escape.
    int run_sdl2_opengl_application(const std::string& title, int width, int height,
                                    const ApplicationSetup& setup);
    /// Runs the application with an optional frame limit and per-frame callback.
    int run_sdl2_opengl_application(const std::string& title, int width, int height,
                                    const ApplicationSetup& setup, std::size_t frame_limit,
                                    const ApplicationFrame& on_frame = {});

} // namespace zgui::sdl2_opengl

#endif // ZGUI_IMPL_SDL2_OPENGL_HH
