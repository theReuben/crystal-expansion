# Phase 0 — Baselines

Status: **gate passed.** Both baseline ROMs build, boot, and reach their title
screens. Evidence in `docs/shots/`.

## Pins

| Role | Repo | Pin |
|---|---|---|
| Engine | rh-hideout/pokeemerald-expansion | `expansion/1.17.0` (`e8bd1cd7b0`, 2026-08-31) — latest release tag |
| Content | Deokishisu/CrystalDust `progress` | `5c11a2b`, 2024-03-23 — matches plan |
| Ancestor | pret/pokeemerald | full clone (needed for three-way diffs in Phase 2) |
| Data ref | pret/pokecrystal | shallow |

All four live in `sources/` (gitignored).

## Host toolchain (macOS 15, Apple silicon)

- devkitARM 15.2.0 at `/opt/devkitpro/devkitARM` — **not on PATH by default**
- Homebrew libpng, mGBA 0.10.5, Go 1.x (for poryscript)
- agbcc: built from `pret/agbcc` into `sources/agbcc`, installed into CrystalDust

Environment needed for any build:

```sh
export PATH=/opt/devkitpro/devkitARM/bin:$PATH
export DEVKITARM=/opt/devkitpro/devkitARM DEVKITPRO=/opt/devkitpro
export CPATH=/opt/homebrew/include LIBRARY_PATH=/opt/homebrew/lib
```

## Expansion 1.17.0

`make modern` — clean first try, ~2m50s.

```
EWRAM:    226412 /  256 KB   86.37%
IWRAM:     28384 /   32 KB   86.62%
ROM:    26742500 /   32 MB   79.70%
```

## CrystalDust

Built with **`make modern`**, not agbcc. Four fixes were needed; see
`docs/crystaldust-macos-build.patch`.

1. `tools/mapjson/Makefile` — drop `--static`; Apple's `ld` rejects it.
2. `Makefile` — add `-std=gnu11` to `CFLAGS` (modern branch). gcc 15 defaults to
   C23, where an empty parameter list means `(void)`, so every call to CD's
   K&R-style `GetMonData()` prototype became "too many arguments".
3. `Makefile` — add `-std=gnu11` to `CPPFLAGS` as well. CD preprocesses in a
   separate `$(CPP)` pass before invoking `cc1`, so the flag on `CFLAGS` alone
   left `__STDC_VERSION__` at C23 during preprocessing and devkitARM's
   `stddef.h` then tried to typedef `nullptr_t`.
4. `src/gbs.c:708` — cast to `(u32 *)`. `currentPointer` is a pointer; gcc 15
   promotes `-Wint-conversion` to an error. Semantics unchanged.

Also: poryscript must be **3.0.3** (the version CD's bundled `CHANGELOG.md`
documents), not current 3.6.1 — 3.6.x requires a `command_config.json` that CD
does not ship. Build from `huderlem/poryscript` at tag `3.0.3` and drop the
binary in `tools/poryscript/`.

### The agbcc path does not work

`make` (agbcc, `MODERN=0`) fails: CD's `progress` HEAD contains C99
declaration-after-statement in at least `event_object_movement.c`,
`field_camera.c`, and `field_player_avatar.c`. agbcc is gcc 2.95 and has no
`-std=` that accepts this. The modern path is the supported route here.

Resulting ROM: `CrystalDust.gba`, 16 MB.

## ROM size — measured, and less alarming than it first looks

Expansion 1.17.0 uses 26.7 MB of the 32 MB ceiling (79.7%), which at first
glance leaves almost no room for two regions of content. Breaking the map file
down by object file shows that reading is wrong:

| | expansion 1.17.0 | CrystalDust |
|---|---|---|
| `data/sound_data.o` | 10.47 MB | 2.66 MB |
| `src/pokemon.o` | 5.94 MB | — (split across several) |
| `src/graphics.o` | 0.94 MB | 2.38 MB |
| tilesets | 0.69 MB | 1.03 MB |
| maps | 0.69 MB | 0.63 MB |
| **attributed total** | **25.1 MB** | **12.7 MB** |

Two things follow.

**Sound dominates, and it gets replaced, not added.** Emerald's `sound_data` is
10.5 MB; CrystalDust's is 2.7 MB. CD substitutes the soundtrack rather than
appending to it, so the merge *frees* roughly 7.8 MB rather than consuming any.
The same is true of maps and tilesets — CD replaces Hoenn with Johto/Kanto, and
its `maps.o` is actually slightly smaller than Emerald's.

**The real swing factor is `src/pokemon.o` at 5.94 MB** — expansion ships front
and back sprites, palettes and icons for every species through Gen 9, where
CrystalDust needs 251. That single file is where the section 6 "leave the
post-Gen-3 species toggled off" decision cashes out in bytes.

Rough merged estimate, keeping *all* of expansion's species:

```
26.7  expansion baseline
-7.8  soundtrack swap
+1.4  CD graphics over Emerald's
+0.3  CD tilesets over Emerald's
+0.5  CD-only source
----
~21 MB, roughly 11 MB of headroom
```

So size is a thing to track at each gate, not a thing to design around now, and
the plan's ~28 MB tripwire is the right one. Re-measure after Phase 1, when the
substitutions are real rather than projected.

## Verification harness

`tools/romshot/` builds a headless screenshotter against `libmgba` — runs a ROM
for N frames with scripted button presses and writes a PNG. Homebrew ships only
mGBA's Qt app, so `libmgba` is built from source into `sources/mgba-build`; see
that directory's README. This is how the Phase 3 and Phase 4 checklists get
verified without a human driving an emulator.
