# zgui

zgui is a retained-mode, dependency-free C++17 GUI toolkit. Its core provides
widgets, layout, clipping, and window management; applications provide graphics
and input implementations.

It keeps the upstream license notice in [LICENSE](LICENSE).
The old v1 codebase remains available on the
[`legacy` branch](https://github.com/UKMysterrr/zgui2/tree/legacy).

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
build/showcase/examples/zgui_classic_example
```

The classic login form is a widget demonstration, not an authentication
system. Its password field masks display only; its value remains in memory.

## Using zgui in a CMake project

The SDL2/OpenGL backend requires SDL2 and OpenGL development packages. To
fetch zgui from GitHub, add the following to your project's `CMakeLists.txt`
alongside your own `main.cc` and `tests.cc`:

```cmake
cmake_minimum_required(VERSION 3.14)
project(my_app LANGUAGES CXX)
include(FetchContent)

set(ZGUI_BUILD_BACKEND_SDL2_OPENGL ON CACHE BOOL "")
FetchContent_Declare(zgui
  GIT_REPOSITORY https://github.com/UKMysterrr/zgui2.git
  GIT_TAG main) # Pin a commit for reproducible builds.
FetchContent_MakeAvailable(zgui)

add_executable(my_app main.cc)
target_link_libraries(my_app PRIVATE zgui::zgui zgui::sdl2_opengl)
add_executable(my_tests tests.cc)
target_link_libraries(my_tests PRIVATE zgui::testing)
```

Alternatively, install zgui and its CMake package (the backend is included
when its dependencies are available):

```sh
cmake --preset release
cmake --build --preset release
cmake --install build/release --prefix /absolute/path/to/zgui-install
```

Then use this in the external project's `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.14)
project(my_app LANGUAGES CXX)
find_package(zgui CONFIG REQUIRED COMPONENTS sdl2_opengl)

add_executable(my_app main.cc)
target_link_libraries(my_app PRIVATE zgui::zgui zgui::sdl2_opengl)
add_executable(my_tests tests.cc)
target_link_libraries(my_tests PRIVATE zgui::testing)
```

Configure it with `cmake -S . -B build -DCMAKE_PREFIX_PATH=/absolute/path/to/zgui-install`.
For a core-only application, omit the component and the backend target.
Include `<zgui.hh>` for the core, `<zgui_impl_sdl2_opengl.hh>` for the backend,
and `<test_context.hh>` for the headless test helpers.

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
