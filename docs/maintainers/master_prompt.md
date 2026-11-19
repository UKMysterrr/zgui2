## 0. ROLE AND MISSION

You are a senior C++17 engineer taking over a small retained-mode GUI framework called **zgui** and bringing it to a polished, tested, documented, release-ready state. The framework's owner is building a separate project called **nanohook** on top of it, so zgui must end up generic, stable, and free of nanohook-specific code.

You will:

1. Audit the repo and record a baseline.
2. Clean up repo hygiene and licensing.
3. Separate nanohook-specific code from the framework.
4. Fix correctness issues in the core.
5. Build a headless test suite using a mock backend.
6. Add a proper build system, CI, and an optional single-file packer.
7. Write documentation.
8. Optionally add missing widgets.
9. Write a brand-new showcase render test from scratch using only the public API.
10. Verify everything and report.

Prefer small, reviewable commits, one logical change each. After every phase, stop, run the verification commands listed for that phase, and print a short report (see section 12) before continuing.

---

## 1. PROJECT BACKGROUND (READ CAREFULLY)

### 1.1 Lineage and license

- zgui is a derivative of **zxvnme/zgui 2.0** ([https://github.com/zxvnme/zgui](https://github.com/zxvnme/zgui), MIT, "Copyright (c) 2019, zxvnme"). The upstream 2.0 branch was a tiny prototype (about 700 lines, version string `2.0.0 P-0.13`, type `PROTOTYPE`). This repo grew that skeleton into a full toolkit.
- The inherited skeleton includes: `Color`, `Point`, `Vertex`, `VertexTopology`, the `Graphics` base class (`append_geometry`, `set_topology`, `draw_line`, `fill_rect`, pure `draw()`/`reset()`), `UIElement` (`children_`, `parent_`, `position_`, `size_`, virtual `draw()`/`update()`), the `zgui::core` globals, and the `DRAW_UPDATE` macro.
- MIT requires the copyright and permission notice to stay in all copies or substantial portions. The current `zgui.hh` and `zgui.cc` carry no notice. **Fixing this is mandatory** (Phase 1).
- The embedded 12px monochrome glyph atlas in the SDL2/OpenGL backend was rasterized from **Liberation Sans**. A `FONT_LICENSE.txt` already exists. Keep it, and reference it from a top-level `THIRD_PARTY_NOTICES.md`.

### 1.2 Current repository layout (as received)

```
zgui.hh                         ~854 lines   public header (single header for the whole toolkit)
zgui.cc                         ~2690 lines  implementation (single translation unit)
backends/sdl2_opengl/
    zgui_impl_sdl2_opengl.hh    ~145 lines
    zgui_impl_sdl2_opengl.cc    ~1468 lines
    FONT_LICENSE.txt
tests/
    render_test.cc              ~2681 lines  (nanohook-flavoured demo; to be replaced, see Phase 10)
    layer_smoke_tests.hh/.cc    ~116 lines
    render_test                 prebuilt ELF binary (must not be in the repo)
    assets/                     Valve Source engine models/textures (copyrighted; must not be in the repo)
```

No CMake, no README, no CI, no `.gitignore` exist for this tree.

### 1.3 Architecture summary of what exists today

Namespace `zgui`. Everything is retained mode: a tree of `UIElement` objects, owned via `std::shared_ptr` (`owned_children_`) with raw `children_` pointers for traversal.

- **Primitives:** `Color`, `Point`, `Rect`, `Vertex`, `VertexTopology`, `Theme` (window_bg, window_header, groupbox_bg, border_outer, border_inner, accent, text_primary, text_dim, control_bg, control_hover).
- **Backend interfaces:**

- `Graphics` (abstract): `draw()`, `flush()`, `reset()`, `set_clip_rect`, `reset_clip_rect`, `draw_text`, `measure_text_width`, `viewport_size`, `fit_text`, plus non-virtual helpers `push_clip_rect`, `pop_clip_rect`, `draw_line`, `draw_rect`, `fill_rect`, `draw_gradient_rect`, `draw_frame`.
- `Input` (abstract): `key_pressed`, `mouse_over`, `mouse_position`, `mouse_button_pressed`, `mouse_wheel_delta`, `pop_text_input(char&)`, `poll_pressed_key`, `key_name`, plus defaulted extras (cursor keys, delete, `mouse_button_down(MouseButton)`, `mouse_delta`, `mouse_double_clicked`, `escape_pressed`).
- **Globals:** `zgui::core::graphics`, `core::input`, `core::window_manager` (raw pointer), `core::focused_text_input` (raw pointer), `core::default_theme`, `core::active_theme` (reference), `core::collect_text_truncations`, `core::text_truncations`.
- **Widgets:** `Label`, `Button`, `CheckBox`, `Slider<T>` (arithmetic template), `ComboBox`, `MultiComboBox`, `TextInput`, `KeyBind`, `ColorPicker`, `ColorSwatch`, `ColorButton`, `TabBar`, `SubTabBar`, `GroupBox`, `TableView` (alias `ListView`), `Viewport`, `Window`.
- **Layout and management:** anchors and fractional constraints on `UIElement`, `StackLayout` (including `StackLayout::page`, `add_two_column_row`, `add_three_column_row`), `WindowManager` (windows, overlays, z-order, `validate_layout`), `spacing::` constants.
- **Backend:** `zgui::sdl2_opengl` with `OpenGLGraphics` (GL 2.1 fixed-function, batches by topology), `SDL2Input`, and `run_sdl2_opengl_application(title, w, h, setup)`.

### 1.4 Baseline facts already measured

- `g++ -std=c++17 -Wall -Wextra -c zgui.cc` produces **zero warnings**.
- `-Wshadow` yields about 9 hits: constructor parameters of `Color`, `Point`, `Vertex` shadow their members.
- `-Wconversion` yields 1 hit (`int` to `float`).
- The backend and the existing tests were **not** built or run by the reviewer (no SDL2 in that environment). Treat their correctness as unknown until you build them.

---

## 2. GLOBAL RULES (APPLY TO EVERY PHASE)

1. **Honesty over speed.** If a tool, library, or display is missing (for example SDL2 dev headers, a GPU, an X server), say so explicitly. Do not claim something was built, run, or verified when it was not. Mark such items `NOT VERIFIED` in your reports.
2. **Do not invent APIs.** Before changing a symbol, search the repo for every use of it. Before adding one, check it does not already exist.
3. **Public API stability.** Snapshot the public API in Phase 0. Any rename, removal, or signature change must be listed in a `CHANGES.md` section called "Breaking changes", with a one-line migration note. Prefer adding overloads or deprecated aliases (`[[deprecated("use X")]]`) over breaking changes, unless this prompt explicitly says to remove something.
4. **C++17 only.** No C++20 features. No new third-party dependencies in the core library. The core must stay dependency-free (standard library only).
5. **Keep the framework backend-agnostic.** Nothing in `zgui.hh`/`zgui.cc` may include SDL, OpenGL, Windows, or Source-engine headers.
6. **No copyrighted third-party assets in the repo.** Do not add, keep, or re-commit Valve/Source models or textures.
7. **Warnings are bugs.** Final state must compile warning-free with GCC and Clang using the flag set in section 8.2.
8. **Every bug fix gets a regression test** (Phase 5 infrastructure). If a fix cannot be tested headlessly, explain why in the commit message.
9. **No behavior changes hidden in refactors.** Keep refactor commits and behavior-change commits separate.
10. **Comments explain why, not what.** Keep header comments short. Public symbols get a one-to-three line doc comment.
11. **Do not reformat the whole codebase in one commit** before the `.clang-format` commit; after it, one dedicated commit may apply formatting, and nothing else may be mixed into that commit.
12. If this prompt and the code disagree (for example a symbol named here does not exist), trust the code, note the discrepancy in your report, and continue.

---

## 3. PHASE 0 — RECON AND BASELINE

Goal: understand the repo and record numbers to compare against at the end.

Tasks:

1. Create a branch `polish/v1`.
2. Print the tree and line counts (`wc -l`) for all source files, excluding `tests/assets`.
3. Build `zgui.cc` alone with `-std=c++17 -Wall -Wextra -Wpedantic -Wshadow -Wconversion` and save the warning list to `docs/baseline_warnings.txt` (this file is deleted at the end of the project; it exists to prove the warning count went to zero).
4. Try to build the SDL2/OpenGL backend and the existing render test. If SDL2/GL dev packages are missing, record exactly what is missing and continue; do not stall.
5. Generate a public API inventory: every symbol declared in `zgui.hh` and the backend header, one per line, into `docs/api_snapshot_before.txt` (class, method, free function, constant, macro). Use a script, not manual copy, and commit the script as `tools/dump_api.py` (a regex or ctags-based approach is fine; document its limits).
6. Search for and list: every raw-pointer binding to user data (`bool*`, `int*`, `Color*`, `std::string*`, `std::size_t*`, `std::vector<bool>*`, `float*`), every `throw`, every `static` or `inline` global, every `new`/`delete`, every `reinterpret_cast`/`const_cast`, every `TODO`/`FIXME`/`HACK`. Save to `docs/audit_notes.txt`.

Verification: the files above exist; the report states which builds succeeded.

---

## 4. PHASE 1 — REPO HYGIENE AND LICENSING

### 4.1 Remove things that must not be in the repo

- `git rm` the prebuilt binary `tests/render_test`.
- `git rm -r` `tests/assets/` (Valve Source models and textures: `*.mdl`, `*.vvd`, `*.vtx`, `*.phy`, `*.vtf`, `*.vmt`).
- If the repo already has history containing them, do **not** rewrite history yourself. Instead add a note to the final report recommending `git filter-repo` and listing the paths, so the owner can decide.
- Add a `.gitignore` covering: build directories (`build/`, `out/`, `cmake-build-*`), IDE folders (`.vs/`, `.idea/`, `.vscode/` except shared settings), binaries and objects (`*.o`, `*.obj`, `*.a`, `*.so`, `*.exe`, `*.pdb`), `compile_commands.json`, and `tests/assets/` (so assets cannot be recommitted by accident).

### 4.2 Licensing (mandatory)

1. Create `LICENSE` containing the full MIT text with **two** copyright lines:

```
Copyright (c) 2019, zxvnme
Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>   <-- leave this placeholder for the owner to fill in; do not guess a name
```

1. Add the same two-line copyright plus an SPDX tag (`// SPDX-License-Identifier: MIT`) as a header comment at the top of **every** source file in this repo, including `zgui.hh`, `zgui.cc`, backend files, tests, tools. Keep the existing "zgui by zxvnme and all the community contributors" attribution line if present.
2. Create `THIRD_PARTY_NOTICES.md` listing: upstream zgui (MIT, zxvnme), Liberation Sans (note: the embedded glyph atlas is derived from it; point to `backends/sdl2_opengl/FONT_LICENSE.txt`), and any other third-party content you find. Do not claim a license you have not read; read `FONT_LICENSE.txt` and quote its license name exactly as it states it.
3. Add a `NOTICE` paragraph to the README (Phase 8) stating the derivation from zxvnme/zgui.

### 4.3 Editor and formatting config

- Add `.editorconfig` (UTF-8, LF, final newline, 2-space or 4-space indentation to match the dominant style you measure in the code; report which you chose).
- Add `.clang-format` matching the existing dominant style. Do **not** apply it repo-wide yet.
- After the above is committed, make **one** commit named `style: apply clang-format` that only reformats, then run the build to confirm no behavior change.

Verification: `git ls-files | grep -E '\.(mdl|vvd|vtx|vtf|vmt|phy)$'` prints nothing; `git ls-files tests/render_test` prints nothing; `grep -L "SPDX-License-Identifier" $(git ls-files '*.cc' '*.hh' '*.py')` prints nothing.

---

## 5. PHASE 2 — SEPARATE NANOHOOK-SPECIFIC CODE FROM THE FRAMEWORK

Problem: `OpenGLGraphics` in `backends/sdl2_opengl/` contains Source-engine model loading and rendering. That is the owner's use case, not part of a GUI framework.

### 5.1 Identify the nanohook-specific surface

In `zgui_impl_sdl2_opengl.hh/.cc`, find and list all of the following (names as of the review; verify):

- Members: `source_model_vertices_`, `weapon_model_vertices_`, `source_model_texture_`, `weapon_model_texture_`, `source_model_height_`, `source_model_center_{x,y,z}_`, `weapon_model_height_`, `weapon_model_center_{x,y,z}_`, `struct SourceModelVertex`.
- Methods: `load_source_model`, `load_source_weapon_model`, `source_model_loaded`, `weapon_model_loaded`, `draw_source_model`, `draw_source_model_view`, `draw_source_weapon_model`, `project_source_model_bounds`, private `draw_model`, and the MDL/VVD/VTX/VTF parsing helpers in the `.cc`.
- Any other function in the `.cc` whose only purpose is parsing or rendering those formats.

### 5.2 Move, don't delete

1. Create `extras/source_model_viewer/` containing the parsing and rendering code, as a standalone module with its own header and source, depending on OpenGL only (not on zgui internals except `Point`/`Color` if needed).
2. The module exposes a small, clean interface, for example:

cpp

```
class SourceModelRenderer {
public:
    bool load(const std::string& mdl, const std::string& vvd,
              const std::string& vtx, const std::string& vtf);
    bool loaded() const;
    void draw(int x, int y, int w, int h, float yaw, float pitch,
              float zoom, float pan_x, float pan_y, zgui::Color tint);
    std::array<float,4> project_bounds(int w, int h, float yaw, float pitch,
              float zoom, float pan_x, float pan_y) const;
    void release();
};
```

1. It is used from a `zgui::Viewport` via `Viewport::set_on_render(...)`, which already gives the callback the on-screen rectangle. Demonstrate that in the extras README.
2. Build it only when CMake option `ZGUI_BUILD_EXTRAS=ON` (default OFF). It must not be linked into the core or the backend.
3. Add `extras/source_model_viewer/README.md` stating: "This module is provided as-is for the nanohook project. It does not ship any Valve assets. Supply your own model files at runtime." Do not bundle any assets.
4. After the move, `OpenGLGraphics` must contain **only**: batching, clip rects, text, the primitive helpers, viewport handling, and the font atlas. Remove `SourceModelVertex`, the model textures, and the model members from it.
5. The owner will take `extras/source_model_viewer/` into nanohook later; keep it self-contained so it can be moved with `git mv` and no edits.

### 5.3 Fix API hiding in `OpenGLGraphics` (verify first)

The backend header declares non-virtual `draw_filled_rect`, `draw_rect`, and `draw_line`. The base `Graphics` also has non-virtual `draw_line` and `draw_rect`. A derived declaration with the same name hides the base one, so a call through `Graphics&` (which is what `zgui::core::graphics` yields) and a call through `OpenGLGraphics&` may execute different code.

Tasks:

1. Verify by reading both implementations and, once the mock backend exists (Phase 5), by a test that draws the same line through both references and compares recorded output.
2. Resolve by making **one** code path. Preferred: delete the derived duplicates and keep only the base class helpers (`fill_rect`, `draw_rect`, `draw_line`) that go through the virtual `append_geometry`/`set_topology`. If `draw_filled_rect` is used by other code, keep it in the base class as an alias of `fill_rect` marked `[[deprecated]]`.
3. If the derived versions exist for performance reasons, make the base methods `virtual` and `override` them, and document it.

### 5.4 Fix topology handling

- `Graphics::vertex_topology_` is uninitialized in the base class. Initialize it.
- The backend keeps its own `pending_topology_` and `batches_` (batches by topology). Confirm that mixing lines and triangles in one frame renders in the correct order; write a mock-backend test for ordering (Phase 5).
- Fix the SDL example behavior if any path draws everything as triangles or indexes an empty vector.

Verification: `grep -R "source_model\|weapon_model\|SourceModel" backends/ zgui.hh zgui.cc` prints nothing; the extras directory builds with `-DZGUI_BUILD_EXTRAS=ON` (or `NOT VERIFIED` with the reason).

---

## 6. PHASE 3 — CORE CORRECTNESS FIXES

Work through each item, one commit each, each with a test (Phase 5; if the test harness does not exist yet, add the test as soon as it does and reference the commit).

### 6.1 Initialization

- `Color`, `Point`, `Vertex`, and `Rect`: give all members default member initializers (`unsigned char r = 0, g = 0, b = 0, a = 255` is acceptable; choose and document). Remove the need for `explicit` constructors that only assign.
- Rename constructor parameters to remove all `-Wshadow` hits (for example `r_`/`g_`/`b_`/`a_` or use member-initializer lists with differently named parameters). Prefer member-initializer lists.
- `Graphics::vertex_topology_` initialize to `VertexTopology::TRIANGLE_LIST`.
- Audit every class for uninitialized members (use `-Wuninitialized`, `-Weffc++` for a one-time scan, and read each class). Fix all.
- Fix the single `-Wconversion` hit with an explicit `static_cast`.

### 6.2 Polymorphism and ownership

- `Graphics` and `Input` already have virtual destructors; confirm. Make `UIElement`'s destructor handle detaching from parents safely: a child destroyed while still referenced by a parent's raw `children_` vector must never leave a dangling pointer. Define the rule: either `children_` is always derived from `owned_children_` plus explicit non-owning adds, or removal updates both. Add `remove_child(UIElement*)` and `clear_children()` if missing, and make destructors of widgets that register with globals (for example `TextInput` clearing `core::focused_text_input`) correct in all orders, including when the `WindowManager` is destroyed before or after its windows.
- `ComboBox`/`MultiComboBox`/`ColorSwatch`/`ColorButton` hold `std::weak_ptr` popups and a `WindowManager*`. Verify there is no use-after-free if the manager dies while a popup is open, or the widget dies while the popup is open. Test both orders under ASan.
- `WindowManager` holds `overlays_`. Confirm `dismiss_overlay` and `bring_to_front` are safe when called during `draw()` or `update()` iteration (iterator invalidation). If not, defer mutations until the end of the frame.

### 6.3 Consistent error handling

- `Slider` throws `std::invalid_argument` on a null value pointer; other widgets accept null and may dereference it. Choose **one** policy for the whole library and apply it everywhere. Default recommendation: no exceptions in the core (many game-embedding contexts disable them); instead take a reference or validate in the constructor and fall back to an internal dummy value, with an `assert` in debug builds. Document the policy in `docs/design.md`.
- Check all `std::size_t` index setters (`ComboBox::set_selected_index`, tab selection, table selection) for out-of-range behavior, including empty containers.

### 6.4 Layout and rendering

- `Slider` uses hardcoded geometry (a 17px track offset, 5px thumb, 11px thumb height, 4px track). Move these into `spacing::` constants (or `Theme` metrics) so they scale and are consistent.
- Verify clip rect handling: `push_clip_rect`/`pop_clip_rect` must balance across `draw()` even when widgets early-return. Add a debug assertion at `Graphics::reset()` or `end of frame` that the clip stack is empty, and tests for nested clips with negative sizes, zero sizes, and clips fully outside the parent.
- `fit_text` and `core::collect_text_truncations`: confirm the global vector cannot grow unbounded when collection is left on. Cap it or clear it per frame, and document.
- Check text truncation for strings shorter than the ellipsis width and for width <= 0.
- Check `TableView`: resizing columns with zero or negative widths, zero rows, zero columns, `selected_row_` after `clear_rows()`, `set_cell` out-of-range, scroll clamping when rows shrink.
- Check `TabBar` with zero tabs, `set_active_tab` out-of-range, `page_scroll_offset_` after tab switch.
- Check `ColorPicker` HSV round-trip: set a color, convert to HSV, back to RGB, and assert the color is stable within ±1 per channel for a grid of inputs, including pure grey, pure primaries, and alpha 0 and 255. Fix drift if found.

### 6.5 API tidying (non-breaking where possible)

- `DRAW_UPDATE` macro: replace with

cpp

```
inline void draw_and_update(UIElement& e) { e.draw(); e.update(); }
template <class P> void draw_and_update(const P& e) { draw_and_update(*e); }
```

Keep the macro, defined in terms of the function, behind `#ifndef ZGUI_NO_LEGACY_MACROS` and mark it deprecated in a comment (macros cannot carry `[[deprecated]]`).

- `spacing::` duplicates: `kCheckboxSize` vs `checkbox_size`, `kGroupTitleHeight` vs `group_title_height`, `kTabLabelHorizontalPadding` vs `tab_label_horizontal_padding`, `kGroupInnerPadding` vs `group_inner_padding`, `kGroupSubTabTop` vs `group_subtab_top`, `kCheckboxLabelGap` vs `checkbox_label_gap`. Keep the `k`-prefixed names as canonical. Keep the snake_case ones only as `[[deprecated]]` inline aliases for one release, then list them under "Removed in 2.x" in `CHANGES.md`. Update all internal uses to the canonical names.
- Remove unused includes from `zgui.hh` (check `<iomanip>`, `<sstream>`, `<iterator>`, `<array>` against actual use in the header; keep ones needed by templates such as `Slider<T>`). Move implementation-only includes to `zgui.cc`.
- Make internal helpers in `zgui.cc` `static` or put them in an anonymous namespace. No non-API symbol may leak.

Verification: compile with the full flag set in section 8.2 on GCC and Clang: zero warnings. `docs/baseline_warnings.txt` can now be deleted.

---

## 7. PHASE 4 — TEXT, INPUT, AND CONTEXT DECISIONS

These are decisions the owner has not made. Implement the **default** below, document the alternative in `docs/design.md`, and mark each as `DECISION: default taken` in your report so the owner can overrule.

### 7.1 Text encoding — default: ASCII-only, explicit and safe

- `Input::pop_text_input(char&)` and the 12px atlas imply ASCII. Make that explicit: document "zgui text is ASCII (0x20–0x7E); other bytes render as `?`" and implement the fallback in `draw_text`, `measure_text_width`, and `TextInput` input filtering so that no out-of-range glyph index is ever read (verify the atlas lookup with a bounds check). Add tests with bytes 0x00, 0x1F, 0x7F, 0x80, 0xFF and a UTF-8 multi-byte string.
- Do not attempt a UTF-8 rewrite in this phase. Add a "Future work: UTF-8 and multiple font sizes" section to `docs/design.md` noting that changing `pop_text_input` later is an interface break, and propose the replacement signature (`bool pop_text_input(std::string& utf8_codepoint)`) as a design sketch only.

### 7.2 Context and globals — default: keep globals, make them safe

- Keep `zgui::core::` globals, but add:

- `core::initialize(std::shared_ptr<Graphics>, std::shared_ptr<Input>)` and `core::shutdown()` that null everything and reset focus/theme.
- Debug assertions at the top of `UIElement::draw/update` paths that `core::graphics` and `core::input` are non-null, with a clear message.
- Document that zgui is single-threaded and single-context. List "Context object for multiple UIs" as future work.
- Make `core::active_theme` safely switchable at runtime (it is currently a reference initialized to the default theme). Provide `core::set_theme(const Theme&)` and `core::reset_theme()` and test that a theme change is visible on the next `draw()`.

### 7.3 Widget value binding — default: keep raw pointers, document and harden

- Keep the pointer-binding API (it is the established style and fits hooking/overlay use), but:

- Document in each widget's header comment that the bound value must outlive the widget.
- Add reference-taking constructor overloads where missing (`CheckBox(label, bool&)`, `Slider<T>(title, T&, ...)`, `KeyBind(title, int&)`, `ColorPicker(Color&)` etc.), delegating to the pointer constructors; keep `TextInput(std::string&)` as is.
- Add an owning variant helper: a tiny `Bound<T>` holder (`std::shared_ptr<T>`) that widgets can take, so beginners can write `auto v = zgui::bound(true);`.

Verification: new tests for each item pass under ASan and UBSan.

---

## 8. PHASE 5 — HEADLESS TEST SUITE WITH A MOCK BACKEND

This is the most valuable phase. All tests run with no window, no GPU, no SDL.

### 8.1 Layout

```
tests/
  CMakeLists.txt
  support/
    mock_graphics.hh/.cc    // records draw calls
    mock_input.hh           // scripted input
    test_harness.hh         // tiny header-only runner (no dependencies)
  unit/
    test_geometry.cc
    test_graphics_base.cc
    test_theme_and_core.cc
    test_widgets_*.cc
    test_layout.cc
    test_window_manager.cc
    test_text.cc
    test_lifetime.cc
    test_table_view.cc
    test_color_picker.cc
  fuzz/
    random_event_sequence.cc
```

Do not add doctest/Catch2/GoogleTest unless the owner asks; keep the repo dependency-free with a ~100-line harness providing `TEST(name)`, `CHECK`, `CHECK_EQ`, `CHECK_NEAR`, a registry, a summary, and a non-zero exit code on failure. Register tests with CTest via one executable and `ctest --output-on-failure`.

### 8.2 Compiler flags (use for tests and library)

GCC/Clang: `-std=c++17 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wold-style-cast -Wnon-virtual-dtor -Woverloaded-virtual -Wnull-dereference -Wformat=2`. For `-Wold-style-cast` and `-Wsign-conversion`, fix legitimate hits; suppress with a justified `static_cast` rather than disabling the warning.
MSVC: `/W4 /permissive- /utf-8`.

Sanitizer matrix (test target only): `-fsanitize=address,undefined -fno-omit-frame-pointer` as one configuration, and a plain Release configuration with `-O2 -DNDEBUG` (assertions off) as another. Optionally a TSan config is not required (single-threaded library).

### 8.3 Mock backend spec

`MockGraphics : zgui::Graphics`

- Records a flat list of commands: `FillRect{pos,size,color}`, `Line{a,b,color}`, `Text{string,pos,color}`, `SetClip{x,y,w,h}`, `ResetClip`, `Draw`, `Reset`, in call order.
- `measure_text_width` returns `text.size() * kCharWidth` (configurable) so truncation tests are deterministic.
- `viewport_size()` configurable.
- Helper queries: `texts()`, `count_fill_rects()`, `commands_between_clips()`, `last_clip()`, `clip_depth()`.

`MockInput : zgui::Input`

- Scriptable: `set_mouse(Point)`, `press(MouseButton)`, `release(MouseButton)`, `scroll(int)`, `type(std::string)`, `press_key(int)`, `set_key_down(int,bool)`, `double_click()`, `escape()`, and `end_frame()` which clears one-shot flags.
- Implements every virtual, including text-cursor keys and `poll_pressed_key`.

A `Harness` helper builds a `WindowManager`, one `Window`, and runs `frame()` = `input.begin_frame; manager.update(); manager.draw(); graphics.flush(); input.end_frame()` in the same order the real SDL loop uses (read `run_sdl2_opengl_application` and copy its exact order; if the loop order differs between `draw` and `update`, record that in `docs/design.md` and make the harness match).

### 8.4 Required test coverage (minimum list)

Geometry and primitives

- `Rect::contains` boundaries (inclusive left/top, exclusive right/bottom), negative sizes.
- `Color`/`Point` default values after Phase 3.

Graphics base class

- `fill_rect` emits 6 vertices, correct winding and positions; `draw_line` emits 2 vertices with `LINE_LIST`; `draw_rect` emits 4 lines (or the documented equivalent); `draw_gradient_rect` corner colors; `draw_frame` layering.
- Clip stack: push/pop balance; nested intersection; zero/negative size; fully outside parent yields zero-area; pop on empty is a no-op.
- Topology: mixing lines and triangles preserves order in the mock and in the real backend's batch logic (if the batch logic is testable without GL, factor it out into a pure class `BatchBuilder` and test that).
- Hiding bug test from 5.3: draw the same line through `Graphics&` and the concrete type; assert identical recorded geometry.

Widgets (each: initial draw, hover, press, release, callback firing exactly once, value change, bound-value update from outside)

- `Label`: alignment left/center/right positions with the mock measure; color override and clear; truncation.
- `Button`: click inside fires; press inside, release outside does not fire; disabled/hidden if supported; callback replaced at runtime.
- `CheckBox`: toggles; `on_change` once per click; external value change reflected; null handling per Phase 3 policy.
- `Slider<int>` and `Slider<float>` and `Slider<unsigned char>` and `Slider<double>`: clamp at both ends; min > max swapped; `min == max` no division by zero; integer rounding; drag outside the widget keeps tracking until release; `on_change` not called when value unchanged; value text formatting; narrow widths truncate title/value without crashing; extreme ranges (±1e9, ±FLT_MAX/2) produce finite ratios.
- `ComboBox`: open/close; selecting item fires `on_change` with the right index; empty items; selected index out of range; `bind_selected_index` sync both ways; popup opens within viewport bounds (flips up if near the bottom); popup dismissed on outside click and on `escape`; manager destroyed while open (ASan clean).
- `MultiComboBox`: toggling items; `vector<bool>` resized when items change; summary text truncation.
- `TextInput`: typing appends at cursor; backspace/delete; cursor left/right/home/end; focus on click, blur on click elsewhere; only one focused input at a time (`core::focused_text_input`); `on_change` per edit; non-ASCII filtering from 7.1; destroying the focused input clears the global pointer.
- `KeyBind`: click enters listening; next key press stored; `escape` cancels; `key_name` shown; ignores the mouse button that started listening.
- `ColorPicker`/`ColorSwatch`/`ColorButton`: SV square, hue bar, alpha bar drag math at the edges; HSV round-trip stability from 6.4; popup lifetime.
- `TabBar`/`SubTabBar`: add tabs; switch by click; label shrinking before truncation (verify `kTabLabelMinimumPadding` behavior); zero tabs; many tabs in narrow width; content clip equals the page rect; per-tab scroll offset behavior; `widget_count` and `visible_widget_count`.
- `GroupBox`: fit-to-content; sub-tab strip placement uses the documented spacing; `content_right`.
- `TableView`: set columns; add rows; cell color; row backgrounds; zebra; selection callback; keyboard/mouse selection; scrollbar drag; wheel scrolling clamps; column resize by drag; column visibility toggling; weight columns distribute remaining width; clear rows resets selection; huge row count performance sanity (10,000 rows draw only visible ones; assert the number of recorded text commands is bounded by visible rows × visible columns).
- `Viewport`: `on_render` called with the correct screen rect and clipped rect; `on_input` receives mouse delta, wheel, rotating (left drag), panning (middle/right drag), double-click flag; pointer capture only when the press started inside.
- `Window`: drag by title bar; resize by corner/edge with min size; `set_minimum_size_from_content`; frameless and title-bar-hidden modes; `set_visible(false)` skips draw and input; always-on-top ordering; status bar checkbox and indicators.

Layout

- `StackLayout`: cursor advance with rows and gaps; `page()` spacing constants; two- and three-column rows; stacking inside `GroupBox` with and without sub-tabs.
- Anchors and constraints under parent resize: left/right/top/bottom anchoring and fractional constraints produce the exact expected rectangles after `set_size` on the parent; no accumulating drift after 1,000 random resizes (assert exact equality to a freshly computed layout).
- `WindowManager::validate_layout`: returns true for valid layouts; false and a useful message for overlap, out-of-bounds, and negative sizes; tests for each failure message.

Window manager

- Z-order: click brings a window to front; always-on-top stays above; overlay (popup) receives input first and swallows clicks; closing an overlay on outside click does not also click the widget underneath unless documented otherwise.
- Mutation during iteration: calling `remove_window`/`dismiss_overlay`/`bring_to_front` from inside a widget callback must not crash (ASan).

Text

- `fit_text`: exact ellipsis behavior at width 0, below ellipsis width, exactly fitting, one pixel short; `core::collect_text_truncations` records only truncated strings and respects the cap from 6.4.
- ASCII fallback from 7.1.

Lifetime and robustness

- Destroy in all orders: manager before windows, windows before manager, child before parent, parent before child (via shared_ptr), popup widget before popup, bound value destroyed after widget (document this one is the caller's responsibility; test the documented safe order).
- Random event-sequence fuzz test (`tests/fuzz/random_event_sequence.cc`): fixed seed(s) printed on failure; generate 100,000 frames of random mouse moves, presses, releases, wheel, typing, key presses, resizes and window drags against a "kitchen sink" UI that contains every widget. Assert no crash, no ASan/UBSan findings, clip depth is 0 at the end of every frame, no NaN/Inf in any recorded coordinate, and all recorded rectangles have non-negative size.

### 8.5 Coverage

Add a `coverage` CMake preset (gcov/llvm-cov). Report line coverage for `zgui.cc`. Target at least 85% lines. List uncovered regions in the report; do not pad tests with meaningless assertions to inflate the number.

Verification: `ctest --output-on-failure` passes in both the sanitizer and the release configurations; fuzz test runs under a time limit (under 60 s).

---

## 9. PHASE 6 — BUILD SYSTEM, CI, PACKING

### 9.1 CMake (3.14+)

Top-level `CMakeLists.txt` with these targets and options:

- `zgui` — static library (option `ZGUI_BUILD_SHARED` default OFF). Sources: `zgui.cc`. Public include dir: repo root. Alias `zgui::zgui`. Compile features `cxx_std_17`. No exceptions required (`-fno-exceptions` must compile if you chose the no-throw policy; add a CI job to prove it, with `-fno-rtti` also if achievable; if not, document why).
- `zgui_sdl2_opengl` — backend library, option `ZGUI_BUILD_BACKEND_SDL2_OPENGL` default ON when SDL2 and OpenGL are found (`find_package(SDL2 CONFIG)` then fallback to `pkg-config`; `find_package(OpenGL)`). Links `zgui::zgui`. Alias `zgui::sdl2_opengl`.
- `zgui_tests` — option `ZGUI_BUILD_TESTS` default ON when top-level, OFF when consumed via `add_subdirectory`.
- `zgui_showcase` — the new demo from Phase 10, option `ZGUI_BUILD_EXAMPLES`.
- `ZGUI_BUILD_EXTRAS` — Phase 2 module, default OFF.
- Export and install rules: headers, library, `zguiConfig.cmake`, so `find_package(zgui)` works. Verify with a tiny out-of-tree consumer project in `tests/install_consumer/` built by CI.
- Presets (`CMakePresets.json`): `debug`, `release`, `asan-ubsan`, `coverage`, `no-exceptions`.
- Warnings as errors only for the project's own targets (`ZGUI_WARNINGS_AS_ERRORS` default ON in CI, OFF otherwise).

### 9.2 Continuous integration (GitHub Actions)

`.github/workflows/ci.yml`:

- Matrix: Ubuntu (GCC, Clang), Windows (MSVC), macOS (Clang) for the core library and the headless tests.
- Separate job: Ubuntu with `libsdl2-dev` and `xvfb` that builds the backend and runs a short headless smoke run of the showcase (render N frames then exit, via a command-line flag in the showcase such as `--frames 60`; if GL under xvfb is unreliable, make the job build-only and say so).
- Jobs: format check (`clang-format --dry-run --Werror`), sanitizer preset, coverage preset (upload artifact), no-exceptions build, install-consumer build.
- Cache nothing fancy; keep it simple and readable.

### 9.3 Single-file packer (optional but recommended)

Upstream's identity was "two files you drop into a project". Keep that option:

- Add `tools/pack.py` (Python 3.8+, standard library only) that emits `dist/zgui.hh` and `dist/zgui.cc` by concatenating the sources in dependency order, stripping duplicate include guards, preserving license headers, and writing a header comment with the git describe/version.
- Decision default: **keep `zgui.hh` + `zgui.cc` as the source of truth, do not split into multiple files in this pass.** If during the work `zgui.cc` is clearly too large to maintain, propose a split plan (core, widgets, layout, window, table, color) in `docs/design.md` and ask the owner; do not split unprompted. If the owner later says "split", do it as a mechanical move commit followed by the packer.
- CI check: building from `dist/` must produce the same test results as building from sources.

### 9.4 Versioning

- Add `zgui/version.hh` (or inside `zgui.hh`): `ZGUI_VERSION_MAJOR/MINOR/PATCH`, `ZGUI_VERSION_STRING`. Start at `0.1.0` unless the owner specifies otherwise. Add `CHANGELOG.md` (Keep a Changelog format).

Verification: fresh clone, `cmake --preset release && cmake --build --preset release && ctest --preset release` succeeds; the install-consumer project builds against the installed package.

---

## 10. PHASE 7 — DOCUMENTATION

Create these files. Keep prose short and concrete; every code sample must compile (CI compiles code blocks extracted from the docs, or the samples live in `examples/snippets/` and the docs include them).

### 10.1 `README.md`

- One-paragraph pitch: retained-mode, dependency-free, C++17, backend-agnostic GUI toolkit for overlays and tools.
- Screenshot or GIF of the showcase (placeholder path `docs/img/showcase.png` if you cannot render one; say so).
- Feature list (accurate to the code).
- A 25-line "hello world" using the SDL2/OpenGL backend and `run_sdl2_opengl_application`.
- Build instructions with the presets.
- Link to docs.
- Attribution paragraph: derived from zxvnme/zgui 2.0 (MIT), Liberation Sans-derived glyph atlas.
- Known limitations (ASCII text, single context, GL 2.1 fixed-function backend).

### 10.2 `docs/getting_started.md`

Embed in an existing app: initialize `core`, create `WindowManager`, build a window with `TabBar`, `GroupBox`, `StackLayout`; per-frame calls; handling resize; changing the theme.

### 10.3 `docs/writing_a_backend.md`

The contract of `Graphics` and `Input`, precisely: which virtuals are mandatory, call order per frame, coordinate system (origin top-left, y down, pixels), whether coordinates are float, clip rect semantics, how text metrics must be consistent between `measure_text_width` and `draw_text`, how `append_geometry` and topologies work, what `flush()` must do, expectations for `Input` one-shot versus held states (`mouse_button_pressed` versus `mouse_button_down`), when `begin_frame` clears state. Include a **minimal complete backend skeleton** (about 60 lines) and the notes needed for Direct3D 9/11, Vulkan, and OpenGL overlays (state save/restore before and after, since overlay frameworks hook inside a host application's render loop; mention `begin_frame`/`end_frame` state backup as the SDL2/OpenGL backend does with matrix mode and program).

### 10.4 `docs/widgets.md`

One section per widget: purpose, constructor(s), callbacks, binding rules, a 5–10 line example, layout notes (row heights, recommended `spacing::` constants), and edge-case behavior verified by tests.

### 10.5 `docs/layout.md`

Anchors, constraints, `StackLayout`, `GroupBox`, `TabBar` page rules, a worked example reproducing the smoke-test layout, and `validate_layout`.

### 10.6 `docs/design.md`

Architecture overview; ownership model; the frame loop; the decisions in Phase 4 with alternatives; error-handling policy; threading statement; performance notes (allocation per frame, how to reduce it); future work list.

### 10.7 `CONTRIBUTING.md` and `CHANGELOG.md`

Style, tests required, how to run sanitizers, how to add a widget (checklist: header, implementation, theme usage, tests, docs entry, showcase entry).

### 10.8 Doc comments

Add concise doc comments to every public class and method in `zgui.hh`. Do not restate the name; state units, ownership, and edge-case behavior.

Verification: all relative links resolve (add a tiny link-check script); all code samples compile.

---

## 11. PHASE 8 — OPTIONAL WIDGET COMPLETION (GATED)

Only do this phase if the earlier phases are green. Add widgets in this priority order. Each widget requires: header declaration with docs, implementation, theme-consistent drawing, unit tests (hover/press/release/callback/edge cases/lifetime), a showcase entry, and a `docs/widgets.md` section. Do **one widget per commit**. Stop after each and re-run the full suite.

Priority list (skip any that already exist; check first):

1. **Separator** (horizontal/vertical, optional label).
2. **ScrollArea** — a container with vertical (and optional horizontal) scrolling, wheel support, draggable scrollbar, `content_bottom`/`content_right` aware, correct clipping and nested scroll behavior (inner area consumes wheel first, then outer when at the end). This is the most important missing piece; `TabBar` has an ad-hoc `page_scroll_offset_`, so evaluate whether `TabBar` pages can reuse it, but do not break existing behavior.
3. **ToggleButton / RadioGroup** (single selection among N, bound to `std::size_t&`).
4. **ProgressBar** (0–1 bound float, optional text).
5. **Tooltip** (hover delay, overlay on the `WindowManager`, clamped to the viewport, one at a time).
6. **ContextMenu / MenuList** (overlay with items, separators, disabled items, keyboard navigation; submenus optional).
7. **NumberInput** (text editing with validation for `int`/`float`, clamp on commit, up/down keys).
8. **MultiLineTextInput** (word wrap optional, vertical scroll, cursor navigation up/down).
9. **TreeView** (expand/collapse, selection, lazy children callback).
10. **Image/Texture widget** (requires a texture handle concept in `Graphics`: add `virtual void draw_texture(TextureId, Point, Point, Color tint)` with a default no-op and a capability query; implement in the SDL2/OpenGL backend; keep core free of GL types by using an opaque `std::uintptr_t` handle type).
11. **Focus and keyboard navigation** across widgets (Tab/Shift+Tab, Enter/Space activate, arrow keys in lists), with a visible focus ring from the theme. Treat as a cross-cutting feature and design it in `docs/design.md` before coding; keep it opt-in until proven.
12. **DPI/scale factor** in `Theme`/`spacing` (multiply constants by a runtime scale; backend supplies the scale). Design first, then implement.
13. **Clipboard** hooks in `Input` (`get_clipboard_text`/`set_clipboard_text` defaulted to no-op) and Ctrl+C/V/X/A in `TextInput`.

For every addition, preserve backward compatibility and the C++17/no-dependency rule.

---

## 12. PHASE 9 — NEW SHOWCASE RENDER TEST (FROM SCRATCH)

Replace `tests/render_test.cc` and the `layer_smoke_tests` demo with a new, small, public-API-only showcase. The old file is nanohook-flavoured, 2,681 lines, and depends on Source-engine assets; **delete it** after the new showcase covers equivalent framework features (do not port Source-model code; that lives in `extras/`).

### 12.1 Constraints

- Location: `examples/showcase/main.cc` (plus at most 2–3 helper files). Target about 300–500 lines total, readable top to bottom as a tutorial.
- Uses only the public API (`zgui.hh` and the SDL2/OpenGL backend entry point `run_sdl2_opengl_application`). No private headers, no friend access, no nanohook code, no assets.
- One function per tab/page: `build_basic_controls`, `build_inputs_and_binding`, `build_selection_widgets`, `build_color_tools`, `build_tables`, `build_layout_and_anchors`, `build_windows_and_overlays`, `build_viewport_demo`, `build_theme_editor`, `build_stress_test`. Each function is self-contained and under ~60 lines.
- Command-line flags: `--frames N` (render N frames then exit with code 0, for CI), `--theme dark|light|<name>`, `--fuzz` (feed random input; reuse the fuzz generator idea).
- No hidden global state beyond a single `struct AppState` holding every bound value.

### 12.2 Required demonstrations

- Every widget that exists after Phase 8, in at least one realistic configuration.
- A **theme editor** page: `ColorButton`/`ColorSwatch` for every `Theme` field, with a live preview (changes apply via `core::set_theme`), plus "Reset" and "Copy as C++" (prints the `Theme{...}` initializer to stdout).
- A **layout** page demonstrating anchors, fractional constraints, `StackLayout` rows, two- and three-column rows, nested `GroupBox` with `SubTabBar`, and a resizable window whose child layout reflows.
- A **windows and overlays** page: spawn/close windows, always-on-top toggle, frameless window, popup combo boxes near screen edges, tooltips and context menus (if added).
- A **viewport** page: a `Viewport` drawing a simple software-generated shape (for example a rotating wireframe cube drawn with `draw_line`, or a plain GL quad in `on_render`) that responds to rotate/pan/zoom/double-click-reset via `ViewportInput`. No external model files.
- A **table** page with 10,000 rows, sortable columns (sorting implemented in the demo through `clear_rows`/`add_row`), colored cells, row backgrounds, selection detail panel, and a column-visibility menu.
- A **stress test** page: slider controlling the number of widgets (up to several thousand) with a frame-time readout (use `SDL_GetPerformanceCounter` in the demo's own code, not in the core).
- A **text** page: truncation behavior, alignment, the ASCII fallback.
- A status line showing zgui version, FPS, widget count, visible widget count (`widget_count()` / `visible_widget_count()`).

### 12.3 Quality bar

- The showcase must run with `--frames 60` and return 0 in CI (or be build-only if there is no GL in CI; say which).
- The same UI-building code is reused by a headless test (`tests/unit/test_showcase_builds.cc`) that constructs every page against the mock backend, runs 120 frames, calls `validate_layout`, and asserts no clip imbalance. To make that possible, split `main.cc` into `showcase_ui.hh/.cc` (builds pages against any `WindowManager`) and `main.cc` (SDL2 entry point).

---

## 13. PHASE 10 — FINAL VERIFICATION AND REPORT

Run, in a fresh clone:

1. `cmake --preset release && cmake --build --preset release -j && ctest --preset release --output-on-failure`
2. `cmake --preset asan-ubsan && cmake --build --preset asan-ubsan -j && ctest --preset asan-ubsan --output-on-failure`
3. `cmake --preset coverage ...` and print the coverage summary.
4. `cmake --preset no-exceptions ...` (if adopted).
5. Build with Clang and GCC; zero warnings with section 8.2 flags.
6. Build the SDL2/OpenGL backend and the showcase; run `--frames 60`. If impossible, mark `NOT VERIFIED` and say why.
7. Install the library and build `tests/install_consumer/` against it.
8. `python3 tools/pack.py` and build the packed `dist/` files against the same tests.
9. Re-run `tools/dump_api.py` into `docs/api_snapshot_after.txt` and diff against `api_snapshot_before.txt`; every difference must be explained in `CHANGELOG.md`.
10. Run the repo checks from Phase 1 (no assets, no binaries, license headers present).
11. Delete `docs/baseline_warnings.txt` and other scratch files.

### Final report format (print at the end of every phase in short form, and in full at the end)

```
PHASE <n> REPORT
Done:        <bullet list of completed tasks>
Verified:    <commands run and results>
NOT VERIFIED:<anything not run, with the reason>
Decisions:   <defaults taken that the owner may overrule>
Breaking:    <API changes, or "none">
Risks:       <anything suspicious you noticed but did not change>
Next:        <next phase>
```

Final report must additionally include: test count, coverage percentage, warning count (expected 0), lines of code before/after for `zgui.hh`, `zgui.cc`, backend, tests; a list of owner action items (fill in the copyright placeholder, consider `git filter-repo` for removed assets, choose UTF-8/split/context decisions, move `extras/source_model_viewer` into nanohook).

---

## 14. DEFINITION OF DONE (CHECKLIST)

- [ ] No Valve assets, no prebuilt binaries in the working tree; assets path ignored.
- [ ] `LICENSE` with upstream + owner copyright; SPDX/copyright headers on all source files; `THIRD_PARTY_NOTICES.md` present.
- [ ] No Source-engine code in `zgui.hh`, `zgui.cc`, or `backends/`; moved to `extras/source_model_viewer/` (off by default).
- [ ] Uninitialized members fixed; `-Wshadow`/`-Wconversion`/`-Wold-style-cast` clean.
- [ ] Single consistent error-handling policy implemented and documented.
- [ ] Draw-helper hiding fixed and tested.
- [ ] Lifetime/ordering tests pass under ASan/UBSan; fuzz test passes.
- [ ] Mock backend and ≥85% line coverage of `zgui.cc`.
- [ ] CMake with install/export, presets, CI on Linux/Windows/macOS.
- [ ] `tools/pack.py` produces working `dist/zgui.hh` + `dist/zgui.cc`.
- [ ] Docs: README, getting started, writing a backend, widgets, layout, design, contributing, changelog.
- [ ] New showcase from scratch, public API only, ≤ ~500 lines, headless build test included; old `render_test.cc` removed.
- [ ] Public API diff explained in `CHANGELOG.md`.
- [ ] Final report printed with all `NOT VERIFIED` items listed honestly.

---

## 15. WHAT NOT TO DO

- Do not rewrite the architecture (retained mode stays).
- Do not add third-party dependencies to the core or the tests.
- Do not split `zgui.cc` into multiple files unless the owner says so.
- Do not convert text handling to UTF-8 in this pass.
- Do not remove public symbols without a deprecated alias and a changelog entry.
- Do not commit generated build output, coverage data, or Source-engine assets.
- Do not claim verification you did not perform.
- Do not guess the owner's name or handle for the copyright line; leave the placeholder.
- Do not mix formatting, refactoring, and behavior changes in one commit.

Begin with Phase 0 now. Print the Phase 0 report, then continue through the phases in order.