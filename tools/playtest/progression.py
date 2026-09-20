#!/usr/bin/env python3
"""Run every on-entry story script in the game and report the ones that stall.

The map sweep proves a map draws. This proves its cutscene still fires: the
185 MAP_SCRIPT_ON_FRAME_TABLE entries across 114 maps are the spine of the
story, and each one is a script that a port can silently break -- a missing
object event, a flag that moved, a movement that never finishes.

For each entry the harness restores a fresh after-intro save, sets the trigger
variable to the value the table waits for, warps the player onto the map, and
answers dialogue until the script says it has finished. A beat passes when the
script both finishes and leaves behind one of the flags or variables its own
source says it sets; it fails when the script never ends (a stall, the thing
a player would call a freeze) or ends having changed nothing.

    tools/playtest/progression.py --out /tmp/prog
    tools/playtest/progression.py --only NewBarkTown_ProfessorElmsLab -v

Hand-written beats that need more than an entry trigger -- taking the starter,
winning a gym battle -- live in beats.py and run the same way.
"""

import argparse
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import playtest  # noqa: E402  (same directory)
import sweep     # noqa: E402
import beats as beats_module  # noqa: E402


def map_numbers():
    """map name -> (group, num)."""
    groups = json.load(open(os.path.join(ROOT, "data", "maps", "map_groups.json")))
    out = {}
    for group, gname in enumerate(groups["group_order"]):
        for num, mapname in enumerate(groups[gname]):
            out[mapname] = (group, num)
    return out


SCRIPT_LABEL = re.compile(r"^(\w+):{1,2}\s*$")
MAP_SCRIPT_2 = re.compile(r"^\s*map_script_2\s+(\w+)\s*,\s*(\w+)\s*,\s*(\w+)")
# Poryscript names the table after the constant; the hand-written Emerald
# maps call it OnFrame. Both mean the same table.
ON_FRAME = re.compile(r"(MAP_SCRIPT_ON_FRAME_TABLE|OnFrame)$")


# `goto`, but also `goto_if_unset FLAG, Label` and `call_if_eq ..., Label`:
# the label a branch jumps to is always its last argument.
BRANCH = re.compile(r"^\s*(?:call|goto)\w*\s+(.*)$")


def branch_target(line, bodies):
    m = BRANCH.match(line)
    if not m:
        return None
    label = m.group(1).split(",")[-1].strip()
    return label if label in bodies else None


def script_bodies(text):
    """label -> the lines under it, for one scripts.inc."""
    bodies, label = {}, None
    for line in text.splitlines():
        m = SCRIPT_LABEL.match(line)
        if m:
            label = m.group(1)
            bodies[label] = []
        elif label:
            bodies[label].append(line)
    return bodies


def effects(bodies, label, seen=None):
    """The flags and vars a script sets, following the calls it makes.

    A cutscene's own source is the only honest statement of what finishing it
    should look like, so the expectation is read from there rather than
    written out by hand and left to rot.
    """
    if seen is None:
        seen = set()
    if label in seen or label not in bodies:
        return [], []
    seen.add(label)
    flags, vars_ = [], []
    for line in bodies[label]:
        m = re.match(r"\s*setflag\s+(\w+)", line)
        if m:
            flags.append(m.group(1))
        m = re.match(r"\s*setvar\s+(\w+)\s*,\s*(\w+)", line)
        if m:
            vars_.append((m.group(1), m.group(2)))
        target = branch_target(line, bodies)
        if target:
            f, v = effects(bodies, target, seen)
            flags += f
            vars_ += v
    return flags, vars_


def reachable(bodies, label, seen=None):
    """Every line a script can reach, following its calls and gotos."""
    if seen is None:
        seen = set()
    if label in seen or label not in bodies:
        return []
    seen.add(label)
    lines = list(bodies[label])
    for line in bodies[label]:
        target = branch_target(line, bodies)
        if target:
            lines += reachable(bodies, target, seen)
    return lines


def constant(token):
    """A script argument as a number, or None if it names something we do not
    resolve (species, items -- none of which are ever a var's state)."""
    try:
        return int(token, 0)
    except ValueError:
        return playtest.CONSTANTS.get(token)


def derive_beats(only=None):
    out = []
    for name in sorted(os.listdir(os.path.join(ROOT, "data", "maps"))):
        if only and name not in only:
            continue
        path = os.path.join(ROOT, "data", "maps", name, "scripts.inc")
        if not os.path.exists(path):
            continue
        text = open(path).read()
        if "map_script_2" not in text:
            continue
        bodies = script_bodies(text)
        # Only the on-frame table. The on-warp table runs its scripts as the
        # map loads rather than through the global script context, so there is
        # nothing for the harness to watch start or finish.
        table = None
        for line in text.splitlines():
            m = SCRIPT_LABEL.match(line)
            if m:
                table = m.group(1)
            m = MAP_SCRIPT_2.match(line)
            if not m or not table or not ON_FRAME.search(table):
                continue
            var, value, label = m.group(1), m.group(2), m.group(3)
            value = constant(value)
            if var not in playtest.VARS or value is None:
                continue
            flags, vars_ = effects(bodies, label)
            if "GameClear" in "\n".join(reachable(bodies, label)):
                # The hall of fame hands control to the credits and never
                # returns to the field; there is no "after" to measure.
                continue
            # Special flags (>= 0x4000) live outside the save block and are
            # scratch space, so they say nothing about whether a beat ran.
            expect = [("flag", f, 1) for f in dict.fromkeys(flags)
                      if playtest.FLAGS.get(f, 0x4000) < 0x4000]
            # A trigger variable that the script moves on is the clearest
            # signal of all: the table will not fire again.
            # A script that warps away takes its temporary variables with it:
            # VAR_TEMP_* is cleared by the next map load, so it cannot be read
            # back afterwards and says nothing either way.
            warps = re.search(r"^\s*warp\w*\s",
                              "\n".join(reachable(bodies, label)), re.M)
            if any(v == var for v, _ in vars_) and not (
                    warps and var.startswith("VAR_TEMP")):
                expect.insert(0, ("var-not", var, value))
            out.append(dict(name="%s:%s" % (name, label.split("_")[-1]),
                            map=name, setup=["setvar %s %d" % (var, value)],
                            act=None, expect=expect, label=label))
    return out


def hand_beats(only=None):
    out = []
    for beat in beats_module.BEATS:
        if only and beat["map"] not in only and beat["name"] not in only:
            continue
        out.append(dict(beat))
    return out


def entrance(mapname):
    """Where a player actually arrives on a map: its first warp.

    The middle of the map is fine for a screenshot but wrong for a cutscene --
    the scripts walk the player a fixed number of steps from the door, and from
    anywhere else they walk into a wall and wait for a movement that never
    finishes.
    """
    try:
        j = json.load(open(os.path.join(ROOT, "data", "maps", mapname, "map.json")))
    except OSError:
        return None
    warps = j.get("warp_events") or []
    return (warps[0]["x"], warps[0]["y"]) if warps else None


def build_script(beats, state, maps, outdir, settle, advance):
    lines = ["include %s" % os.path.join(ROOT, "tools", "playtest", "scripts",
                                         "afterintro.txt"),
             "savestate %s" % state]
    for beat in beats:
        group, num = maps[beat["map"]]
        x, y = (beat.get("pos") or entrance(beat["map"]) or sweep.warp_target(
            os.path.join(ROOT, "data", "maps", beat["map"])))
        lines += ["echo BEAT %s" % beat["name"], "loadstate %s" % state]
        lines += beat.get("setup", [])
        lines += [
            "write16 gPlaytestWarp+2 %d" % group,
            "write16 gPlaytestWarp+4 %d" % num,
            "write16 gPlaytestWarp+6 65535",
            "write16 gPlaytestWarp+8 %d" % x,
            "write16 gPlaytestWarp+10 %d" % y,
            "write16 gPlaytestWarp 1",
            # Watch for the map immediately and start the beat the moment it
            # arrives. Settling first lets a short cutscene run and finish
            # unobserved, which reads as a script that never started.
            "untilmap %d %d %d" % (group, num, settle),
        ]
        lines += beat.get("act") or ["advance %d" % advance]
        for kind, name, _ in beat["expect"]:
            lines.append(("var %s" if kind.startswith("var") else "flag %s") % name)
        if beat.get("shot", True):
            lines.append("shot %s" % beat["name"].replace(":", "_").replace("/", "_"))
    return "\n".join(lines) + "\n"


def parse(output, beats):
    """Split the harness's output per beat and judge each one."""
    results, current = {}, None
    for line in output.splitlines():
        m = re.match(r"echo BEAT (\S+)", line)
        if m:
            current = {"name": m.group(1), "reads": {}, "stalled": False,
                       "arrived": True, "crash": False, "never_ran": False}
            results[m.group(1)] = current
            continue
        if not current:
            continue
        m = re.match(r"until (\S+) TIMEOUT", line)
        if m:
            # The until that timed out names itself: `warp` never landed on
            # the map, `script` started the cutscene and never left it.
            if m.group(1) == "warp":
                current["arrived"] = False
            elif m.group(1) == "scriptstart":
                current["never_ran"] = True
            else:
                current["stalled"] = True
        m = re.match(r"pbit (\w+) = (\d+)", line)
        if m:
            current["reads"][m.group(1)] = int(m.group(2))
        m = re.match(r"pread (\w+) = (\d+)", line)
        if m:
            current["reads"][m.group(1)] = int(m.group(2))
        if line.startswith("crash:"):
            current["crash"] = True

    verdicts = []
    for beat in beats:
        r = results.get(beat["name"])
        if not r:
            verdicts.append((beat["name"], "no output"))
            continue
        why = []
        if not r["arrived"]:
            why.append("never reached the map")
        if r["stalled"]:
            why.append("script never finished")
        if r["crash"]:
            why.append("crashed")
        if not why and beat["expect"]:
            met = False
            for kind, name, value in beat["expect"]:
                got = r["reads"].get(name)
                if got is None:
                    continue
                if kind == "var-not":
                    met = met or got != value
                elif kind == "var":
                    met = met or got == value
                else:
                    met = met or got == value
            if not met:
                # A script with no msgbox or movement in it runs to completion
                # inside a single frame, so never being seen in CONTEXT_RUNNING
                # is only evidence of a problem when nothing changed either.
                why.append("%s (%s)" % (
                    "never ran" if r["never_ran"] else "ran but changed nothing",
                    ", ".join("%s=%s" % (n, r["reads"].get(n, "?"))
                              for _, n, _ in beat["expect"])))
        verdicts.append((beat["name"], "; ".join(why)))
    return verdicts


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--rom", default=os.path.join(ROOT, "pokecrystal.gba"))
    ap.add_argument("--map", default=os.path.join(ROOT, "pokecrystal.map"))
    ap.add_argument("--out", default=os.path.join(ROOT, "progression-out"))
    ap.add_argument("--only", nargs="*", help="map or beat names")
    ap.add_argument("--settle", type=int, default=900,
                    help="frames to wait for the warp to land")
    ap.add_argument("--advance", type=int, default=12000,
                    help="frames to let a derived beat's script run")
    ap.add_argument("--derived", action="store_true", help="skip the hand beats")
    ap.add_argument("--hand", action="store_true", help="only the hand beats")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    os.makedirs(args.out, exist_ok=True)
    only = set(args.only) if args.only else None
    beats = []
    if not args.hand:
        beats += derive_beats(only)
    if not args.derived:
        beats += hand_beats(only)
    if not beats:
        sys.exit("progression: nothing to run")
    # Beats the harness cannot judge are named and set aside rather than run
    # and reported as failures.
    skipped = [(b["name"], beats_module.unrunnable(b)) for b in beats
               if beats_module.unrunnable(b)]
    beats = [b for b in beats if not beats_module.unrunnable(b)]

    syms = playtest.load_symbols(args.map)
    state = os.path.join(args.out, "afterintro.state")
    script = build_script(beats, state, map_numbers(), args.out, args.settle,
                          args.advance)
    expanded = playtest.expand_text(script, syms, args.out)
    print("running %d beats..." % len(beats))
    import subprocess
    proc = subprocess.run([playtest.HARNESS, args.rom, "-"], input=expanded,
                          text=True, capture_output=True)
    if args.verbose:
        sys.stdout.write(proc.stdout)
    sys.stderr.write(proc.stderr)

    verdicts = parse(proc.stdout, beats)
    bad = [(n, w) for n, w in verdicts if w]
    print("%d beats run, %d failed" % (len(verdicts), len(bad)))
    for name, why in bad:
        print("  %-52s %s" % (name, why))
    if skipped:
        print("%d beats skipped:" % len(skipped))
        for name, why in skipped:
            print("  %-52s %s" % (name, why))

    try:
        from PIL import Image
    except ImportError:
        return 1 if bad else 0
    for name in sorted(os.listdir(args.out)):
        if name.endswith(".ppm"):
            src = os.path.join(args.out, name)
            Image.open(src).save(src[:-4] + ".png")
            os.remove(src)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
