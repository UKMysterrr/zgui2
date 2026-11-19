# Contributing

## Style

- Keep the core C++17-only and dependency-free.
- Follow `.clang-format`; use 4-space indentation.
- Keep changes focused and preserve public API compatibility. Document any
  unavoidable breaking change in `CHANGELOG.md`.
- Do not add third-party assets or backend-specific headers to the core.

## Tests

Every correctness fix needs a focused headless regression test. Run the
smallest relevant test during development, then run the release and sanitizer
presets before submitting broader changes:

```sh
cmake --preset release
cmake --build --preset release
ctest --preset release --output-on-failure
cmake --preset asan-ubsan
cmake --build --preset asan-ubsan
ctest --preset asan-ubsan --output-on-failure
```

The `no-exceptions` preset is also required for changes to core error handling.
The CI workflow runs the GCC/Clang/MSVC matrix, backend checks, coverage,
formatting, package consumer, documentation examples, and packed-source suite.

## Adding a widget

1. Check for an existing widget or reusable helper first.
2. Add the public declaration and concise API comments in `zgui.hh`.
3. Implement drawing and input behavior in `zgui.cc`, using `Theme` and
   `spacing::` values.
4. Add headless tests for drawing, input transitions, callbacks, edge cases,
   and lifetime.
5. Add a section and compiled example to `docs/widgets.md`.
6. Add the widget to the public-API showcase and its headless page-construction
   test.
7. Run formatting, Release, sanitizer, and no-exceptions checks as applicable.
