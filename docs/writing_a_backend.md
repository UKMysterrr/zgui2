# Writing a backend

The backend maps zgui's pixel-space drawing and input contract to a host
graphics/input API. The core is single-threaded and has one active graphics and
input pair, installed with `core::initialize()`.

## Required methods

`Graphics` requires `draw`, `reset`, `set_clip_rect`, `reset_clip_rect`,
`draw_text`, and `measure_text_width`. `flush()` defaults to `draw()`;
`viewport_size()` defaults to zero. `append_geometry()` and `set_topology()`
have core implementations and may be overridden for batching.

`Input` requires `key_pressed`, `mouse_over`, `mouse_position`,
`mouse_button_pressed`, `mouse_wheel_delta`, `pop_text_input`,
`poll_pressed_key`, and `key_name`. Extra cursor, delete, held-button, mouse
delta, double-click, and Escape methods have default implementations.

## Coordinates and rendering

- Positions and sizes are floating-point pixels; the origin is top-left and y
  increases downward. Integer clipping rectangles are also in pixels.
- `push_clip_rect()` intersects each clip with its parent. A zero or negative
  size becomes an empty clip. `pop_clip_rect()` restores the parent clip or
  calls `reset_clip_rect()` when the stack becomes empty.
- `draw_text()` and `measure_text_width()` must use the same printable-ASCII
  glyph advances. Unsupported bytes are mapped to `?` by the SDL backend.
- Geometry is submitted with `VertexTopology`: line lists/strips and triangle
  lists/strips. Preserve submission order when flushing mixed topologies.
- `flush()` must make pending geometry visible before later immediate-mode
  text or clip changes that depend on draw order. `reset()` clears frame-local
  buffers and backend state.
- `mouse_button_pressed()` is a one-shot press transition. Use
  `mouse_button_down(MouseButton)` for held state. `begin_frame()` should clear
  transient wheel/delta/double-click/press-transition values, but not held
  keys or buttons; process all host events before `WindowManager::update()`.

## Minimal backend skeleton

```cpp
#include "zgui.hh"

class ExampleGraphics final : public zgui::Graphics {
public:
    void draw() override {}
    void reset() override {}
    void set_clip_rect(int, int, int, int) override {}
    void reset_clip_rect() override {}
    void draw_text(const std::string&, zgui::Point, zgui::Color) override {}
    float measure_text_width(const std::string& text) const override {
        return static_cast<float>(text.size()) * 8.f;
    }
    zgui::Point viewport_size() const override { return {800.f, 600.f}; }
};

class ExampleInput final : public zgui::Input {
public:
    bool key_pressed(int) const override { return false; }
    bool mouse_over(zgui::Point, zgui::Point) const override { return false; }
    zgui::Point mouse_position() const override { return {}; }
    bool mouse_button_pressed() const override { return false; }
    int mouse_wheel_delta() const override { return 0; }
    bool pop_text_input(char&) override { return false; }
    std::optional<int> poll_pressed_key() override { return std::nullopt; }
    std::string key_name(int) const override { return {}; }
};
```

The skeleton compiles but deliberately draws nothing; a real backend must
implement all graphics and input methods.

## Frame order and state preservation

Each frame clears transient input, drains the platform event queue, begins
backend drawing, updates and draws the manager, ends drawing, then presents:

1. `input.begin_frame()`; dispatch every host event to the input adapter.
2. Update the host viewport and call the backend's begin-frame hook.
3. `manager.update()`, then `manager.draw()`.
4. End the backend frame and present/swap the host buffer.

The SDL2/OpenGL backend's `begin_frame()`/`end_frame()` saves and restores
matrix mode and the active shader program. A backend embedded in a host
renderer should save and restore every piece of state it changes. For an
OpenGL overlay, preserve the host's relevant GL bindings and viewport. A
Direct3D 9/11 backend should capture and restore render targets, viewport,
blend/depth/rasterizer state, shaders, and resources it changes. A Vulkan
overlay should record commands into the host's render pass/command buffer and
respect its synchronization and descriptor ownership; it must not submit or
reset host-owned work.
