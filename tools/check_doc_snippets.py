#!/usr/bin/env python3
# Copyright (c) 2019, zxvnme
# Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
# SPDX-License-Identifier: MIT

"""Compile every fenced C++ example in Markdown with the configured compiler."""

import os
import re
import shlex
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CPP_FENCE = re.compile(r"^\s*```(?:cpp|c\+\+)\s*$", re.IGNORECASE)


def snippets(markdown: Path):
    lines = markdown.read_text(encoding="utf-8").splitlines()
    line_number = 0
    while line_number < len(lines):
        if not CPP_FENCE.match(lines[line_number]):
            line_number += 1
            continue
        start = line_number + 1
        line_number += 1
        code = []
        while line_number < len(lines) and not lines[line_number].lstrip().startswith("```"):
            code.append(lines[line_number])
            line_number += 1
        yield start, "\n".join(code) + "\n"
        line_number += 1


def main() -> int:
    compiler = shlex.split(os.environ.get("CXX", "c++"))
    include_flags = [
        f"-I{ROOT}",
        f"-I{ROOT / 'backends/sdl2_opengl'}",
    ]
    checked = 0
    with tempfile.TemporaryDirectory(prefix="zgui-doc-snippets-") as directory:
        for markdown in sorted(ROOT.rglob("*.md")):
            if ".git" in markdown.parts or "maintainers" in markdown.parts:
                continue
            for block_number, (line, code) in enumerate(snippets(markdown), start=1):
                source = Path(directory) / f"snippet_{checked:03}.cc"
                source.write_text('#include "zgui.hh"\n' + code, encoding="utf-8")
                result = subprocess.run(
                    compiler + ["-std=c++17", "-fsyntax-only"] + include_flags + [str(source)],
                    cwd=ROOT,
                    check=False,
                    capture_output=True,
                    text=True,
                )
                if result.returncode != 0:
                    print(f"{markdown.relative_to(ROOT)}:{line}: C++ example {block_number} failed:")
                    print(result.stderr, end="")
                    return result.returncode
                checked += 1

    print(f"Compiled {checked} fenced C++ examples.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
