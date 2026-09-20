#!/usr/bin/env python3
"""Driver for the headless play-test harness.

Resolves symbol names against pokecrystal.map so scripts can say
`read32 gSaveBlock1Ptr` instead of an address, expands a few convenience
commands, runs tools/playtest/playtest, and converts the screenshots to PNG.

    tools/playtest/playtest.py scripts/newbark.txt --out /tmp/shots

Script syntax is the harness's own (see playtest.c --help) plus:
    where                 print the player's map group/num and x/y
    player                position, elevation and the behaviour underfoot
    sym <name>            print a symbol's address
    flag <FLAG_X>         read a save-block flag by name
    setflag/clearflag <FLAG_X>
    var <VAR_X> / setvar <VAR_X> <value>
    advance [maxframes]   answer dialogue until the running script ends
    untilmap <group> <num> [maxframes]
    waitfade [maxframes]
Addresses may be written `symbol`, `symbol+0x10`, or a literal.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
HARNESS = os.path.join(ROOT, "tools", "playtest", "playtest")

# struct SaveBlock1: pos at +0, location (WarpData) at +4.
SB1_MAPGROUP, SB1_MAPNUM, SB1_X, SB1_Y = 4, 5, 8, 10

# enum in src/script.c. A script sits in CONTEXT_WAITING for the whole of every
# msgbox and every waitmovement, so only CONTEXT_SHUTDOWN means it has ended.
CONTEXT_RUNNING = 0
CONTEXT_SHUTDOWN = 2

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


CONST_CACHE = os.path.join(ROOT, "tools", "playtest", ".constants")


def load_constants():
    """FLAG_* and VAR_* name -> number.

    The headers define nearly every entry relative to the one before it and
    reach into the trainer and rematch ids to do it, so rather than reimplement
    the preprocessor the names are handed to the host compiler and it prints
    what it makes of them. Cached, because the headers rarely move.
    """
    heads = [os.path.join(ROOT, "include", "constants", h)
             for h in ("flags.h", "vars.h")]
    newest = max(os.path.getmtime(h) for h in heads)
    if os.path.exists(CONST_CACHE) and os.path.getmtime(CONST_CACHE) > newest:
        return {n: int(v) for n, v in
                (l.split() for l in open(CONST_CACHE) if l.strip())}

    names = set()
    for head, prefix in zip(heads, ("FLAG_", "VAR_")):
        names |= set(re.findall(r"^#define\s+(%s\w+)" % prefix,
                                open(head).read(), re.M))
    src = ['#include "constants/flags.h"', '#include "constants/vars.h"',
           "#include <stdio.h>", "int main(void){"]
    src += ['printf("%%s %%d\\n","%s",(int)(%s));' % (n, n) for n in sorted(names)]
    src += ["return 0;}"]
    tmp = CONST_CACHE + ".c"
    open(tmp, "w").write("\n".join(src) + "\n")
    exe = CONST_CACHE + ".bin"
    cc = subprocess.run(["cc", "-w", "-I", os.path.join(ROOT, "include"),
                         "-o", exe, tmp], capture_output=True, text=True)
    if cc.returncode:
        sys.stderr.write(cc.stderr)
        return {}
    out = subprocess.run([exe], capture_output=True, text=True).stdout
    open(CONST_CACHE, "w").write(out)
    for path in (tmp, exe):
        os.remove(path)
    return {n: int(v) for n, v in (l.split() for l in out.splitlines() if l.strip())}


CONSTANTS = load_constants()
FLAGS = {k: v for k, v in CONSTANTS.items() if k.startswith("FLAG_")}
VARS = {k: v for k, v in CONSTANTS.items() if k.startswith("VAR_")}


def objdump():
    for cand in (os.path.join(os.environ.get("DEVKITARM", ""), "bin",
                              "arm-none-eabi-objdump"),
                 "/opt/devkitpro/devkitARM/bin/arm-none-eabi-objdump",
                 "arm-none-eabi-objdump"):
        if os.path.exists(cand) or shutil.which(cand):
            return cand
    return None


def save_layout(elf):
    """(flags byte offset, var index bias) inside SaveBlock1.

    struct SaveBlock1's offsets shift with the expansion's FREE_* build
    options, and the comments in global.h are stale, so they are read out of
    the ROM: FlagGet and GetVarPointer both carry the offset they use in their
    constant pool, and those are the numbers the game itself believes.
    """
    od = objdump()
    if not od:
        return 0x1248, -0x35E5
    def words(fn):
        out = subprocess.run([od, "-d", "--disassemble=" + fn, elf],
                             text=True, capture_output=True).stdout
        return [int(w, 16) for w in re.findall(r"\.word\s+0x([0-9a-f]{8})", out)]
    flags = [w for w in words("FlagGet") if 0 < w < 0x10000]
    bias = [w - (1 << 32) for w in words("GetVarPointer")
            if 0xFFFF0000 < w < 0xFFFFFFFF and w != 0xFFFF8000]
    return (flags[0] if flags else 0x1248,
            bias[0] if bias else -0x35E5)


FLAG_OFFSET, VAR_BIAS = save_layout(os.path.join(ROOT, "pokecrystal.elf"))


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
        if cmd in ("flag", "setflag", "clearflag"):
            # The save block's flag bitfield, addressed by FLAG_ name. Setting
            # a flag is how a story beat is handed the prerequisites of the
            # beats before it without replaying them.
            name = parts[1]
            if name not in FLAGS:
                sys.exit("playtest: unknown flag '%s'" % name)
            idx = FLAGS[name]
            if idx >= 0x4000:
                sys.exit("playtest: %s is a special flag, not in the save block" % name)
            what = {"flag": "read", "setflag": "set", "clearflag": "clear"}[cmd]
            out.append("pbit %s 0x%X %d %s %s" % (resolve("gSaveBlock1Ptr", syms),
                                                  FLAG_OFFSET + idx // 8, idx % 8,
                                                  what, name))
            continue
        if cmd in ("var", "setvar"):
            name = parts[1]
            if name not in VARS:
                sys.exit("playtest: unknown var '%s'" % name)
            off = (VARS[name] + VAR_BIAS) * 2
            if cmd == "var":
                out.append("pread %s 0x%X 2 %s" % (resolve("gSaveBlock1Ptr", syms), off, name))
            else:
                out.append("pwrite %s 0x%X 2 %s" % (resolve("gSaveBlock1Ptr", syms),
                                                  off, parts[2]))
            continue
        if cmd == "advance":
            # Answer dialogue until the script that is running finishes. A beat
            # that never finishes is the interesting result, so the timeout is
            # reported rather than ignored.
            limit = parts[1] if len(parts) > 1 else "12000"
            status = resolve("sGlobalScriptContextStatus", syms)
            out.append("mash A 12")
            # Wait for the script to start before waiting for it to end. The
            # context is stopped both before a cutscene begins and after it
            # finishes, so without this a beat that never fires at all reads
            # exactly like one that ran instantly.
            out.append("until abs %s 0 1 ne %d 300 scriptstart" % (status, CONTEXT_SHUTDOWN))
            out.append("until abs %s 0 1 eq %d %s script" % (
                status, CONTEXT_SHUTDOWN, limit))
            out.append("mash NONE")
            out.append("wait 20")
            continue
        if cmd == "untilmap":
            # Wait for a warp to land. mapGroup and mapNum are adjacent bytes,
            # so one 16-bit compare covers both.
            group, num = int(parts[1], 0), int(parts[2], 0)
            limit = parts[3] if len(parts) > 3 else "900"
            out.append("until ptr %s 0x%X 2 eq 0x%X %s" % (resolve("gSaveBlock1Ptr", syms),
                                                           SB1_MAPGROUP, group | (num << 8),
                                                           limit + " warp"))
            continue
        if cmd == "waitfade":
            limit = parts[1] if len(parts) > 1 else "600"
            # gPaletteFade.active is the top bit of the bitfield word at +12.
            out.append("until abs %s 0 4 lt 0x80000000 %s fade" %
                       (resolve("gPaletteFade+12", syms), limit))
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
