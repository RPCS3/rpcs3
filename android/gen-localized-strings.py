#!/usr/bin/env python3
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SRC = ROOT / ".." / "rpcs3" / "rpcs3qt" / "localized_emu.h"
DST = ROOT / "app" / "src" / "main" / "cpp" / "localized_strings.inl"

CASE = re.compile(r'case\s+localized_string_id::([A-Za-z_0-9]+)\s*:\s*return\s+(.*?);\s*$')


def read_literal(text, start):
    if text[start] != '"':
        return None, start
    out = []
    i = start + 1
    while i < len(text):
        c = text[i]
        if c == '\\':
            out.append(text[i:i + 2])
            i += 2
            continue
        if c == '"':
            return ''.join(out), i + 1
        out.append(c)
        i += 1
    return None, i


def main():
    if not SRC.is_file():
        sys.exit(f"missing {SRC}")

    entries = []
    for line in SRC.read_text(encoding='utf-8').splitlines():
        m = CASE.search(line.strip())
        if not m:
            continue

        name, expr = m.group(1), m.group(2).strip()

        if expr.startswith('""'):
            entries.append((name, ''))
            continue

        idx = expr.find('tr(')
        if idx < 0:
            continue

        j = expr.find('"', idx)
        if j < 0:
            continue

        literal, _ = read_literal(expr, j)
        if literal is None:
            continue

        entries.append((name, literal))

    seen = set()
    unique = []
    for name, value in entries:
        if name in seen:
            continue
        seen.add(name)
        unique.append((name, value))

    lines = [
        "MAKE_STRING(%s, \"%s\")," % (name, value) for name, value in unique
    ]

    DST.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    print(f"wrote {len(unique)} strings to {DST}")


if __name__ == "__main__":
    main()
