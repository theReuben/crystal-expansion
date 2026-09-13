# Phase 0 — Baselines

Status: both baseline ROMs **build**. Boot confirmation pending (see "Open" below).

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

## ROM size — act on this now

Expansion 1.17.0 alone occupies **26.7 MB of the 32 MB ceiling (79.7%)** with no
CrystalDust content in it at all. The plan's "act if it climbs past ~28 MB"
tripwire is 1.3 MB away at baseline, and Phase 1 adds 27 MB of graphics, 12 MB
of tilesets and 11 MB of sound before any deduplication.

This is not a Phase 4 problem. It is a Phase 1 problem, and it bears directly on
the section 6 decision: expansion's post-Gen-3 species data is a large part of
that 26.7 MB, so "field-merge only, species toggles off" may be forced rather
than chosen.

## Open

- Neither ROM has been **booted**. mGBA on macOS ships only as a GUI `.app`;
  it has no headless or screenshot mode and swallows stdout, so the boot half
  of the Phase 0 gate cannot be automated from here. Needs a human to launch
  both ROMs once.
