#!/usr/bin/env python3
"""Warp the harness into every map in turn and report the ones that look wrong.

Uses the debug-only gPlaytestWarp hook (D96) to jump straight from a fresh save
to each map in data/maps/map_groups.json, waits for the load, screenshots it and
reads the player's position and the metatile behaviour underfoot.

    tools/playtest/sweep.py --out /tmp/sweep

Flags a map when it fails to load, when the behaviour underfoot is invalid, or
when the frame is mostly black or nearly monochrome -- the signature of a
tileset that is being read the wrong way.
"""

import argparse
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import playtest  # noqa: E402  (same directory)


def map_list():
    """(group, num, name, dir) for every map, in map_groups.json order."""
    groups = json.load(open(os.path.join(ROOT, "data", "maps", "map_groups.json")))
    out = []
    for group, name in enumerate(groups["group_order"]):
        for num, mapname in enumerate(groups[name]):
            out.append((group, num, mapname, os.path.join(ROOT, "data", "maps", mapname)))
    return out


def layouts():
    if not hasattr(layouts, "cache"):
        layouts.cache = {l["id"]: l for l in json.load(
            open(os.path.join(ROOT, "data", "layouts", "layouts.json")))["layouts"]}
    return layouts.cache


def warp_target(mapdir):
    """A walkable tile near the middle of the map, avoiding warps.

    Warp id 0 is no good: on many maps it is the door you arrive on, which
    warps you straight back out again, and the two maps ping-pong forever.
    Collision lives in the top bits of each map.bin entry, so a free tile can
    be picked without the game's help.
    """
    try:
        j = json.load(open(os.path.join(mapdir, "map.json")))
    except OSError:
        return (5, 5)
    layout = layouts().get(j.get("layout"))
    if not layout:
        return (5, 5)
    w, h = layout["width"], layout["height"]
    try:
        blocks = open(os.path.join(ROOT, layout["blockdata_filepath"]), "rb").read()
    except OSError:
        return (w // 2, h // 2)
    warps = {(e["x"], e["y"]) for e in j.get("warp_events") or []}
    best = None
    for y in range(h):
        for x in range(w):
            i = (y * w + x) * 2
            if i + 1 >= len(blocks):
                continue
            entry = blocks[i] | (blocks[i + 1] << 8)
            if (entry >> 10) & 3:          # collision bits
                continue
            if (x, y) in warps:
                continue
            d = abs(x - w // 2) + abs(y - h // 2)
            if best is None or d < best[0]:
                best = (d, x, y)
    return (best[1], best[2]) if best else (w // 2, h // 2)


def map_info(mapdir):
    """(expected fraction of black, requires_flash) for a map.

    A map smaller than the screen is legitimately surrounded by black, and a
    cave that wants Flash is legitimately dark, so both have to be discounted
    before "mostly black" means anything.
    """
    try:
        j = json.load(open(os.path.join(mapdir, "map.json")))
    except OSError:
        return 0.0, False
    layout = layouts().get(j.get("layout"))
    if not layout:
        return 0.0, False
    # The screen shows 15x10 metatiles; anything past the edge is black.
    seen = min(layout["width"], 15) * min(layout["height"], 10)
    return 1.0 - seen / (15.0 * 10.0), bool(j.get("requires_flash"))


def build_script(maps, settle, state):
    """Play the opening once, snapshot it, then restore that snapshot before
    each map: a map that traps the player in a cutscene then costs only itself.
    """
    lines = ["include %s" % os.path.join(ROOT, "tools", "playtest", "scripts", "afterintro.txt"),
             "savestate %s" % state]
    for group, num, name, mapdir in maps:
        x, y = warp_target(mapdir)
        lines += [
            "loadstate %s" % state,
            "write16 gPlaytestWarp+2 %d" % group,
            "write16 gPlaytestWarp+4 %d" % num,
            "write16 gPlaytestWarp+6 65535",
            "write16 gPlaytestWarp+8 %d" % x,
            "write16 gPlaytestWarp+10 %d" % y,
            "write16 gPlaytestWarp 1",
            "wait %d" % settle,
            "echo MAP %d %d %s" % (group, num, name),
            "player",
            "shot %s" % name,
        ]
    return "\n".join(lines) + "\n"


def parse(output):
    """Group the harness's output into one record per map."""
    records, cur = [], None
    for line in output.splitlines():
        m = re.match(r"echo MAP (\d+) (\d+) (\S+)", line)
        if m:
            cur = {"group": int(m.group(1)), "num": int(m.group(2)),
                   "name": m.group(3), "log": []}
            records.append(cur)
            continue
        if cur is None:
            continue
        m = re.match(r"(px|py|elevation) = (-?\d+)", line)
        if m:
            cur[m.group(1)] = int(m.group(2))
        elif line.startswith("behavior = "):
            cur["behavior"] = line.split(" ", 2)[2]
        elif line.startswith("map = "):
            g, n = line.split("= ")[1].split(".")
            cur["landed"] = (int(g), int(n))
        elif line.startswith(("game:", "mgba:")):
            cur["log"].append(line)
    return records


def frame_stats(path):
    from PIL import Image
    img = Image.open(path).convert("RGB")
    px = list(img.getdata())
    black = sum(1 for p in px if p == (0, 0, 0))
    return black / len(px), len(set(px))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(ROOT, "playtest-out", "sweep"))
    ap.add_argument("--settle", type=int, default=150, help="frames to wait after each warp")
    ap.add_argument("--scale", type=int, default=1)
    ap.add_argument("--only", help="substring: sweep just the matching maps")
    ap.add_argument("--names", help="file of map names, one per line: sweep just those")
    args = ap.parse_args()

    maps = map_list()
    if args.only:
        maps = [m for m in maps if args.only.lower() in m[2].lower()]
    if args.names:
        want = set(open(args.names).read().split())
        maps = [m for m in maps if m[2] in want]
    os.makedirs(args.out, exist_ok=True)
    syms = playtest.load_symbols(os.path.join(ROOT, "pokecrystal.map"))
    state = os.path.join(args.out, "prelude.state")
    script = build_script(maps, args.settle, state)
    expanded = playtest.expand_text(script, syms, args.out)

    print("sweeping %d maps..." % len(maps), file=sys.stderr)
    proc = subprocess.run([playtest.HARNESS, os.path.join(ROOT, "pokecrystal.gba"), "-"],
                          input=expanded, text=True, capture_output=True)
    records = parse(playtest.decorate(proc.stdout))

    from PIL import Image
    bad = []
    for r in records:
        ppm = os.path.join(args.out, r["name"] + ".ppm")
        if os.path.exists(ppm):
            r["black"], r["colors"] = frame_stats(ppm)
            img = Image.open(ppm)
            if args.scale != 1:
                img = img.resize((img.width * args.scale, img.height * args.scale), Image.NEAREST)
            img.save(ppm[:-4] + ".png")
            os.remove(ppm)
        reasons = []
        if r.get("landed") != (r["group"], r["num"]):
            reasons.append("did not load (landed on %s)" % (r.get("landed"),))
        if r.get("behavior", "").endswith("MB_INVALID"):
            reasons.append("invalid behaviour underfoot")
        expected, flash = map_info(os.path.join(ROOT, "data", "maps", r["name"]))
        if not flash:
            if r.get("black", 0) > expected + 0.3:
                reasons.append("%.0f%% black (%.0f%% expected)" % (100 * r["black"], 100 * expected))
            if r.get("colors", 99) < 8 and expected < 0.6:
                reasons.append("only %d colours" % r["colors"])
        if r["log"]:
            reasons.append("; ".join(r["log"][:2]))
        if reasons:
            bad.append((r, reasons))

    json.dump(records, open(os.path.join(args.out, "sweep.json"), "w"), indent=1)
    print("\n%d maps swept, %d flagged" % (len(records), len(bad)))
    for r, reasons in bad:
        print("  %-44s %s" % (r["name"], "; ".join(reasons)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
