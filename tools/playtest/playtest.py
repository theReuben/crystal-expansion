#!/usr/bin/env python3
"""Driver for the headless play-test harness.

Resolves symbol names against pokecrystal.map so scripts can say
`read32 gSaveBlock1Ptr` instead of an address, expands a few convenience
commands, runs tools/playtest/playtest, and converts the screenshots to PNG.

    tools/playtest/playtest.py scripts/newbark.txt --out /tmp/shots

Script syntax is the harness's own (see playtest.c --help) plus:
    where            print the player's map group/num and x/y
    sym <name>       print a symbol's address
Addresses may be written `symbol`, `symbol+0x10`, or a literal.
"""

import argparse
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
HARNESS = os.path.join(ROOT, "tools", "playtest", "playtest")

# struct SaveBlock1: pos at +0, location (WarpData) at +4.
SB1_MAPGROUP, SB1_MAPNUM, SB1_X, SB1_Y = 4, 5, 8, 10

ADDR_CMDS = {"read8", "read16", "read32", "write8", "write16", "write32",
             "dump", "deref", "sym"}


def load_behaviors():
    """MB_* names, so a behaviour dump reads as English. They live in a plain
    enum, so the values have to be counted out."""
    names, value = {}, 0
    path = os.path.join(ROOT, "include", "constants", "metatile_behaviors.h")
    pat = re.compile(r"^\s*(MB_\w+)\s*(?:=\s*(0x[0-9A-Fa-f]+|\d+))?\s*,")
    try:
        for line in open(path):
            m = pat.match(line)
            if m:
                if m.group(2):
                    value = int(m.group(2), 0)
                names.setdefault(value, m.group(1))
                value += 1
    except OSError:
        pass
    return names


BEHAVIORS = load_behaviors()


def load_symbols(map_path):
    syms = {}
    # The ELF carries the file-local statics the linker map leaves out.
    elf = map_path[:-4] + ".elf"
    if os.path.exists(elf):
        nm = subprocess.run(["nm", elf], text=True, capture_output=True)
        for line in nm.stdout.splitlines():
            parts = line.split()
            if len(parts) == 3 and parts[1] not in "Uw":
                syms.setdefault(parts[2], int(parts[0], 16))
    pat = re.compile(r"^\s+0x([0-9a-f]{8,16})\s+(\S+)\s*$")
    with open(map_path, errors="replace") as f:
        for line in f:
            m = pat.match(line)
            if m:
                syms.setdefault(m.group(2), int(m.group(1), 16))
    return syms


def resolve(token, syms):
    m = re.fullmatch(r"([A-Za-z_][A-Za-z0-9_]*)([-+]0[xX][0-9a-fA-F]+|[-+]\d+)?", token)
    if not m:
        return token
    if m.group(1) not in syms:
        sys.exit("playtest: unknown symbol '%s'" % m.group(1))
    addr = syms[m.group(1)]
    if m.group(2):
        addr += int(m.group(2), 0)
    return "0x%08X" % addr


def expand(path, syms, outdir):
    return expand_lines(read_lines(path), syms, outdir)


def expand_text(text, syms, outdir, basedir=None):
    """Same as expand(), for a script held in memory."""
    return expand_lines(resolve_includes(text.splitlines(True), basedir or ROOT),
                        syms, outdir)


def expand_lines(lines, syms, outdir):
    out = []
    for raw in lines:
        line = raw.split("#")[0].strip()
        if not line:
            continue
        parts = line.split()
        cmd = parts[0]
        if cmd == "player":
            # gObjectEvents[gPlayerAvatar.objectEventId]: live position,
            # elevation and the behaviour of the metatile underfoot.
            base = resolve("gObjectEvents", syms)
            idx = resolve("gPlayerAvatar+5", syms)
            for off, label, size in ((0x10, "px", 16), (0x12, "py", 16),
                                     (0x1E, "behavior", 8), (0x0B, "elevation", 8)):
                out.append("indexed %s 24 %s %X %d %s" % (base, idx, off, size // 8, label))
            out.append("deref %s %d %d %s" % (resolve("gSaveBlock1Ptr", syms),
                                              SB1_MAPGROUP, 2, "mapwhere"))
            continue
        if cmd == "where":
            for off, label, size in ((SB1_MAPGROUP, "mapGroup", 8), (SB1_MAPNUM, "mapNum", 8),
                                     (SB1_X, "x", 16), (SB1_Y, "y", 16)):
                out.append("deref %s %d %d %s" % (resolve("gSaveBlock1Ptr", syms), off,
                                                  size // 8, label))
            continue
        if cmd == "shot":
            parts[1] = os.path.join(outdir, os.path.basename(parts[1]))
            if not parts[1].endswith(".ppm"):
                parts[1] += ".ppm"
        elif cmd in ADDR_CMDS and len(parts) > 1:
            parts[1] = resolve(parts[1], syms)
            if cmd == "sym":
                out.append("echo %s = %s" % (line.split()[1], parts[1]))
                continue
        out.append(" ".join(parts))
    return "\n".join(out) + "\n"


def read_lines(path, depth=0):
    """`include <file>` pulls in another script, relative to this one."""
    yield from resolve_includes(open(path), os.path.dirname(os.path.abspath(path)), depth)


def resolve_includes(lines, basedir, depth=0):
    if depth > 8:
        sys.exit("playtest: include nesting too deep")
    for raw in lines:
        parts = raw.split("#")[0].split()
        if parts and parts[0] == "include":
            inc = os.path.join(basedir, parts[1])
            yield from resolve_includes(open(inc), os.path.dirname(os.path.abspath(inc)),
                                        depth + 1)
        else:
            yield raw


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("script")
    ap.add_argument("--rom", default=os.path.join(ROOT, "pokecrystal.gba"))
    ap.add_argument("--map", default=os.path.join(ROOT, "pokecrystal.map"))
    ap.add_argument("--out", default=os.path.join(ROOT, "playtest-out"))
    ap.add_argument("--scale", type=int, default=2)
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()

    os.makedirs(args.out, exist_ok=True)
    syms = load_symbols(args.map)
    expanded = expand(args.script, syms, args.out)
    env = dict(os.environ)
    if args.verbose:
        env["PLAYTEST_VERBOSE"] = "1"
    proc = subprocess.run([HARNESS, args.rom, "-"], input=expanded, env=env,
                          text=True, capture_output=True)
    sys.stdout.write(decorate(proc.stdout))
    sys.stderr.write(proc.stderr)
    if proc.returncode:
        return proc.returncode

    try:
        from PIL import Image
    except ImportError:
        return 0
    for name in sorted(os.listdir(args.out)):
        if name.endswith(".ppm"):
            src = os.path.join(args.out, name)
            img = Image.open(src)
            if args.scale != 1:
                img = img.resize((img.width * args.scale, img.height * args.scale),
                                 Image.NEAREST)
            img.save(src[:-4] + ".png")
            os.remove(src)
    return 0


def decorate(text):
    """Turn the raw byte dumps of `where` back into numbers."""
    out = []
    for line in text.splitlines():
        m2 = re.match(r"dump mapwhere 0x[0-9A-F]+ ([0-9A-F]{4})$", line)
        if m2:
            raw = bytes.fromhex(m2.group(1))
            out.append("map = %d.%d" % (raw[0], raw[1]))
            continue
        m3 = re.match(r"dump (px|py|behavior|elevation) 0x[0-9A-F]+ ([0-9A-F]+)$", line)
        if m3:
            raw = bytes.fromhex(m3.group(2))
            v = int.from_bytes(raw, "little", signed=len(raw) > 1)
            if m3.group(1) == "elevation":
                v &= 0xF  # a 4-bit field sharing its byte
            if m3.group(1) == "behavior":
                out.append("behavior = 0x%02X %s" % (v, BEHAVIORS.get(v, "?")))
            else:
                out.append("%s = %d" % (m3.group(1), v))
            continue
        m = re.match(r"dump (mapGroup|mapNum|x|y) 0x[0-9A-F]+ ([0-9A-F]+)$", line)
        if m:
            raw = bytes.fromhex(m.group(2))
            out.append("%s = %d" % (m.group(1), int.from_bytes(raw, "little", signed=True)))
        else:
            out.append(line)
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    sys.exit(main())
