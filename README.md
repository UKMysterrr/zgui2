# zgui

zgui is a retained-mode, dependency-free C++17 GUI toolkit. The core in
[`src/`](src/) provides widgets, layout, clipping, and window management;
applications provide graphics and input implementations. An optional SDL2 /
OpenGL 2.1 backend is included.

The upstream license notice is in [LICENSE](LICENSE). The old v1 codebase
remains available on the [`legacy` branch](https://github.com/UKMysterrr/zgui2/tree/legacy).

## Build and run

The core requires CMake 3.14+ and a C++17 compiler. The example also needs SDL2
and OpenGL development packages. To build the core alone:

```sh
cmake -S . -B build/core -DZGUI_BUILD_BACKEND_SDL2_OPENGL=OFF
cmake --build build/core
```

To build the backend and the two-window interactive example:

```sh
cmake -S . -B build/example -DZGUI_BUILD_EXAMPLES=ON
cmake --build build/example
build/example/example/zgui_example
```

The primary window has Classic and Extended tabs. The inspector shows live
values, a theme switcher, a selectable table with draggable column boundaries,
and a canvas: drag to rotate, scroll to zoom, or
double-click to reset. Drag either title bar to focus and move its window;
drag the bottom-right corner to resize. Click the color swatch's Pick button
to open its HSV/alpha editor. The login form is a demonstration, not an
authentication system: the password is only masked on screen.

For bounded smoke runs, use `zgui_example --frames 60 --theme dark` (or
`--theme light`); the frame count must be positive. On a headless Linux host:

```sh
xvfb-run -a build/example/example/zgui_example --frames 60
```

The `debug`, `release`, `asan-ubsan`, `coverage`, and `no-exceptions`
configure/build presets are also available. Set `ZGUI_WARNINGS_AS_ERRORS=ON`
to enforce the project's warning flags.

## Using zgui with FetchContent

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
target_link_libraries(my_app PRIVATE zgui::sdl2_opengl)
```

For core-only use, set `ZGUI_BUILD_BACKEND_SDL2_OPENGL` to `OFF` and link
`zgui::zgui` instead. Include `<zgui.hh>` for the core or
`<zgui_impl_sdl2_opengl.hh>` for the backend. The backend target is available
when its SDL2 and OpenGL dependencies are found; requesting the example without
them fails configuration.

Alternatively, install with `cmake --install build/example --prefix /path/to/install`
and use `find_package(zgui CONFIG REQUIRED COMPONENTS sdl2_opengl)` with
`CMAKE_PREFIX_PATH` set to that prefix. Omit the component for core-only use.

See the [documentation](docs/README.md) for integration and widget details.

## Attribution and limitations

zgui is derived from [zxvnme/zgui 2.0](https://github.com/zxvnme/zgui) and is
licensed under the MIT License. The optional backend's embedded glyph atlas is
derived from Liberation Sans; see [FONT_LICENSE.txt](backends/sdl2_opengl/FONT_LICENSE.txt).
Text input supports printable ASCII only; the renderer also recognizes an
em dash, but otherwise falls back for non-ASCII bytes. The core has one active
context and is single-threaded. The renderer uses the OpenGL 2.1 fixed-function
pipeline and requires an active compatible context.
