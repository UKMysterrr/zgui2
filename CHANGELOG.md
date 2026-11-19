# Changelog

Notable changes to zgui are documented here.

## [0.1.0] - Unreleased

### Added

- `Bound<T>`, explicit core/theme lifecycle helpers, and reference/owning bindings.
- `Separator`, `ScrollArea`, `ToggleButton`, `RadioGroup`, `ProgressBar`, and
  `Tooltip` widgets with headless regression coverage.
- CMake install/export support for the core library and its public header.
- Cross-platform CI, an out-of-tree package consumer, and a packed-source test build.
- Public version macros and a standard-library-only two-file packer.
- User guides, backend and widget documentation, and compiled C++ examples.
- `zgui::testing` target for reusable headless test support.
- `ZGUI_INSTALL` option to disable installation when included by a parent project.
- Public-API showcase with ten pages, a shared headless regression test, and
  optional frame-limit/per-frame callback support in the SDL runner.

### Changed

- The core and headless tests are built and tested with the project warning set in CI.
- The legacy `DRAW_UPDATE` macro now delegates to `draw_and_update`; its name and
  pointer-style call form remain available unless legacy macros are disabled.
- `draw_filled_rect` is retained as a deprecated alias of `Graphics::fill_rect`.

### Breaking changes

- The backend's model-loading API and helper vertex type were removed from
  `OpenGLGraphics`; its rendering implementation now lives in a separate module.
- The legacy render-test target and `ZGUI_BUILD_LEGACY_RENDER_TEST` option were
  removed. Migration: build `zgui_showcase` with `ZGUI_BUILD_EXAMPLES=ON`.
- Repository restructure: extras/source_model_viewer and the Nanohook example
  were moved out of this tree; `ZGUI_BUILD_EXTRAS` and
  `zgui_source_model_viewer` no longer exist. `ZGUI_BUILD_EXAMPLES` builds only
  `zgui_showcase`.

The public API snapshot also records additive helpers, bindings, layout/widgets,
runtime theme/core functions, and slider spacing constants; no other public
declarations were intentionally removed or renamed.
