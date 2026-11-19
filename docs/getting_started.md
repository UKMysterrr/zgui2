# Getting started

See the [build instructions](../README.md#build-and-run) for the core and
`zgui_example`, and [FetchContent usage](../README.md#using-zgui-with-fetchcontent)
for integrating the `src/` public headers in another CMake project.

The core owns no platform window. Initialize it with application-owned
`Graphics` and `Input` implementations, build a `WindowManager`, then call
`update()` and `draw()` once per frame while the graphics backend's frame is
active.

## Build a window

```cpp
#include "zgui.hh"

void build_ui(zgui::WindowManager& manager) {
    auto window = std::make_shared<zgui::Window>("Settings");
    window->set_size({480.f, 360.f});

    auto tabs = std::make_shared<zgui::TabBar>();
    tabs->set_position({8.f, 30.f});
    tabs->set_size({464.f, 322.f});
    auto page = tabs->add_tab("General");
    auto group = std::make_shared<zgui::GroupBox>("Display");
    group->set_position({8.f, 8.f});
    group->set_size({448.f, 270.f});
    auto layout = zgui::StackLayout(*group);
    auto enabled = zgui::bound(true);
    layout.add_row(std::make_shared<zgui::CheckBox>("Enabled", enabled),
                   zgui::spacing::kCheckboxRowPitch);
    page->add_child(group);
    window->add_child(tabs);
    manager.add_window(window);
}
```

The values in `Point`, widget sizes, positions, and spacing constants are
floating-point pixel coordinates. Coordinates are relative to the parent;
`get_absolute_position()` returns the screen-space location.

## Initialize and run frames

```cpp
#include "zgui_impl_sdl2_opengl.hh"

void render_frame(zgui::WindowManager& manager,
                  zgui::sdl2_opengl::OpenGLGraphics& graphics,
                  zgui::sdl2_opengl::SDL2Input& input,
                  int width, int height) {
    input.begin_frame();
    graphics.begin_frame(width, height);
    manager.update();
    manager.draw();
    graphics.end_frame();
}
```

Poll platform events after `begin_frame()` and before updating the manager.
Handle resize events by updating the backend viewport and any application
windows whose size should follow the host. The helper
`run_sdl2_opengl_application()` owns this lifecycle for SDL2/OpenGL apps;
standalone integrations call `core::initialize(graphics, input)` before the
first frame and `core::shutdown()` before releasing the backend/context.
An overload accepts a frame limit and callback, invoked after platform events
are polled and before manager update; this is useful for frame metrics and
bounded smoke runs.

## Themes and bindings

`core::set_theme(theme)` replaces the active theme for subsequent drawing.
`core::reset_theme()` restores `core::default_theme`. Pointer and reference
bindings do not own their values; use `Bound<T>` when the widget should retain
shared ownership:

```cpp
void use_owned_value(zgui::Window& window) {
    auto accent = zgui::bound(zgui::Color(255, 140, 0));
    auto swatch = std::make_shared<zgui::ColorSwatch>("Accent", accent);
    window.add_child(swatch);
}
```

## Resize behavior

Use anchors to keep a child's selected edges at fixed margins, or constraints
to calculate each edge from a parent-size fraction plus an offset. Constraints
are suitable for split columns and fill-to-parent regions. Call
`WindowManager::validate_layout()` during development to check child bounds,
finite non-negative dimensions, and sibling overlap.
