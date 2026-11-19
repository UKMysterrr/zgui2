# zgui

zgui is a retained-mode, dependency-free C++17 GUI toolkit. Its core provides
widgets, layout, clipping, and window management; applications provide graphics
and input implementations.

This directory is intended to replace the contents of a fork of
[zxvnme/zgui](https://github.com/zxvnme/zgui). It keeps the upstream license
notice in [LICENSE](LICENSE).

## Features

- Retained widget tree with explicit shared ownership or non-owning child links.
- Windows, overlays, tabs, group boxes, anchors, constraints, and stack layouts.
- Input, selection, text, color, table, and viewport widgets.
- Theme customization, clipped drawing, text measurement, and validation.
- Optional SDL2/OpenGL 2.1 backend.

## Hello world

This example opens a window containing one tab and a label:

```cpp
#include "zgui_impl_sdl2_opengl.hh"

int main() {
    using namespace zgui;
    return sdl2_opengl::run_sdl2_opengl_application(
        "zgui", 640, 480, [](WindowManager& manager, sdl2_opengl::OpenGLGraphics&) {
            auto window = std::make_shared<Window>("Hello zgui");
            window->set_size({420.f, 260.f});
            auto tabs = std::make_shared<TabBar>();
            tabs->set_position({8.f, 30.f});
            tabs->set_size({404.f, 220.f});
            auto page = tabs->add_tab("Welcome");
            auto layout = StackLayout::page(*page);
            layout.add_row(std::make_shared<Label>("Hello, zgui!"));
            window->add_child(tabs);
            manager.add_window(window);
        });
}
```

## Build and test

The core requires CMake 3.14+ and a C++17 compiler. SDL2 and OpenGL
development packages are needed for the optional backend and interactive
showcase.

```sh
cmake --preset release
cmake --build --preset release
ctest --preset release --output-on-failure
```

Other presets are `debug`, `asan-ubsan`, `coverage`, and `no-exceptions`.
Build and run the ten-page public-API showcase with:

```sh
cmake -S . -B build/showcase -DCMAKE_BUILD_TYPE=Release \
  -DZGUI_BUILD_EXAMPLES=ON
cmake --build build/showcase
build/showcase/examples/zgui_showcase --frames 60 --theme light --fuzz
```

To install the library and CMake package:

```sh
cmake --preset release
cmake --build --preset release
cmake --install build/release --prefix install
```

Consumers can use `find_package(zgui CONFIG REQUIRED)` and link
`zgui::zgui`; the optional SDL2/OpenGL target is `zgui::sdl2_opengl`. The
`zgui::testing` target provides the headless test helpers.

The source packer creates a distributable `zgui.hh` and `zgui.cc` pair under
`dist/`:

```sh
python3 tools/pack.py
```

## Attribution and limitations

zgui is derived from [zxvnme/zgui 2.0](https://github.com/zxvnme/zgui) and is
licensed under the MIT License. The optional SDL2/OpenGL backend's embedded
glyph atlas is derived from Liberation Sans; see
[FONT_LICENSE.txt](backends/sdl2_opengl/FONT_LICENSE.txt).

Text is printable ASCII only, the core has one active context and is
single-threaded, and the included renderer uses the OpenGL 2.1 fixed-function
pipeline. The OpenGL backend requires an active compatible context.
