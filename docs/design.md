# Design decisions

## Runtime model

zgui is single-threaded and single-context. Initialize its core before drawing or
updating widgets:

```cpp
void initialize_ui(std::shared_ptr<zgui::Graphics> graphics,
                   std::shared_ptr<zgui::Input> input) {
    zgui::core::initialize(std::move(graphics), std::move(input));
}
```

The `WindowManager` becomes the active manager when it draws or updates. Call
`zgui::core::shutdown()` before releasing the application context; this clears
the graphics, input, active manager, focused text input, text-truncation
collection, and restores the default theme. Multiple independent UIs and
thread-safe contexts are future work.

The active theme is a value copy. Use `core::set_theme(theme)` to make a theme
change visible on the next draw, or `core::reset_theme()` to restore
`core::default_theme`.

## Errors and value bindings

The core does not use C++ exceptions. Invalid pointer bindings assert in debug
builds and use a safe internal fallback in release builds. Invalid index setters
return `false` without changing the current selection; out-of-range table
mutators likewise report failure or do nothing. The optional SDL2/OpenGL backend
retains exceptions for backend initialization and rendering failures.

`WindowManager::validate_layout()` checks child bounds against each parent's
content rectangle, sibling overlap, and finite non-negative child sizes. It
returns `false` and a diagnostic string for the first invalid layout found.

Pointer and reference bindings remain non-owning: their values must outlive the
widget and any popup opened by it. To transfer shared ownership, create a
`Bound<T>` with `zgui::bound(initial_value)` and pass it to the widget
constructor or binding method. For example:

```cpp
#include "zgui.hh"

void add_owned_checkbox(zgui::Window& window) {
    auto enabled = zgui::bound(true);
    auto checkbox = std::make_shared<zgui::CheckBox>("Enabled", enabled);
    window.add_child(checkbox);
}
```

## Text

`TextInput` accepts printable ASCII bytes (`0x20` through `0x7e`), handles
backspace separately, and ignores other input bytes. The SDL2/OpenGL backend
also renders a UTF-8 em dash; other non-ASCII bytes map to `?` for rendering
and measurement. General UTF-8 input is not supported.

Future work: UTF-8 and multiple font sizes. Replacing the byte-oriented
`Input::pop_text_input(char&)` interface would be an API break. A possible future
signature is `bool pop_text_input(std::string& utf8_codepoint)`, with the output
containing one complete UTF-8 code point.

## Architecture and ownership

The public API is in [`src/zgui.hh`](../src/zgui.hh);
[`src/zgui.cc`](../src/zgui.cc) implements widgets and window management.
The optional SDL2/OpenGL backend adapts platform events and
OpenGL 2.1 drawing. Core code depends only on the C++ standard library.

`UIElement` stores raw child pointers for traversal and retains children added
with `shared_ptr`. Adding a raw pointer is explicitly non-owning; the caller
must ensure the child outlives its parent relationship. Removing a child or
destroying either side detaches the relationship. Draw, update, and input
dispatch use child snapshots, so callbacks may safely remove or reparent
shared-owned children during traversal. Windows and overlays are retained by
`WindowManager`. Popup widgets hold weak popup references so dismissing a
popup does not extend its lifetime.

## Frame loop

Initialize one graphics/input pair. Each frame, collect platform input, begin
the backend frame, update the manager, draw it, finish backend drawing, and
present. The manager sets the active manager while updating/drawing so popup
widgets can register overlays. `core::shutdown()` clears core references before
the graphics context is destroyed. No core calls should race across threads.

## Error handling and validation

The core does not throw exceptions. Invalid pointer bindings assert in debug
builds and use internal fallback storage in release builds. Invalid index
setters return `false` without changing the selection; invalid table cell
updates return `false`. The optional SDL2/OpenGL backend may throw for platform
initialization or rendering errors.

`WindowManager::validate_layout()` is a development/test diagnostic, not a
layout repair mechanism. It reports the first invalid child size, bound, or
overlap.

## Performance

Avoid rebuilding the widget tree on every frame; keep shared widgets and update
their bound values. `TableView` draws only visible rows, and clipping limits
child drawing/hit testing. Core geometry helpers use stack-backed vertex arrays,
and the SDL/OpenGL backend combines adjacent geometry with matching topology
into one batch. Reuse callbacks and avoid per-frame allocation in application
callbacks, especially viewport render/input handlers. The core currently has
no general-purpose virtualized list or frame allocator.

## Future work

- UTF-8 text and multiple font sizes, with an explicit input API migration.
- Multiple independent UI contexts and thread-safe rendering.
- DPI-aware spacing and focus/keyboard navigation.
- Clipboard hooks and a reusable nested `ScrollArea`.
- More backend adapters beyond SDL2/OpenGL.
