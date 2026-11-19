# zgui polish progress

Source of truth: [master prompt](copilot/master_prompt.md). The working tree
contained pre-existing changes at handoff; they are being preserved and included
in the polish commits where they belong.

| Step | Status | Commit | Notes |
|---|---|---|---|
| A. Stabilize and commit handoff | done | `43a8092`, `2d4f4a3`, `88efc99`, `418eced` | Handoff and Phase 0–3 partial work are committed. Historical assets remain in Git history, but no protected files remain in the working tree. |
| B. Minimal build system | done | `464c856`, `0f8664a`, `6944f37` | Root CMake/presets build core/backend/extras with GCC warnings-as-errors; a dedicated format-only commit passed earlier, though the formatter binary is not currently available on PATH. Clang compiler unavailable. |
| C. Test infrastructure | done | `3a20d4c` | Header-only harness, mock graphics/input, CTest registration, clip-stack diagnostics, and primitive/lifetime/manager mutation regressions. Release and ASan/UBSan builds and tests pass. |
| D. Finish Phase 3 correctness | done | — | Core initialization, ownership, no-exception fallbacks, indices, geometry, clipping, bounded truncation, popup placement/dismissal, pointer capture, live z-order, and layout validation are implemented with focused regressions. The extended lifetime and edge suite passes in Release and ASan/UBSan. |
| E. Phase 4 decisions | done | `0052ffd` | ASCII fallback, safe core/theme lifecycle, mutable runtime theme, reference bindings and owning `Bound<T>`. Design decisions documented. |
| F. Complete test suite | done | — | The 62 core cases cover primitives, drawing/topology/clips, widget input and bindings, popup and manager lifetimes, empty tables/tabs, constrained and anchored layouts, 10,000-row bounded drawing, layout diagnostics, and a fixed-seed 100,000-frame kitchen-sink sequence. The HSV round-trip test covers a grid of six hues, four saturation/value steps, and alpha endpoints. `zgui.cc` coverage is 90.52%, above the 85% goal. |
| G. CI, install, packer, version | done | — | Added GNU install-directory-aware rules for the core library and public header, exported `zgui::zgui`, and generated relocatable `zguiConfig.cmake`/version files. An out-of-tree consumer found, linked, built, and ran against a temporary install prefix. Added GitHub Actions for formatting, GCC/Clang/MSVC core builds, backend build/tests, sanitizers, coverage, no-exceptions, install consumer, and packed-source tests. Added version macros, a standard-library packer and changelog. The canonical core is already the requested two files, so packing stages those files and adds a git/version stamp instead of concatenating split modules. Hosted CI and non-local platforms are NOT VERIFIED until Actions runs. |
| H. Documentation | done | — | Replaced the project README; added getting-started, backend, widget, layout, design, documentation-index, and contributing guides. Added comments across the public API, a relative-link checker, and CI compilation of fenced C++ examples; clarified the extra's lifetime-safe callback example. Link check and all 34 C++ examples pass locally. |
| I. Optional widgets | done | `7dbfa99`, `02c9b69`, `a0f4962`, `bc96fc7`, `2a88294` | Added `Separator`, `ScrollArea`, `ToggleButton`, `RadioGroup`, `ProgressBar`, and delayed `Tooltip`. Scroll areas provide clipping, bounded vertical/optional horizontal scrolling, draggable scrollbars, and inner-first wheel handoff; toggle/radio controls support bound values and callbacks; tooltips are viewport-clamped and exclusive per manager. Optional additions were limited to these six widgets; remaining priority-list items were not required for the selected scope. Release, ASan/UBSan, and no-exceptions suites pass (72 core cases); all 34 C++ documentation examples compile. |
| J. New showcase | done | `6e124d2` | Added ten public-API pages, shared with the 120-frame headless layout/clip regression, frame/status metrics, themes, CLI flags, synthetic mouse/wheel fuzz input, table column menu, and a software-GL/Xvfb CI smoke run. Removed the legacy demo. The 60-frame light-theme fuzz run also passed on the available local display. |
| K. Final verification and report | done | — | Release, ASan/UBSan, no-exceptions, coverage, install-consumer, extras, packed-source, documentation, hygiene, and local 60-frame showcase checks pass. Core coverage is 90.77%; Clang, clang-format, hosted CI, and non-local platforms remain unverified. |

## Handoff facts

- Branch: `polish/v1`.
- GCC baseline core compile: 28 diagnostics under `-Wall -Wextra -Wpedantic -Wshadow -Wconversion`; after primitive/geometry fixes the core compiles with zero diagnostics under those flags.
- SDL2 2.30.0 and OpenGL headers are installed. Backend/extras build succeeded. The showcase completed 60 frames with `--theme light --fuzz` on the available display; Xvfb itself is unavailable locally.
- `clang++`, `clang-format`, and `xvfb-run` are not available on the current PATH; the configured CI jobs cover formatter and Xvfb checks.
- Historical asset paths exist in Git history; no model assets or prebuilt render binary remain in the working tree.
- Previous informal checks passed but do not count as permanent tests: primitive defaults, child detach under ASan/UBSan, and five render-test modes.
- Step A commits: `43a8092` licensing/configuration/instructions; `2d4f4a3` renderer extraction; `88efc99` initial core fixes; `418eced` Phase 0 records.
- Step B commits: `464c856` strict GL conversions; `0f8664a` CMake targets/presets; `6944f37` dedicated clang-format pass.

## Current status

- Phase D implemented no-exceptions core validation, safe pointer fallbacks, table/tab/index and layout fixes, clip-stack diagnostics, bounded truncation tracking, slider metrics, API cleanup, and noncopyable runtime ownership objects. GCC `-Weffc++` was run as a one-time scan; its remaining reports concern default-member-initialized fields and pointer-bearing polymorphic classes whose copying is prohibited.
- Phase E implements printable-ASCII input, atlas fallback and bounds checking, explicit core initialize/shutdown, switchable themes, and reference/`Bound<T>` bindings. Color popups retain bound values independently of their widgets.
- Baseline validation before optional widgets: Release, ASan/UBSan, no-exceptions, and coverage builds/tests pass (62 core test cases plus one optional backend text test). With the optional widgets implemented so far, Release, ASan/UBSan, and no-exceptions suites pass (72 core cases); coverage has not been rerun with them.
- Packaging validation: Release install and out-of-tree consumer configure/build/run pass; the Release, ASan/UBSan, and no-exceptions test presets pass with the install/export rules enabled.
- CI validation: the workflow parses as YAML, and its GCC core configure/build/test commands pass locally. Other hosted matrix platforms and the backend job await GitHub Actions.
- Packed-source validation: the packer generated both distribution files and the packed CMake build ran the same headless core suite successfully (62 cases).
- Documentation validation: all relative Markdown links resolve, all 34 fenced C++ examples compile, and Release plus backend tests pass. A local clang-format binary is unavailable; the configured GitHub Actions format job will verify formatting.
- Expanded test validation: Release, ASan/UBSan, no-exceptions, and coverage presets pass; the core suite reports 73/73 cases including backend text coverage, and current `zgui.cc` line coverage is 90.77%.
- Phase J validation: GCC Release with warnings-as-errors builds core, backend, tests, and showcase; the headless showcase runs 120 frames with valid layout and balanced clips. The SDL example completed 60 frames with light theme and fuzz input; invalid CLI arguments return status 2. CI also runs the 60-frame example under Xvfb with Mesa software rendering.
