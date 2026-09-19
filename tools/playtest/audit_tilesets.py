#!/usr/bin/env python3
"""Static audit of every layout against its tilesets.

Complements the harness sweep: the sweep sees what one screen of a map looks
like, this checks every block of every map. Flags layouts whose map.bin refers
to metatiles the tilesets do not contain, primaries that overflow the
640-metatile split, and tilesets missing attributes for their metatiles.
"""

import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
NUM_METATILES_IN_PRIMARY = 640
NUM_METATILES_TOTAL = 1024
METATILE_ID_MASK = 0x03FF


def tileset_paths():
    """gTileset_X -> (metatiles.bin, metatile_attributes.bin) via the C data."""
    sym_to_file = {}
    for line in open(os.path.join(ROOT, "src", "data", "tilesets", "metatiles.h")):
        m = re.match(r'const u\d+ (\w+)\[\] = INCBIN_U\d+\("([^"]+)"\);', line.strip())
        if m:
            sym_to_file[m.group(1)] = m.group(2)
    headers = open(os.path.join(ROOT, "src", "data", "tilesets", "headers.h")).read()
    out = {}
    for m in re.finditer(r"const struct Tileset (\w+) =\s*\{(.*?)\};", headers, re.S):
        body = m.group(2)
        mt = re.search(r"\.metatiles = (\w+)", body)
        at = re.search(r"\.metatileAttributes = (\w+)", body)
        sec = re.search(r"\.isSecondary = (\w+)", body)
        out[m.group(1)] = (sym_to_file.get(mt.group(1) if mt else ""),
                           sym_to_file.get(at.group(1) if at else ""),
                           bool(sec and sec.group(1) == "TRUE"))
    return out


def size(path, per_entry):
    try:
        return os.path.getsize(os.path.join(ROOT, path)) // per_entry
    except (OSError, TypeError):
        return None


def main():
    tilesets = tileset_paths()
    layouts = json.load(open(os.path.join(ROOT, "data", "layouts", "layouts.json")))["layouts"]
    problems = []

    for name, (mt, at, is_secondary) in sorted(tilesets.items()):
        n = size(mt, 16)
        if n is None:
            continue
        attrs = size(at, 2)
        if not is_secondary and n > NUM_METATILES_IN_PRIMARY:
            problems.append("%s: primary holds %d metatiles, the split allows %d"
                            % (name, n, NUM_METATILES_IN_PRIMARY))
        if is_secondary and n > NUM_METATILES_TOTAL - NUM_METATILES_IN_PRIMARY:
            problems.append("%s: secondary holds %d metatiles, only %d addressable"
                            % (name, n, NUM_METATILES_TOTAL - NUM_METATILES_IN_PRIMARY))
        if attrs is not None and attrs < n:
            problems.append("%s: %d metatiles but only %d attribute entries"
                            % (name, n, attrs))

    for layout in layouts:
        prim = tilesets.get(layout["primary_tileset"])
        sec = tilesets.get(layout["secondary_tileset"])
        if not prim or not sec:
            problems.append("%s: unknown tileset" % layout["id"])
            continue
        nprim, nsec = size(prim[0], 16), size(sec[0], 16)
        try:
            data = open(os.path.join(ROOT, layout["blockdata_filepath"]), "rb").read()
        except OSError:
            problems.append("%s: missing %s" % (layout["id"], layout["blockdata_filepath"]))
            continue
        expected = layout["width"] * layout["height"] * 2
        if len(data) != expected:
            problems.append("%s: map.bin is %d bytes, %dx%d needs %d"
                            % (layout["id"], len(data), layout["width"], layout["height"], expected))
        worst = None
        for i in range(0, len(data) - 1, 2):
            mid = (data[i] | (data[i + 1] << 8)) & METATILE_ID_MASK
            limit = nprim if mid < NUM_METATILES_IN_PRIMARY else NUM_METATILES_IN_PRIMARY + nsec
            if mid >= limit and (worst is None or mid > worst[0]):
                worst = (mid, limit)
        if worst:
            problems.append("%s: metatile %d is past the end of its tilesets (%d available)"
                            % (layout["id"], worst[0], worst[1]))

    print("%d layouts, %d tilesets checked" % (len(layouts), len(tilesets)))
    print("%d problems" % len(problems))
    for p in problems:
        print("  " + p)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
