#!/usr/bin/env python3
# Copyright (c) 2019, zxvnme
# Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
# SPDX-License-Identifier: MIT

"""Print a regex-based inventory of declarations in the zgui public headers.

This intentionally avoids a compiler dependency. It reports declaration-like
lines and may miss declarations split across lines or include implementation
details from inline bodies; review the headers when a complete AST is needed.
"""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
HEADERS = (
    ROOT / "zgui.hh",
    ROOT / "backends/sdl2_opengl/zgui_impl_sdl2_opengl.hh",
)

TYPE_DECLARATION = re.compile(
    r"^(?:class|struct|namespace)\s+\w+|^enum(?:\s+class)?\s+\w+"
)
CONSTANT = re.compile(r"\bconstexpr\b.*=")
CONTROL = re.compile(r"^(?:if|for|while|switch|catch|return)\b")
CALLABLE_NAME = re.compile(r"(?:~?\w+|operator\s*\S+)\s*$")


def callable_signature(lines: list[str], start: int) -> tuple[str, int] | None:
    first = lines[start].strip()
    if "(" not in first or "->" in first or "=" in first or CONTROL.match(first):
        return None

    collected = first
    for index in range(start, min(start + 8, len(lines))):
        if index != start:
            collected += " " + lines[index].strip()
        prefix = collected.split("(", maxsplit=1)[0]
        if not CALLABLE_NAME.search(prefix):
            return None

        terminator = min(
            (position for position in (collected.find(";"), collected.find("{")) if position >= 0),
            default=-1,
        )
        if terminator >= 0:
            signature = " ".join(collected[:terminator].split())
            return signature, index
    return None


def main() -> None:
    for header in HEADERS:
        relative = header.relative_to(ROOT)
        source_lines = header.read_text(encoding="utf-8").splitlines()
        skipping_macro = False
        line_number = 0
        while line_number < len(source_lines):
            raw_line = source_lines[line_number]
            line = raw_line.split("//", maxsplit=1)[0].strip()
            if skipping_macro:
                skipping_macro = line.endswith("\\")
                line_number += 1
                continue
            if not line:
                line_number += 1
                continue
            if line.startswith("#define"):
                macro_name = line.split(maxsplit=1)[1].split(maxsplit=1)[0]
                if not macro_name.endswith("_HH"):
                    print(f"{relative}:{line_number + 1}: {line.rstrip(chr(92)).strip()}")
                skipping_macro = line.endswith("\\")
                line_number += 1
                continue

            if TYPE_DECLARATION.match(line):
                print(f"{relative}:{line_number + 1}: {line}")
            elif CONSTANT.search(line):
                print(f"{relative}:{line_number + 1}: {line.rstrip(';')}")
            else:
                signature = callable_signature(source_lines, line_number)
                if signature:
                    declaration, last_line = signature
                    print(f"{relative}:{line_number + 1}: {declaration}")
                    line_number = last_line
            line_number += 1


if __name__ == "__main__":
    main()
