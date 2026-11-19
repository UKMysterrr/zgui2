#!/usr/bin/env python3
# Copyright (c) 2019, zxvnme
# Copyright (c) <YEAR>, <OWNER NAME OR HANDLE>
# SPDX-License-Identifier: MIT

"""Check that relative Markdown links point to existing repository paths."""

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
LINK = re.compile(r"!?\[[^\]]*\]\(([^)]+)\)")
SCHEME = re.compile(r"^[A-Za-z][A-Za-z0-9+.-]*:")


def main() -> int:
    errors = []
    for markdown in ROOT.rglob("*.md"):
        if ".git" in markdown.parts or "maintainers" in markdown.parts:
            continue
        text = markdown.read_text(encoding="utf-8")
        in_code_block = False
        for line_number, line in enumerate(text.splitlines(), start=1):
            if line.lstrip().startswith("```"):
                in_code_block = not in_code_block
                continue
            if in_code_block:
                continue
            for match in LINK.finditer(line):
                target = match.group(1).strip().split(maxsplit=1)[0].strip("<>")
                if not target or target.startswith("#") or target.startswith("//"):
                    continue
                if SCHEME.match(target):
                    continue
                path = target.split("#", maxsplit=1)[0].split("?", maxsplit=1)[0]
                if not path:
                    continue
                resolved = (markdown.parent / path).resolve()
                if not resolved.exists():
                    errors.append(
                        f"{markdown.relative_to(ROOT)}:{line_number}: missing {target}"
                    )

    if errors:
        print("\n".join(errors))
        return 1
    print("All relative Markdown links resolve.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
