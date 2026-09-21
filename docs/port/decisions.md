# Decision log

Running record of constraint decisions. Anything that drops or changes behaviour
from either parent project gets an entry here, with the count of what was
affected. Nothing gets dropped silently.

---

## D1 — Textbox: wide (Emerald style)

**Decided by Reuben, 2026-09-13.** Use expansion's window widths throughout.

### The plan's premise for this was inverted

The plan expected "text overflow across thousands of strings" because
CrystalDust uses FRLG's narrower box. Measured, the widths are:

| window | CrystalDust | expansion |
|---|---|---|
| overworld standard (`sStandardTextBox_WindowTemplates`) | 26 tiles / 208 px | **27 tiles / 216 px** |
| battle message (`B_WIN_MSG`) | **28 tiles** | 26 tiles |

CrystalDust's poryscript config confirms the overworld figure: its default font
is `1_latin_rse` at `maxLineLength=208`, i.e. Emerald metrics, not FRLG's 198.

So in the overworld, going wide makes the box **one tile bigger**. Every one of
CrystalDust's 4,820 hard line breaks (`\n`/`\l` across 405 `.pory` files) still
fits, because they were wrapped for a *narrower* box. There is no overflow risk
in that direction at all — only slightly ragged right margins.

The narrowing is in *battle*, 28 → 26 tiles. Per the plan's section 2a we take
expansion's battle files wholesale, so expansion's battle strings arrive already
matched to its own 26-tile box. The exposure is only CrystalDust-authored battle
text that survives in CD-only files — Bug-Catching Contest messages and similar.

**Consequence: Phase 5 shrinks from "audit 33,000 lines" to "audit
CD-authored battle strings."** Still worth scripting a width detector, but it
should be pointed at battle text, not the whole script corpus.

Note CrystalDust uses **zero** `format()` calls — all wrapping is manual. So
nothing re-wraps for free, and cosmetic tidying of the overworld ragged edge is
a real (if optional, and purely visual) cost.

---

## D2 — Charmap merge: 54 byte-value collisions, must not be unioned blindly

`charmap.txt` maps names to byte sequences. CrystalDust has 1,064 entries,
expansion 1,094.

Good news: **no entry means two different things in the two repos.** Every name
present in both maps to the same bytes (0 conflicts).

Bad news: **54 CrystalDust-only names occupy byte values that expansion-only
names also use.** These are not accidents — they are CrystalDust *renaming
Emerald's slots in place*:

```
AB 01   CD:MUS_AZALEA                expansion:MUS_DEWFORD
95 01   CD:MUS_NEW_BARK              expansion:MUS_LITTLEROOT
7F 01   CD:MUS_ELMS_LAB              expansion:MUS_BIRCH_LAB
...51 more
```

A naive union charmap would leave both names resolving to the same byte. Any
expansion code or script that still plays `MUS_DEWFORD` would silently play
Azalea Town's theme instead. That is precisely the class of silent breakage this
log exists to prevent.

**Decision:** take CrystalDust's charmap as the base for the music ID range,
since CrystalDust's soundtrack replaces Emerald's wholesale (see the size
analysis in `docs/phase0-baselines.md` — that substitution is also what keeps
the ROM under the ceiling). Then sweep expansion's sources for surviving
references to the Hoenn song names and repoint them. Expansion-only song names
with no CrystalDust counterpart keep their IDs.

**Three of the 54 are not music and need real work:**

```
FD 35   CD:B_BUG_CONTEST_MON             expansion:B_ATK_TRAINER_NAME
FD 1A   CD:B_SCR_ACTIVE_ABILITY          expansion:B_SCR_ABILITY
FD 13   CD:B_SCR_ACTIVE_NAME_WITH_PREFIX expansion:B_SCR_NAME_WITH_PREFIX
```

These are battle text-substitution placeholders. Because we take expansion's
`battle_message.c`, `FD 35` will mean "attacking trainer's name". CrystalDust's
Bug-Catching Contest strings that use `B_BUG_CONTEST_MON` would print the wrong
substitution. **These three need fresh IDs allocated and CrystalDust's strings
repointed** — they are a Phase 2 work item, not a copy-and-go.

---

## D3 — agbcc dropped as a build path

CrystalDust's `progress` HEAD does not compile under agbcc (C99
declaration-after-statement in three field files; agbcc is gcc 2.95). The modern
devkitARM path is the only working route, and is what expansion uses anyway.

**Nothing is lost** — this affects only how the reference ROM is built, not what
is in it. Recorded because the plan anticipated running both toolchains.

## D4 — Build with GNU Make 4.x (`gmake`), not Apple's `/usr/bin/make`

**Decision.** Crystal Expansion requires GNU Make 4.0 or newer. On macOS that
means Homebrew's `gmake`. Apple's `/usr/bin/make` (GNU Make 3.81, 2006) is not
supported.

**Why.** After the Phase 1 rule-file merge, `make -j8 modern` under 3.81 hung
indefinitely: 100% CPU, zero child processes, output frozen mid-log. `sample`
on the stuck process showed deep self-recursion in make's implicit-rule search.
3.81's search is exponential in the number of pattern rules; the merge roughly
triples them (`graphics_file_rules.mk` 313 -> 869 lines, `spritesheet_rules.mk`
25 -> 829). GNU Make 4.4.1 runs the identical, unmodified rule set to completion
in 3.8 seconds.

**What this is not.** It is not a defect in the merged rules, and no rules were
removed or simplified to work around it. The duplicate-target warnings
(`map_data_rules.mk:84` vs `:37` for `include/constants/layouts.h`;
`json_data_rules.mk:43` vs `Makefile:254` for `src/data/wild_encounters.h`) are
real and should still be deduplicated in Phase 2, but they were not the cause --
they are warnings, and 4.4.1 builds through them.

**No feature dropped.** This costs a `brew install make` on macOS. Linux
distributions have shipped make 4.x since 2014.

**Diagnostic note for future hangs.** An unexplained make hang with no children
is almost certainly 3.81 implicit-rule search, not a stuck tool. Confirm with
`sample <pid>` and look for self-recursive frames.

## D5 — Attribution: in-game credits deferred to Phase 6; not dropped

**Decision.** The README, `CREDITS.md`, and the RHH attribution string are done
now (Phase 1). The **in-game** credits page is deferred to Phase 6.

**Why the deferral.** `src/credits.c` does not compile yet, and the credits data
is part of the end-game sequence that Phase 6 reworks anyway. Adding a page now
would be written against an API that Phase 2 is about to change.

**Tracked, not dropped.** Phase 6 must add a Crystal Expansion page to
`src/data/credits.h` naming CrystalDust, RHH, and pret. This is a release
blocker: **do not publish a build without it.**

**Finding.** CrystalDust ships no credits file, and its `src/data/credits.h` is
Emerald's staff roll unmodified — CrystalDust never added itself to its own
in-game credits. So there is no upstream contributor list to inherit. Our
`CREDITS.md` is assembled from CrystalDust's README and `alpha_testers.txt` and
is flagged in-file as probably incomplete.

**Also required before publishing** (plan section 7): tell the CrystalDust
developers directly. Not done; it is premature while the project does not build.

## D6 — Hoenn/event-island map scripts: take expansion's version, defer Q3/Q4

**Decision.** For 48 `data/maps/*/scripts.inc` files covering Battle Frontier,
the Battle Tents, Contest Hall, Southern Island, Birth Island, Faraway Island,
Navel Rock, Trainer Hill, and the Mystery Events House, take
pokeemerald-expansion's version rather than the CrystalDust copy the Phase 1
merge had installed.

**Why.** They failed to assemble with `expected symbol name` on lines like
`.set LOCALID_PLAYER, 1`. Expansion promoted `LOCALID_PLAYER` to a global
constant (`include/constants/event_objects.h`, value 255), so the C preprocessor
rewrites the `.set` to `.set 255, 1` before the assembler sees it. CrystalDust's
copies predate that promotion. Diffing confirmed the only differences are
expansion's newer conventions — named vars (`VAR_TEMP_FRONTIER_TUTOR_ID` for
`VAR_TEMP_E`), global local-IDs (`LOCALID_SOUTHERN_ISLAND_LATI`), and the
compressed `goto_if_ne` form. CrystalDust made no gameplay changes here; it
simply never touched this Hoenn content.

**No feature dropped.** This is Hoenn content Crystal has no analogue for, and
CrystalDust does not use it. Taking expansion's version *restores* working
content rather than removing any. The files stay present and building.

**Relationship to Q3/Q4.** This deliberately does not answer whether the Battle
Frontier ships. It removes the question from the critical path: the maps now
assemble, so the scope call can be made later on its merits instead of being
forced by a build error. If Q3 comes back "out of scope", these are deleted
then — as a decision, not as a side effect.

## D7 — Day/night: two complete implementations collide (RESOLVED)

**Status: resolved. No feature dropped, and no new config flag needed — the
exact behaviour CrystalDust had is already an existing expansion preset.**

**What was found.** `include/rtc.h` produced `conflicting types for
'GetTimeOfDay'`. The header is expansion's, unmodified. The conflict is that
*both* projects implement day/night, and they disagree on the signature:

    expansion   enum TimeOfDay GetTimeOfDay(void);      // reads the clock itself
    CrystalDust u8             GetTimeOfDay(s8 hours);  // classifies a given hour

pokeemerald-expansion 1.17 ships a time-of-day system CrystalDust never had. It
is wired into things CD's is not: time-of-day wild encounter tables
(`OW_TIME_OF_DAY_ENCOUNTERS`), the Pokédex (`GetTimeOfDayForDex`), configurable
generational time bands (`GenConfigTimeOfDay`, `OW_TIMES_OF_DAY`), a fake-RTC
debug mode (`OW_USE_FAKE_RTC`), and palette tinting in `field_weather.c` /
`overworld.c`.

CrystalDust's `day_night.c` is a parallel implementation whose distinctive part
is `struct PaletteOverride` — per-slot palette swaps gated on `startHour`/
`endHour`, with a `gPlttBufferPreDN` shadow buffer. That is what gives Johto lit
windows at night and the Crystal-specific palette feel. Expansion's tinting is a
global weather-style blend and does not do per-slot scheduled overrides.

**Recommendation.** Expansion's `GetTimeOfDay` becomes the single source of
truth for *what time it is*, since the encounter, Pokédex, and config wiring all
depend on it and reimplementing that on CD's function would be a large
regression. CrystalDust's `PaletteOverride` layer is kept for *how it looks*,
rewritten to ask expansion's `GetTimeOfDay()` rather than its own clock reader,
and CD's `GetTimeOfDay(s8)` is renamed `ClassifyHourAsTimeOfDay(s8)` to end the
collision.

**What that costs, explicitly.** The two projects divide the day differently.
CrystalDust follows Crystal's bands; expansion's default is `OW_TIMES_OF_DAY`
set per generation. Adopting expansion's boundaries means night falls at
different in-game times than in CrystalDust unless `OW_TIMES_OF_DAY` is tuned to
match Crystal. That tuning is a config change, not a code change, and should be
done deliberately — it is a visible gameplay difference, not a detail.

**Do not** resolve this by deleting `day_night.c`. That would silently drop the
per-slot palette overrides and Johto would lose its night look.


### D7 resolution

The cost I originally flagged — "night falls at different in-game times" — turned
out not to exist. Measuring both sides:

| Preset | Morning | Day | Evening | Night |
|---|---|---|---|---|
| CrystalDust | 4–10 | 10–20 | (none) | 20–4 |
| `OW_TIMES_OF_DAY GEN_2` | 4–10 | 10–18 | (none) | 18–4 |
| `OW_TIMES_OF_DAY GEN_3` | (none) | 12–24 | (none) | 0–12 |
| **`OW_TIMES_OF_DAY GEN_4`** | **4–10** | **10–20** | **(none)** | **20–4** |
| `GEN_6` | 4–11 | 11–18 | 18–21 | 21–4 |
| `GEN_7` | 6–10 | 10–17 | 17–18 | 18–6 |
| `GEN_8`/`GEN_9` (`GEN_LATEST`, the default) | 6–10 | 10–19 | 19–20 | 20–6 |

**CrystalDust's bands are `GEN_4` exactly.** So the fix is a one-line config
change, not a new flag: `OW_TIMES_OF_DAY` is set to `GEN_4`. Expansion's empty
evening band is handled correctly — `IsBetweenHours(h, 0, 0)` is always false,
so `TIME_EVENING` is simply never returned.

Applied:
1. `OW_TIMES_OF_DAY` set to `GEN_4` in `include/config/overworld.h`.
2. CrystalDust's `GetTimeOfDay(s8)` and `GetCurrentTimeOfDay()` deleted;
   expansion's `GetTimeOfDay(void)` is now the single clock reader. There were
   only four call sites.
3. CrystalDust's `PaletteOverride` layer in `day_night.c` is **kept** — it now
   asks expansion's clock. Johto keeps its night palettes.

**A numbering trap worth recording.** `include/constants/day_night.h` (CD) and
`include/constants/rtc.h` (expansion) both defined `TIME_MORNING`/`TIME_DAY`/
`TIME_NIGHT`/`TIMES_OF_DAY_COUNT`, and **they disagree**: CD numbers
`TIME_NIGHT` 2, expansion numbers it 3 and uses 2 for `TIME_EVENING`. CD's were
`#define`s and expansion's an `enum`, so CD's would have textually rewritten
expansion's enumerators. Removed CD's four defines in favour of the enum.

**Follow-up, not yet done.** `src/radio.c:287` does
`Random() % TIMES_OF_DAY_COUNT`, which was 3 and is now 4. Under `GEN_4` the
fourth value (`TIME_EVENING`) never occurs naturally, so a radio show could be
selected for a time of day that never happens. Must be checked when radio.c is
ported (Group B). Noted here so it is not lost.

### Related: duplicate make targets

CD's `wild_encounters.h` rule in `json_data_rules.mk` overrode expansion's
(`Makefile:254`) and required a `wild_encounters.json.txt` that does not exist,
breaking the build once the config change forced a regeneration. Removed CD's
rule: expansion's generator is time-of-day aware and reads
`config/overworld.h`, which CD's jsonproc rule is not.

74 further duplicate targets remain in the appended rule blocks (72 in
`graphics_file_rules.mk`, 2 in `map_data_rules.mk`). They are warnings only and
currently resolve to CrystalDust's rules, which is the intended outcome for
CD-replaced art. Left alone deliberately rather than mass-edited; revisit if one
of them misbuilds.

## D8 — OW_TIMES_OF_DAY set to GEN_LATEST (supersedes the GEN_4 choice in D7)

**Decision (user's).** Follow expansion's latest-by-default convention rather
than matching CrystalDust exactly.

**What changes versus CrystalDust.** Morning starts at 6 rather than 4, day ends
at 19 rather than 20, night runs 20–6, and **an evening band (19–20) exists that
CrystalDust never had**.

**Consequence being tracked.** `TIME_EVENING` is now a reachable state, so
CrystalDust content that assumes three times of day can be reached with a fourth.
The surface turned out to be small — a tree-wide scan found only two places in
CrystalDust code that care:
  - `src/radio.c:287` — `Random() % TIMES_OF_DAY_COUNT`, now 4 rather than 3.
    Under GEN_LATEST evening genuinely occurs, so this is no longer the
    "impossible value" bug noted in D7; it now means radio programming needs an
    evening schedule or a deliberate fallback.
  - `src/radio.c:814` — `GetTimeOfDay() == TIME_MORNING`.
Everything else indexing `TIMES_OF_DAY_COUNT` (`debug.c`, `wild_encounter.h`) is
expansion's and already four-aware.

CrystalDust's `PaletteOverride` entries are keyed on `startHour`/`endHour`, not
on `TIME_*`, so the night palettes are unaffected by the band change.

## D9 — CrystalDust strings were dropped by the Phase 1 merge; 782 restored

**Found.** `src/strings.c` and `include/strings.h` were byte-identical to
expansion's. Both projects had edited them, and the Phase 1 three-way
classification resolved the conflict in expansion's favour, which silently
discarded every string CrystalDust added — including all the radio, Pokégear,
and phone text. This was the single largest silent loss in the merge.

**Restored.** 782 CrystalDust-only strings, appended to `src/strings.c` with
declarations in `include/strings.h`.

**How "CrystalDust-only" was determined.** Not by comparing `strings.h` alone.
A first attempt did that and broke 11 files, because some `gText_` symbols are
defined in expansion's `strings.c` without a header declaration, and others are
declared `static` inside `main_menu.c` and `option_menu.c` — adding an `extern`
for those produced "static declaration follows non-static declaration". The
exclusion set is now built by scanning every `.c` and `.h` under `src/`,
`include/`, and `gflib/` for both definitions and declarations. 53 further
symbols are declared in CrystalDust's header but never defined in its
`strings.c`; those are skipped and listed in the build log rather than faked.

**Not done here.** 934 names exist in both projects. Those kept EXPANSION's
wording, which for Crystal-specific text is likely wrong. Auditing shared
strings against Crystal is the Phase 5 text sweep and is deliberately out of
scope for this merge — flagged so it is not mistaken for finished.

**Lesson for the rest of Phase 2.** Any file the Phase 1 log recorded as
"conflict-kept" may have dropped CrystalDust content the same way. That list is
in `docs/port/phase1-asset-merge.txt` and should be re-audited file by file.

### Known issue, not yet fixed

`data/tilesets/secondary/inside_ship/tiles.png` now builds with
`num_tiles=342`, over the 256 maximum, and fails. This comes from the duplicate
graphics rules noted in D7 (CrystalDust's rule winning over expansion's).
Tracked for the graphics pass.

## D10 — Flag and var space overflows; SaveBlock must grow (DECISION NEEDED)

**Status: open, and blocking radio.c, phone_contact.c, pokegear.c and every
Johto map script. Flagging before acting because it changes the save layout.**

**How this surfaced.** After restoring the dropped strings (D9), `radio.c`'s
remaining errors are all missing *constants*: 46 `TRAINER_*`, 19 `MAPSEC_*`,
10 `MUS_*`, 6 `FLAG_*`, plus Johto `ROUTE*`. Checking the headers showed the
same conflict-kept pattern D9 found in `strings.c` — every one of these is
byte-identical to expansion's, so all of CrystalDust's additions were dropped:

| Header | expansion | CrystalDust-only | shared |
|---|---|---|---|
| `flags.h` | 1883 | **879** | 374 |
| `opponents.h` (`TRAINER_*`) | 856 | **371** | 125 |
| `region_map_sections.h` | (enum) | **214** | 0 |
| `songs.h` | 481 | **59** | 436 |
| `vars.h` | 252 | **52** | 194 |

**The constraint.** Unlike strings, these carry numeric values, and both pools
are near capacity:

    FLAGS   2400 slots, 1883 used, 517 free, 879 needed  -> short by 362
    VARS     256 slots,  252 used,   4 free,  52 needed  -> short by 48

Vars are the harder wall: the range is `VARS_START 0x4000` to `VARS_END 0x40FF`,
exactly 256, and only 4 are free.

**Why this is not simply "copy CrystalDust's values".** CrystalDust's numbering
was assigned against vanilla pokeemerald's much emptier pools. Reusing its
values would collide with flags and vars expansion has since allocated. Every
CrystalDust-only flag and var needs a *fresh* ID in this tree, and its
references rewritten — which is fine, but it is a translation, not a copy.

**Options.**
1. **Grow both pools.** `NUM_FLAG_BYTES` and the var array both live in
   `SaveBlock1`; this is a decomp, so the layout is ours to change. Costs save
   size and must be settled *before* any playtesting, since it invalidates
   saves. Vars additionally need `VARS_END` pushed past `0x40FF`, and script
   macros that range-check var arguments (`setvar`/`copyvar` guards in
   `event.inc`) must move with it.
2. **Reclaim rather than grow.** Expansion allocates flags and vars for Hoenn
   content Crystal will never use (Battle Frontier, contests, secret bases). If
   Q3/Q4 come back "out of scope", several hundred slots free up and the
   shortfall may vanish. This makes D10 dependent on Q3/Q4.
3. Some mix: reclaim what the scope decision frees, grow to cover the rest.

**Recommendation.** Answer Q3/Q4 first, then size the growth to what remains.
Growing the save layout twice is worse than growing it once.

**Do not** resolve this by dropping CrystalDust flags to fit. That silently
removes Johto events, and the failure mode is a script that never fires rather
than a build error.

### D10 addendum — measured: Q3/Q4 do NOT solve the shortfall

The recommendation above ("answer Q3/Q4 first, then size the growth") was wrong,
and is withdrawn. It assumed reclaiming Hoenn content would free enough slots to
matter. Measured against `flags.h` and `vars.h` by name pattern:

| Reclaimable by cutting | Flags | |
|---|---|---|
| Battle Frontier / Tents / Tower / Trainer Hill | 26 | |
| Contests | 12 | |
| Secret bases | 17 | |
| Sevii and event islands | 25 | |
| **Total unique** | **80** | against a **362** shortfall |
| **Vars, total unique** | **30** | against a **48** shortfall |

Cutting *all* of it closes roughly a fifth of the flag gap and leaves vars still
18 short. **Growing the pools is required regardless of how Q3 and Q4 are
answered.**

Consequences:
- **D10 no longer blocks on Q3/Q4.** Grow `NUM_FLAG_BYTES` and push `VARS_END`
  past `0x40FF`, sized for CrystalDust's full 879 flags and 52 vars with
  headroom, and do it once.
- **Q3 and Q4 become pure design questions** — whether the Battle Frontier and
  Sevii belong in a Crystal game — decided on their merits, not forced by
  capacity. Whatever they free is a bonus, not the fix.

Caveat on method: this counts *named* constants matching those subsystems. It
does not account for unnamed gaps or contiguous unused ranges, so the true
reclaim is somewhat higher — but not by the ~4x that would change the
conclusion.

## D11 — Scope answers: Q2, Q3, Q4

**Q2 — visual style: Emerald wins.** Where CrystalDust's FR/LG-leaning presentation
and expansion's Emerald conventions disagree, take Emerald. Rationale: nostalgia
for Emerald's look is stronger, and it is consistent with D1 (the wide Emerald
textbox). This applies to UI chrome, window frames, and menu styling — *not* to
Johto's map art, tilesets, or overworld palettes, which are CrystalDust's own
work and are the point of the project. When a case is ambiguous, it goes in this
log rather than being decided silently.

**Q3 — Battle Frontier: left in place for now.** Not cut, not committed to.
D6 already restored expansion's Battle Frontier scripts so they assemble, so
there is no build pressure to decide. Revisit before release.

**Q4 — Sevii Islands: in scope, to be included.** Treat as content to keep and
make work, not scaffolding to leave dangling.

**Effect on D10.** All three answers are "keep", so nothing is reclaimed. The
flag and var pools must be grown to cover CrystalDust's additions in full. This
confirms the D10 addendum: growth was required regardless.

## D12 -- SaveBlock1 sector budget caps the flag/var pools

**Status:** resolved, but leaves only 48 bytes of slack.

D10 grew the pools to 1024 flags + 512 vars. That built until `src/save.c`
failed with `size of array 'SaveBlock1FreeSpace' is negative` -- the
`STATIC_ASSERT` doing exactly its job.

**Measured**, by bisecting the assert's bound with the pools reverted to
baseline:

| | bytes |
|---|---|
| `sizeof(struct SaveBlock1)` before any growth | 15568 |
| Budget: `SECTOR_DATA_SIZE` (3968) x 4 sectors (`SECTOR_ID_SAVEBLOCK1_START` 1 .. `..._END` 4) | 15872 |
| **Free for both pools combined** | **304** |

CrystalDust's actual need is 879 flags (110 bytes) and 52 vars (104 bytes) =
214 bytes, so the content fits. The D10 sizing did not: it asked for 640.

**Chosen:** size the pools to the need plus modest headroom.

- `NUM_CRYSTAL_FLAGS` 1024 -> 128 bytes (879 used, 145 spare)
- `VARS_END` `0x413F`, i.e. 320 vars -> 128 bytes (52 used, 12 spare)
- Total 256 of 304. **48 bytes, or 384 flags, of slack remain.**

**Rejected:** extending `SECTOR_ID_SAVEBLOCK1_END` past 4 and shifting
`SECTOR_ID_PKMN_STORAGE_START`.

> **Correction (D13):** this paragraph originally called that option an
> "escape hatch ... if a later phase runs out". That was wrong, and the error
> mattered enough to fix in place rather than only note below. The 128 KB flash
> has exactly 32 sectors and every one is spoken for: 2 slots x
> `NUM_SECTORS_PER_SLOT` (14) = 28, plus `SECTOR_ID_HOF_1` (28), `HOF_2` (29),
> `TRAINER_HILL` (30) and `RECORDED_BATTLE` (31). Within a slot, `PokemonStorage`
> needs all 9 of its sectors. There is no free sector to take. Growing
> `SaveBlock1` means cutting a feature, not repartitioning. See D13.

**Nothing is dropped by this decision.** All 879 CrystalDust flags and all 52
vars fit. Every Q3/Q4 scope answer from D11 stands unaffected: Sevii is still in,
and the Battle Frontier is still merely deferred, not cut for space.

**Constraint to carry forward:** the flag and var pools are now effectively
closed. Any future phase wanting more than ~384 additional flags, or any
additional vars beyond the 12 spare, must either reclaim IDs or take the
rejected sector option. Re-measure with the bisect before growing either.


## D13 -- the save budget is now the project's binding constraint

Merging CrystalDust's 371 trainers turned out to cost saved-flag space, not just
constant IDs: `TRAINER_FLAGS_END` is `TRAINER_FLAGS_START + MAX_TRAINERS_COUNT - 1`
and `SYSTEM_FLAGS` follows immediately, so every trainer added shifts the whole
flag pool up by one bit. 371 trainers = 376 slots = 47 bytes, against the 48
bytes D12 had left.

**Everything still fits, and nothing has been dropped.** Achieved by removing my
own over-allocation rather than any content:

| | D12 | now | reason |
|---|---|---|---|
| `NUM_CRYSTAL_FLAGS` | 1024 | 880 | 867 actually used |
| `VARS_END` | `0x413F` (320) | `0x4137` (312) | 51 actually used |
| `MAX_TRAINERS_COUNT_EMERALD` | 864 | 1240 | +371 CrystalDust trainers |
| `TRAINERS_COUNT_EMERALD` | 855 | 1226 | |

Merged so far: 867 flags, 51 vars, 371 trainers, all with fresh IDs.

**Measured after:** `sizeof(struct SaveBlock1)` = 15840 of 15872.
**32 bytes free. That is 256 flags, or 16 vars, or 0 additional trainers.**

### This is a constraint, not a resolved problem

Phases 3-7 (New Bark Town, CrystalDust's TODO, Sevii per D11, expansion
features) have 32 bytes between them. The next phase that needs saved state will
hit this. There is no spare flash sector to take -- see the D12 correction.

**The reserve, when it is needed, is Hoenn-only state that a Johto game does not
use.** Measured from `struct SaveBlock1`:

| member | bytes | note |
|---|---|---|
| `secretBases[SECRET_BASES_COUNT]` | 3200 | Hoenn-only mechanic; 25600 flags' worth |
| `tvShows[TV_SHOWS_COUNT]` | 900 | CrystalDust replaces TV with the radio (`radio.c`) |
| `contestWinners` + `pokeblocks` | 736 | contests |
| `mail[MAIL_COUNT]` | 576 | |

Cutting `secretBases` alone ends the problem permanently and is the obvious
candidate, since it is Hoenn furniture with no Johto counterpart and
CrystalDust never used it.

**Not doing that here.** It is a feature cut, and the standing instruction is
that no feature gets dropped without being surfaced first. Recorded as the
recommendation for when the budget next binds, to be decided then rather than
assumed now.

## D14 -- the merged song table silently pointed 43 songs at the wrong music

Found while merging the sound constants, and worth recording because nothing
about it would have shown up as a build error.

Song IDs are indices into `gSongTable`. Phase 1 took CrystalDust's
`sound/song_table.inc` wholesale but kept expansion's `include/constants/songs.h`.
Both tables have 610 entries, so nothing failed to assemble -- but:

- The 487 songs the two projects share **agree exactly**. Both inherit vanilla
  pokeemerald's ordering, so the whole vanilla range was correct.
- Above that range they diverge. **43 expansion-only songs were missing from the
  table entirely**, and 44 of CrystalDust's 56 new songs sat at indices
  expansion's constants had already assigned to something else.

The symptom would have been wrong music on 43 cues, with a clean build.

**Resolved:** `gSongTable` is expansion's 610 entries again, with CrystalDust's
56 songs appended at indices 610-665 (`0x262-0x299`) and fresh constants in
`songs.h` allocated to match. CrystalDust's own song numbers are not reused, for
the same reason as flags and trainers.

`gGBSSongTable` is kept as CrystalDust wrote it. It is a separate table keyed by
an explicit song_id rather than by position, so it is unaffected. Its `song_gbs`
macro (12-byte entries, versus `song`'s 8) was missing and has been added to
`asm/macros/m4a.inc`; it is purely additive and does not touch expansion's `song`.

**This generalises.** Same failure mode as D9 and D10: a Phase 1 "conflict-kept"
file that took one side wholesale and lost the other's additions without a
diagnostic. Song tables were worse than flags only because the loss is silent at
build time. The re-audit of `docs/port/phase1-asset-merge.txt` that D9 called for
should treat every index-ordered table as high risk.

## D15 -- RESOLVED (see D17): the map section enum is 10 over its u8 ceiling

**Status: resolved by D17, by a route not listed among the options below --
reclaiming Hoenn side-area map sections without deleting their maps.**

Map section IDs share a `u8` with the met-location specials
`METLOC_SPECIAL_EGG` (0xFD), `METLOC_IN_GAME_TRADE` (0xFE) and
`METLOC_FATEFUL_ENCOUNTER` (0xFF), so `MAPSEC_COUNT` must be <= 253.

| | count |
|---|---|
| expansion's existing map sections | 209 |
| CrystalDust-only, after removing 4 unused stubs | 53 |
| `MAPSEC_NONE` | 1 |
| **total** | **263** |
| **ceiling** | **253** |
| **over by** | **10** |

CrystalDust's `MAPSEC_SEVII_ISLE_6` through `_9` were dropped to get to 53.
That is not a loss: they sit at (0,0) with no position, and expansion already
ships a complete and far more detailed Sevii set (One through Seven Island,
the isle paths, meadows and ports). Per D11 Sevii is in, via expansion's.

Nothing else reclaimable was found. Battle Frontier, Trainer Hill and Secret
Base account for only 3 map sections between them, so even cutting the content
D11 defers does not close the gap.

### Options

**(a) Merge Johto sub-areas into their parents.** `MAPSEC_ALPH_CHAMBERS` into
`MAPSEC_RUINS_OF_ALPH`, `MAPSEC_TIN_TOWER` and `MAPSEC_BURNED_TOWER` into
`MAPSEC_ECRUTEAK_CITY`, and so on. About 11 candidates exist, so this closes
the gap exactly. Cost: those locations stop showing their own name on the
region map, the map-name popup and the summary screen's met location. No
gameplay change. Reversible.

**(b) Widen `mapsec_u8_t` to `u16`.** Removes the ceiling permanently.
`include/gametypes.h` provides the typedef for exactly this purpose, but warns
against it: met location sits inside every Pokemon, Pokemon substructs are
exactly 12 bytes, and going wider needs the substructs rearranged to avoid
overflowing. `PokemonStorage` already occupies all 9 of its sectors with no
slack (see D13), and it changes the trade/link data format. Expensive and
invasive, but it is the only option that actually scales.

**(c) Cut ~10 Hoenn map sections.** Real content loss in a region we are
shipping. Not recommended.

**Recommendation: (a).** It costs only location labels on a handful of Johto
interiors, fits exactly, is reversible, and leaves the save and trade formats
alone -- which matters given D13 left 32 bytes. (b) is the right answer if
later phases need many more map sections, but it should be a deliberate
save-format change made once, not something done to win 10 slots.

## D16 -- reclaim save space via expansion's FREE_* toggles, not by cutting content

Supersedes D13's warning that the save budget was nearly exhausted, and
supersedes its recommendation to cut `secretBases`.

`include/config/save.h` already provides 13 `FREE_*` toggles for exactly this
purpose, all shipped `FALSE`. They `#if` the fields out of `struct SaveBlock1`
and are supported, tested paths -- not a hand-rolled cut. Ten are now `TRUE`:

| toggle | what it drops |
|---|---|
| `FREE_MYSTERY_EVENT_BUFFERS` | ramScript, e-Reader / Mystery Event |
| `FREE_RECORD_MIXING_HALL_RECORDS` | record mixing |
| `FREE_MYSTERY_GIFT` | Mystery Gift |
| `FREE_UNION_ROOM_CHAT` | Union Room chat |
| `FREE_BATTLE_TOWER_E_READER` | Battle Tower e-Reader |
| `FREE_LINK_BATTLE_RECORDS` | link battle records |
| `FREE_EXTRA_SEEN_FLAGS_SAVEBLOCK1` | unused Pokedex seen flags |
| `FREE_ENIGMA_BERRY` | e-Reader Enigma Berry |
| `FREE_TRAINER_HILL` | Trainer Hill |
| `FREE_POKEMON_JUMP` | Pokemon Jump |

**Measured: `sizeof(struct SaveBlock1)` 15840 -> 13528. Free space 32 -> 2344
bytes.** No new build failures.

**What this costs.** These are link-cable, e-Reader and Mystery Gift features.
They are genuinely dropped, not merely hidden, so this is recorded as a cut --
but none is single-player Johto content and every one is a one-line revert.

**`FREE_MATCH_CALL` (104 bytes) was deliberately NOT set**, even though it looks
like an obvious candidate. CrystalDust's Pokegear phone is still being merged
(`phone_contact.c`, `pokegear.c`) and may build on match call's rematch data.
Revisit once the phone system compiles.

**`secretBases` (3200) and `tvShows` (900) are NOT cut.** D13 recommended them
as the reserve; they are no longer needed, so they stay. If a later phase needs
another kilobyte, they remain the next candidates.

**This does not affect D15.** The map section ceiling is a `u8` value-space
limit, not a save-size limit. Freeing save bytes cannot raise it.

## D17 -- Johto map sections merged; Hoenn side-areas folded into their parents

Resolves D15. User's call: "take the small slice now, revisit after phase 3",
on the reasoning that Hoenn is expansion's content and we want Crystal's.

**Implemented by retargeting, not deleting.** The goal was 10 map section IDs,
not the removal of maps. Ten Hoenn side-areas were repointed to their parent
map section, which frees the ID while leaving every map fully playable and no
warp dangling:

| dropped map section | maps now report | maps |
|---|---|---|
| `MAPSEC_AQUA_HIDEOUT` | `MAPSEC_LILYCOVE_CITY` | 6 |
| `MAPSEC_MAGMA_HIDEOUT` | `MAPSEC_ROUTE_112` | 8 |
| `MAPSEC_MIRAGE_TOWER` | `MAPSEC_ROUTE_111` | 4 |
| `MAPSEC_TRAINER_HILL` | `MAPSEC_ROUTE_111` | 7 |
| `MAPSEC_ARTISAN_CAVE` | `MAPSEC_BATTLE_FRONTIER` | 2 |
| `MAPSEC_DESERT_UNDERPASS` | `MAPSEC_ROUTE_114` | 1 |
| `MAPSEC_ALTERING_CAVE` | `MAPSEC_ROUTE_103` | 1 |
| `MAPSEC_UNDERWATER_105/125/129` | their routes | 3 |

**The only cost is cosmetic:** those areas no longer show their own name on the
region map, the map-name popup, or a caught Pokemon's met location. No map, warp,
script, encounter or item was removed. Every one is reversible.

`MAPSEC_MARINE_CAVE`, `MAPSEC_TERRA_CAVE` and `MAPSEC_UNDERWATER_MARINE_CAVE`
were deliberately left alone -- they drive the roaming Groudon/Kyogre weather
system, and that is not worth disturbing for IDs we did not need.

Also dropped, at no cost: CrystalDust's `MAPSEC_SEVII_ISLE_6..9` (unpositioned
stubs at 0,0; expansion ships a complete Sevii set, per D11) and its
`MAPSEC_ROUTE_3_FLYDUP` / `ROUTE_10_FLYDUP` (Kanto duplicates of map sections
expansion already has). CrystalDust's Johto `ROUTE_32_FLYDUP` is kept.

**Result: 55 Johto map sections merged. `MAPSEC_COUNT` is 252 against a ceiling
of 252 -- exactly full, zero headroom.** The next map section needs another
reclaim; Marine/Terra Cave are the obvious next three.

### Two latent Phase 1 failures this uncovered

Both were invisible until the map JSON edits forced a regeneration.

**1. CrystalDust's map rules in `map_data_rules.mk` overrode expansion's** and
called `mapjson` with the vanilla-era signature (no output directories), which
expansion's mapjson rejects outright. Removed. Identical to the duplicate
`wild_encounters.h` rule from D7.

**2. 181 `local_id` declarations were lost from 58 map.json files.** Phase 1
took CrystalDust's map.json for shared maps, and CrystalDust predates
expansion's named object-event locals -- so `LOCALID_FARAWAY_ISLAND_MEW`,
`LOCALID_CONTESTANT_1`, `LOCALID_FRONTIER_NURSE` and others simply vanished.
This did not fail the build until now only because the generated
`map_event_ids.h` was stale.

Repaired in two passes: 100 declarations injected field-by-field where the
event was provably the same one (matching graphics id and coordinates), and the
remaining 32 maps -- all Battle Frontier, Battle Tent, Contest Hall and event
island harbours -- restored from expansion wholesale under D6, then retargeted
again.

**Third instance of the same Phase 1 pattern**, after D9 (strings), D10
(constants) and D14 (song table). The re-audit of
`docs/port/phase1-asset-merge.txt` is now clearly not optional.

### Also fixed here

`MapHasSpecies` in `pokedex_area_screen.c` identified Altering Cave by map
section. Altering Cave now reports Route 103's, which would have false-positived
on the real Route 103, so the check now tests the map id directly. This is
strictly more correct than the original.

## D18 -- Pokenav: CrystalDust's 10 files were stale duplicates, not content

Phase 1 left the Pokenav subsystem as 23 source files: expansion's 13, plus 10
of CrystalDust's under pret's **older** naming (`pokenav_conditions_1/2/3.c`,
`pokenav_match_call_1/2/ui.c`, `pokenav_menu_handler_1/2.c`,
`pokenav_ribbons_1/2.c`). pret later split and renamed these; vanilla
pokeemerald and expansion both use the 13-file layout.

So the tree held two incompatible revisions of one subsystem compiled together.
The CrystalDust-named files accounted for roughly 350 of the remaining errors,
all from calling an older API against expansion's `include/pokenav.h`.

**Verified duplicates before removing:** 289 function definitions appear in both
sets under identical names. Of the 118 that appear only in the older files, 117
are absent from current vanilla pokeemerald too -- consistent with having been
renamed by pret rather than added by CrystalDust. Decisively, **nothing outside
those 10 files calls any of them**, so no caller was orphaned.

The 10 files are deleted. Errors 807 -> 355, files 34 -> 24.

CrystalDust does not use Pokenav: it replaces it with the Pokegear
(`pokegear.c`, `phone_contact.c`), which is merged separately and does not go
through these functions.

## D19 — 125 CrystalDust trainers were silently aliased onto Emerald trainers

**Found while merging the Pokegear phone system.** `include/constants/opponents.h`
had 496 CrystalDust trainer names and 855 Emerald ones, but only 371 new IDs were
allocated in the Phase 1 constants merge. The other **125 CrystalDust trainers
shared a name with an unrelated Emerald trainer and were unified by name**, so
every CrystalDust reference to them pointed at a Hoenn trainer. No build error;
the symbol resolved.

Example: CrystalDust's Fisher Wilton (Route 44, 2 rematches, ID 188) collapsed
onto Emerald's Fisherman Wilton (Route 111, 4 rematches, ID 78).

**Resolution:** the 125 collided names get their own IDs, suffixed `_GSC`
(IDs 1226-1350). `TRAINERS_COUNT_EMERALD` 1226 -> 1351, `MAX_TRAINERS_COUNT_EMERALD`
1240 -> 1365. 165 references retargeted across 61 Johto/Kanto `scripts.pory`
files plus `phone_contact.c` and `radio.c`. Nothing dropped.

Cost: +125 trainer flags = ~16 save bytes. Budget remains comfortable.

### D19a — CrystalDust rematch table restored

`include/constants/gym_leader_rematch.h` survived Phase 1 as CrystalDust's copy
but **nothing included it** -- expansion replaced that header with the
`REMATCH_*` enum in `rematches.h`, so CrystalDust's 24 phone-rematch trainers
(Joey, Wade, Liz, Ralph, ...) existed nowhere in the build.

- The 24 entries are inserted into `rematches.h` **before**
  `REMATCH_SPECIAL_TRAINER_START`, so they count as normal trainers.
  `REMATCH_TABLE_ENTRIES` 83 -> 107.
- `REMATCH_WILTON` collided the same way as the trainer constant; CrystalDust's
  is now `REMATCH_WILTON_GSC`. Both rows exist in `gRematchTable`.
- `struct RematchTrainer` regains CrystalDust's `phoneContactId` field. Emerald
  rows get `PHONE_CONTACT_NONE` via the existing `REMATCH()` macro; CrystalDust
  rows use a new `REMATCH_PHONE()`. ROM-only, no save cost.
- `MAX_REMATCH_ENTRIES` 100 -> 112 (must exceed 107). +12 save bytes.
- `struct PhoneContact.trainerId` / `.rematchTrainerId` widened `u8` -> `u16`:
  CrystalDust trainer IDs now start at 855 and truncated to garbage.
- Orphan `include/constants/gym_leader_rematch.h` deleted.

### D19b — map constant convention drift (no content impact)

CrystalDust's `MAP_NUM(X)` / `MAP_GROUP(X)` took a bare map name; expansion's
take the `MAP_`-prefixed constant. 100 call sites in `phone_contact.c`,
`radio.c`, `day_night.c` and `bug_catching_contest.c` were rewritten to pass
`MAP_<NAME>`. Every referenced map resolved; none missing.

**Open:** CrystalDust's `match_call.c` rewrite (the `SelectMatchCallMessage_*`
family and `IsMatchCallRematchTime`) was lost the same way -- our `match_call.c`
is expansion's. That merge is the next step, and settles `FREE_MATCH_CALL`.

## D20 — CrystalDust's Pokegear phone system (match_call) restored

Phase 1 kept expansion's `src/match_call.c` **byte for byte**, discarding
CrystalDust's 2738-line rewrite: 39 functions, the entire Pokegear call flow,
mass-outbreak calls, Mom's shopping, and the 26-entry `gMatchCallTrainers`
dialogue table. Re-merged three-way against pokeemerald as the common ancestor
(16 conflicts).

**Conflict policy applied:**
- *Call-window pipeline* -> **CrystalDust**. My first pass took expansion's
  hunks here on D2 ("let Emerald win visually") and produced incoherent code:
  the two sides use different windows, tasks and task data. D2 governs the
  overworld/textbox style, not this screen, which has no Emerald counterpart
  in use. Expansion's `RedrawMatchCallTextBoxBorder` is retained (`src/menu.c`
  calls it) and repointed at CrystalDust's window.
- *Trainer party access, Pokedex rating, `FREE_MATCH_CALL` accessors* ->
  **expansion** (modern APIs; CrystalDust's used structures that no longer exist).
- *Everything else* -> both sides kept.

**Collateral losses found and fixed in the same sweep** (all the same Phase 1
pattern):
- `data/text/match_call.inc` was expansion's 2954 lines; CrystalDust's is 5938.
  Expansion never modified this file, so the merge was clean.
- `include/strings.h` was missing **779** CrystalDust externs. 69 further
  candidates were *excluded* because expansion defines those symbols `static`
  in its own sources -- adding them breaks the build, and silently shadowing
  them would be the D19 name-unification trap again.
- `src/graphics.c` / `include/graphics.h` were missing **154** CrystalDust
  graphics declarations. Two are skipped because the assets are absent:
  `graphics/interface/hp_numbers.4bpp.lz` and
  `graphics/pokemon/question_mark/footprint.1bpp`. **Not yet investigated.**
- `struct MapHeader` lost CrystalDust's `phoneService` flag, and `mapjson`
  lost its emitter, though every `map.json` still carries `phone_service`.
  Restored end to end: struct bit, `map_header_flags` macro argument
  (optional, defaults FALSE), and the `mapjson` emitter.

**API drift resolved:** `ScriptContext2_Enable/Disable` ->
`LockPlayerFieldControls`/`UnlockPlayerFieldControls`; `EnableBothScriptContexts`
-> `ScriptContext_Enable`; `sub_808BCF4` -> `StopPlayerAvatar`;
`gBirchDexRatingText_*` -> `gPokedexRatingText_*`; `gSpeciesNames[]` ->
`GetSpeciesName()`; wild-encounter fields now go through
`encounterTypes[timeOfDay]`. `GetTotalMinutes()` ported into `src/rtc.c`.

### D20a — CrystalDust's three time periods vs GEN_LATEST's four

CrystalDust authored phone dialogue for morning/day/night. D8 set
`OW_TIMES_OF_DAY GEN_LATEST`, which adds **evening**. Rather than drop
evening or leave the switches non-exhaustive, **evening falls through to
night**, matching GSC, where dialogue changed at dusk. Applied to both
time-of-day switches in `match_call.c`; the same rule is owed to `radio.c`
(`radio.c:287`, `radio.c:814`) and any other CrystalDust three-period switch.

### D20b — rematch flag vs rematch stage (same save field)

CrystalDust repurposed `SaveBlock1.trainerRematches[]` as a **bitfield**
(one bit per trainer, "wants a rematch"). Emerald stores a **rematch stage
0-4 per entry** in the same field. Emerald's is strictly more information, so
it is kept, and CrystalDust's `CheckRematchTrainerFlag` / `SetRematchTrainerFlag`
are implemented on top of it. No behaviour lost on either side, no extra save cost.

### D20c — new SaveBlock1 fields

`bankedMoney` (Mom's savings), `gameBuild`, `saveBlockMagic`, and
`roomDecorInventory` (`struct RoomDecor`, 0x18) added: **36 bytes**.
CrystalDust's three mass-outbreak fields (`outbreakSpecialLevel1`,
`outbreakWildState`, `outbreakSpecialLevel2`) cost **nothing** -- they reuse
Emerald's existing padding at 0x2B95/0x2B96/0x2BA0, which is what CrystalDust
did originally. `SaveBlock1FreeSpace` still passes.

### D21 — Bug-Catching Contest: NPC level rolls ignore the player's lead ability

CrystalDust's `ChooseWildMonLevelWithAbility(wildMon, useAbility)` has no counterpart
in expansion; expansion's `ChooseWildMonLevel` *always* applies the Hustle / Vital
Spirit / Pressure max-level boost and exposes no opt-out. CrystalDust deliberately
passed `useAbility = FALSE` when generating the contest NPCs' catches.

Rather than change the shared encounter function (which would alter every wild
encounter in the game), a static `ChooseContestWildMonLevel` reproducing the plain
min/max roll now lives in `src/bug_catching_contest.c`. Behaviour is identical to
CrystalDust. No feature dropped.

Also this gate: `B_OUTCOME_NO_PARK_BALLS` (11) added to `include/constants/battle.h`;
`GetTotalSeconds` added alongside D20's `GetTotalMinutes` in `src/rtc.c`;
`IsPlayerDefeated` un-`static`'d in `src/battle_setup.c` and declared in the header,
since CrystalDust's version was identical to expansion's.

### D22 — PC item storage screen (`item_pc.c`)

CrystalDust's FRLG-style PC item screen is a file expansion has no counterpart for, so
it was kept whole and its ~25 pret-era API calls retargeted onto expansion's names
(`ItemId_GetName` → `GetItemName`, `MenuHelpers_LinkSomething` → `MenuHelpers_IsLinkActive`,
`ListMenuSetUnkIndicatorsStructField` → `ListMenuSetTemplateField`, the insert-indicator
bar → expansion's item-menu swap line, and so on). Four calls needed judgement:

- **`ResetItemMenuIconState()` dropped.** It memset CrystalDust's
  `sItemMenuIconSpriteIds` table; expansion's `item_menu_icons.c` no longer keeps that
  state, so there is nothing to reset. No behaviour change.
- **`unused_ItemPc_AddTextPrinterParameterized` deleted.** It was already dead code in
  CrystalDust and was the sole user of `FONTATTR_STYLE`, which expansion removed along
  with the printer's `style` field.
- **`gItemPcBgPals` switched from `bg.gbapal.lz` to the uncompressed `bg.gbapal`.**
  Expansion dropped `LoadCompressedPalette` entirely; every palette is now loaded
  uncompressed. Costs 16 bytes of ROM. Same pixels.
- **`gPCText_Give` is `static` inside expansion's `pokemon_storage_system.c`** — the D19
  name-unification trap again. A local `sItemPcText_Give` with identical text is used
  rather than promoting the storage-system string to a global.

`CB2_PartyMenuFromItemPC` was ported into `party_menu.c`; `PARTY_ACTION_GIVE_PC_ITEM`
already existed there (expansion marks it "Unused" — CrystalDust uses it).

### D23 — stale pret-era duplicates removed; D20 match_call collateral repaired

Three more Phase-1 "kept one side" leftovers, all pure duplicates rather than feature
losses — each was superseded by an expansion file that already does the same job:

- **`src/unk_text_util_2.c` deleted.** CrystalDust's standalone braille font (`Font6Func`,
  `GetGlyphWidthFont6`). Expansion implements `FONT_BRAILLE` inside `text.c`; nothing in
  the tree referenced CrystalDust's copy.
- **`src/mevent_{client,news,scripts,server,server_helpers}.c` and their headers deleted.**
  pret renamed `mevent_*` to `mystery_gift_*` after CrystalDust forked; expansion carries
  the renamed versions (plus `wonder_news.c`), which we already have. The `mevent_*` files
  were self-referential and included a `mevent.h` that the merge never brought across.
- **Four `gMon*_CircledQuestionMark` definitions removed from `src/graphics.c`.** These
  were my own D20 error: expansion already defines them in `src/data/graphics/pokemon.h`,
  as `u16`, from `.png` rather than `.lz`.

D20's `match_call.c` rebuild also dropped three functions expansion's Pokenav still calls.
Restored:

- **`IsMatchCallTaskActive`** — expansion asks this to choose the Pokenav-styled dialogue
  frame and name box. Expansion tested `FuncIsActiveTask(ExecuteMatchCall)`; CrystalDust
  has no such task, so it now returns `PhoneScriptContext_IsEnabled()`, the native
  equivalent. CrystalDust's static `LoadMatchCallWindowGfx(u8 taskId)` was renamed
  `MatchCallTask_LoadWindowGfx` to free the name for expansion's 3-argument version.
- **`StartMatchCallFromScript`** keeps CrystalDust's `(script, callerId)` signature;
  expansion's one-argument `pokenavcall` path passes `PHONE_CONTACT_NONE`, which is never
  dereferenced because `triggeredFromScript` short-circuits the contact lookup.
- **`SelectMatchCallMessage`** keeps CrystalDust's 4-argument signature; the Pokenav list
  screen passes `FALSE, NULL`.

### D24 — small-subsystem sweep: Johto Dex, Mom's Bank, Buena, apricorns, card flip, TV

Cleared nine small CrystalDust files. Mostly the API renames already established
(`LoadThinWindowBorderGfx` → `LoadStdWindowGfx`, `gSpeciesNames[x]` → `GetSpeciesName(x)`,
`gMoveNames[x]` → `GetMoveName(x)`, `gTypeNames[x]` → `gTypesInfo[x].name`,
`SPRITE_INVALID_TAG` → `TAG_NONE`, `TEXT_SPEED_FF` → `TEXT_SKIP_DRAW`, `CreateMonIcon`
losing its trailing argument, and the list-menu `itemPrintFunc` losing its `index`
parameter). Six decisions worth stating:

- **`GetJohtoPokedexCount` written fresh rather than ported.** CrystalDust's
  `gJohtoToNationalOrder` table in `pokemon.c` is mislabelled — its contents are the
  *Hoenn* dex order, starting at Treecko, left over from an incomplete rename. The real
  Johto Dex is National #1-251 in national order, so no reordering table is needed and
  the new function indexes the national dex directly. This is more correct than what
  CrystalDust shipped. `GetRegionalPokedexCount` now returns the Johto count on the
  non-FRLG path, since this is a Johto game.
- **`ChangeBcdDigit` restored to `src/util.c`** (Mom's savings-account digit spinner);
  it was lost with the rest of CrystalDust's `util.c` additions. `ConvertBcdToBinary`
  already existed in `rtc.c`; `mom_bank.c` only needed the include.
- **`APRICORN_COUNT 7` restored** to `constants/apricorn_tree.h`. It lived in
  CrystalDust's `constants/items.h`, so it went down with D10.
- **`charmap.txt` lost 47 of expansion's entries** (another "kept one side" case, now
  confirmed for charmap too). Three battle placeholders are actually referenced and were
  restored: `B_SCR_NAME_WITH_PREFIX`, `B_SCR_ABILITY`, `B_ATK_TRAINER_NAME`. The other 44
  are Hoenn `MUS_*`/`SE_*` song aliases that CrystalDust renamed; nothing in our text
  references them, so they are left out pending the D14 song-table follow-up.
- **`src/reset_save_heap.c` deleted.** Its only function, `sub_81700F8`, had no callers —
  CrystalDust's `intro.c` called it, but ours is expansion's. It also depended on
  `sub_815355C`, another symbol the merge never brought across.
- **`data/layouts/layouts.json` took CrystalDust's side wholesale** — 419 layouts versus
  expansion's 785, so all 580 Hoenn layouts are absent while `data/maps/SSTidal*` and
  friends still exist. This matches where the Hoenn decision is heading, but the tree is
  currently inconsistent. The immediate consequence was `tv.c` referencing
  `LAYOUT_SS_TIDAL_*`; those three case labels are commented out, not deleted, so the
  full-Hoenn-removal review after Phase 3 can settle it either way in three lines.

### D25 — metatile_labels.h pointed at the wrong tilesets (62 wrong values, 86 missing)

Fixing `fruit_tree.c` turned up the worst silent loss so far, and it is the D14
song-table failure mode exactly: **`include/constants/metatile_labels.h` took
expansion's side wholesale (925 labels, byte-identical to expansion's) while 86 of the
tileset binaries under `data/tilesets/` are CrystalDust's.** Labels and tiles had
drifted apart with nothing to catch it — every one of these compiles, links and runs,
it just addresses the wrong metatile.

Scope, measured rather than estimated:

- **62 labels had values that disagreed with the tileset we actually ship, and all 62 are
  referenced by live code.** Door animations (`General_Door`, `General_Door_Gym`,
  `Door_PokeCenter`, and the Battle Arena / Dome / Palace / Frontier doors), tree
  overlays (`General_Grass_Tree*`, `General_TallGrass_Tree*`), the Pokemon Center
  escalator's 22 animation frames, the Sealed Chamber braille entrance, Shoal Cave's
  dirt and blue stones, and the PC on/off tiles. `General_Door` alone was off by 0x1C.
- **86 CrystalDust labels were missing entirely, 14 of them referenced by live code** —
  the Ruins of Alph puzzle holes, the Radio Tower floors, the Goldenrod Underground
  doors, and the fruit-tree tops that started this.

Every conflicting label was traced to its tileset directory and every one of those
tilesets is CrystalDust's, so **CrystalDust's value wins for all 62**, and the 86
CrystalDust-only labels are appended. Expansion-only labels (the FRLG tilesets, which we
do ship for Sevii per Q4) are untouched. The three ambiguous Kanto city door labels
(`PewterCity`, `SaffronCity`, `ViridianCity`) exist in both a CrystalDust and an FRLG
tileset; `data/tilesets/headers.inc` was followed through to `metatiles.inc` to confirm
`gTileset_ViridianCity` and friends resolve to CrystalDust's `viridiancity` directory,
not expansion's `viridian_city_frlg`, so those take CrystalDust's values too.

The fruit/apricorn tree background event was also restored end to end, since it existed
in none of the four places it needs to: `BG_EVENT_FRUIT_TREE` in `constants/event_bg.h`,
`berryTreeId` in `struct BgEvent`'s union, the `bg_fruit_tree_event` macro in
`asm/macros/map.inc`, and a `fruit_tree` case in `tools/mapjson/mapjson.cpp`. 23 map.json
files use it.

**This warrants a wider audit.** `metatile_labels.h` and `layouts.json` (D24) are both
files where the merge picked a side that contradicts the binary assets sitting next to
them. Any other header that indexes into an asset the other side supplied is suspect.

## D26 — Headbutt restored end to end

`src/fldeff_headbutt.c` shipped from CrystalDust but nothing it depended on
survived Phase 1. Restored, in dependency order:

- `MB_HEADBUTT_TREE` — CrystalDust's value is `0x04`, which in Emerald is
  `MB_UNUSED_04`, referenced by nothing. **No collision**; the behaviour byte in
  our tilesets already means "headbutt tree", so taking 0x04 is required, not
  merely convenient.
- `MetatileBehavior_IsHeadbuttTree` in `src/metatile_behavior.c`.
- `TREEMON_SCORE_BAD/GOOD/RARE` and the three headbutt prototypes in
  `include/fldeff.h`; `GetPlayerTrainerIdOnesDigit` declared in
  `include/field_specials.h` (it was defined but undeclared).
- `FLDEFF_USE_HEADBUTT` allocated as **82** (CrystalDust used 67; expansion's
  table already runs to 81) with a matching `gFieldEffectScript_UseHeadbutt`
  and pointer-table entry.
- `EventScript_HeadbuttTree` / `EventScript_UseHeadbutt` and their text in
  `data/scripts/field_move_scripts.inc`, externs in `include/event_scripts.h`,
  `def_special HeadbuttTreeWildEncounter` in `data/specials.inc`.
- `PartyHasMonWithHeadbutt` in `src/field_player_avatar.c`, and the A-press hook
  at the top of `GetInteractedWaterScript` (CrystalDust's own location).
- `BATTLE_TYPE_TREE` given bit 30, previously the unused `BATTLE_TYPE_30`, and
  `BattleSetup_StartWildBattleFromTree()` to set it — expansion's
  `BattleSetup_StartWildBattle` takes no flags argument.
- Encounter side: `headbuttMonsInfo` added to `struct WildEncounterTypes`,
  `WILD_AREA_HEADBUTT` to `enum WildPokemonArea`, a `headbutt_mons` field to
  `src/data/wild_encounters.json` (expansion's generator is schema-driven, so
  this is all the plumbing needed), plus `ChooseWildMonIndex_Tree`,
  `GenerateHeadbuttWildMon` and `HeadbuttTreeWildEncounter` in
  `src/wild_encounter.c`, including GSC's asleep-species lists.

### Constraints accepted (not silent)

1. **The 12-slot chance table is hardcoded** in `wild_encounter.c` rather than
   generated. CrystalDust's jsonproc template emitted
   `ENCOUNTER_CHANCE_HEADBUTT_MONS_*`; expansion's Python generator does not
   emit per-field chance tables at all. Values are unchanged (50/15/15/10/5/5).
2. **Two cosmetic BATTLE_TYPE_TREE behaviours are not yet ported**: the mon
   sprite dropping out of the tree (`battle_main.c`) and the "fell out of the
   tree!" intro string (`battle_message.c`). The flag is set, so both are a
   later patch in the battle files, not a redesign.
3. **No map has headbutt encounter data yet** — see D27. Headbutt works, trees
   respond, but every tree is empty until the Johto encounter tables land.

## D27 — OPEN: `wild_encounters.json` is expansion's, Johto's is missing

Discovered while wiring D26. `src/data/wild_encounters.json` holds **240 maps,
all Hoenn/Kanto** — expansion's file taken wholesale in Phase 1. CrystalDust's
**124 Johto maps** are gone; only 39 names overlap, and those overlaps are
almost certainly Kanto maps carrying Hoenn-era data.

This is the eighth confirmed instance of the Phase 1 one-side merge, and the
largest by gameplay impact: **there are currently no Johto wild encounters at
all**, headbutt or otherwise. 22 of CrystalDust's maps carry `headbutt_mons`.

Not fixed in D26 because it is a data merge of its own size, and it interacts
with the open question of removing Hoenn's 504 maps (deferred to after Phase 3).
Tracked as the next data task.

## D28 — Phone script context compiles

`src/phone_script.c` (CrystalDust's second script VM, for phone calls) needed:

- `sScriptConditionTable` re-braced — GCC 15's `-Werror=missing-braces`.
- `gScriptStringVars[i]` → `GetStringVar(i)`, expansion's accessor (4 sites).
- `IsFirstTrainerIdReadyForRematch` un-`static`'d in `src/battle_setup.c` and
  declared in the header. It already existed and is already `FREE_MATCH_CALL`
  aware, so nothing was duplicated.
- `CreatePhoneYesNoMenu` ported into `src/menu.c`. Rather than copying the
  function, `CreateYesNoMenuAtPos`'s body was lifted into a new static
  `CreateYesNoMenuWithFrame(..., u8 type)` and both entry points now call it —
  CrystalDust's own structure. `YESNO_STANDARD` keeps the existing behaviour
  byte for byte, so no existing Yes/No box changes.

This is the first piece that makes D20's `FREE_MATCH_CALL == FALSE` decision
pay off: the phone VM now builds against the retained match-call save data.

Errors 86 -> 79.

## D29 — GBS playback: engine-level fields restored

`src/gbs.c` is CrystalDust's GBS player, which plays GSC's original Game Boy
sound data directly rather than re-arranged m4a tracks. It needs three things
from the sound engine that Phase 1 dropped, all restored to match CrystalDust:

- `struct MusicPlayerInfo.gbsTempo` (u16, after `fadeOV`), reset to `0x100` in
  `MPlayStart`.
- `struct MusicPlayerTrack.gbsIdentifier` — CrystalDust splits the existing
  `u8 patternLevel` into two 4-bit fields. **Constraint accepted:**
  `patternLevel` is now capped at 15. It is a pattern-nesting depth that the
  engine never drives above 3, so this is safe, but it is a real narrowing.
- `gUsedCGBChannels` (u8, in `src/m4a.c`) — a bitmask of the CGB channels m4a
  currently owns, cleared at the top of `CgbSound` and set per active channel,
  so the GBS player knows which hardware channels it may take over.

Both structs are pure C in expansion and CrystalDust alike (neither ships m4a
assembly), so the layout change has no hardcoded offsets to chase.

Also fixed a pointer/integer comparison at `gbs.c:704` that agbcc accepted.

Errors 79 -> 71.

## D30 — The radio compiles

`src/radio.c` (GSC's radio stations) needed six separate restorations:

- **`gPokedexShowEntries`** — CrystalDust gives every Johto-dex species a
  second, shorter blurb (`pokedexShowEntry`) that the Pokédex radio show reads
  out. Phase 1 kept expansion's species data, which has no such field. Rather
  than widen `gSpeciesInfo` by a pointer per species, the 251 texts are ported
  as a standalone table in `src/data/pokemon/pokedex_show_entry_table.h`, and
  `GetPokedexShowEntry()` falls back to the ordinary species description for
  everything outside #1-251 — exactly what CrystalDust's own data did for the
  136 Hoenn entries.
- **`CountBadges`** in `src/script.c` — counts all **16** badges. Expansion's
  `NUM_BADGES` is 8 (Johto only); the Kanto `FLAG_BADGE09_GET`..`16` flags do
  exist, so nothing is lost, but `NUM_BADGES` cannot be used as the total.
- **`GetMapWildMonFromIndex`** in `src/wild_encounter.c` — Oak's Pokémon Talk
  names a species from another map's land table. Rewritten against expansion's
  `encounterTypes[timeOfDay]` layout. **Returns `SPECIES_NONE` for every Johto
  route until D27's encounter tables land.**
- **`GetCurrentRegion`** — already existed in `include/regions.h`, but its
  `GetRegionForSectionId` could only ever return `REGION_KANTO` or
  `REGION_HOENN`, so `== REGION_JOHTO` was dead. Johto's map sections are one
  contiguous block, so `JOHTO_MAPSEC_START`/`_END` were added to the mapsec
  constants template and the lookup taught about them.
  `MAPSEC_LAVENDER_RADIO_TOWER` sits inside that block but is Kanto, and is
  special-cased.
- Symbol renames: `gSpeciesNames[x]` → `GetSpeciesName(x)`, `StringCopy10` →
  `StringCopy`, `gTrainers[x].trainerName` → `GetTrainerNameFromId(x)`,
  `CHAR_DBL_QUOT_*` → `CHAR_DBL_QUOTE_*`.

**D20a debt discharged for `radio.c`:** the two flagged sites are not
three-period switches. `radio.c:288` picks a period at random and expansion's
four-entry tables handle that correctly; `radio.c:815` is a single
`== TIME_MORNING` test. No evening→night fallthrough is owed here.

Errors 71 -> 62.

## D31 — Two day/night systems; expansion's tinting wins

**This one changes how the game looks. Flagging it explicitly.**

`src/day_night.c` is CrystalDust-only, but expansion 1.17 ships its own,
independent day/night system — `gTimeBlend`, `UpdateTimeOfDay`,
`UpdateAltBgPalettes`, `BeginTimeOfDayPaletteFade`, `MapHasNaturalLight` — and
it is already wired through `palette.c`, `field_weather.c` and `overworld.c`.
Running both would have two things fighting over `gPlttBufferUnfaded`.

**Decision: expansion's tinting wins; `day_night.c` is reduced to the parts
expansion has no equivalent for.** This follows D2 ("let Emerald win visually"),
and expansion's system is also strictly more capable — it blends time-of-day
tint with weather, which CrystalDust's hour-LERP does not.

Removed: `sTimeOfDayTints` (the 24 hourly tints), `LerpColors`,
`TintPaletteForDayNight`, `gPlttBufferPreDN` (an entire extra `PLTT_BUFFER_SIZE`
of EWRAM), and the retint phase machinery.

Removed as dead: `LoadPaletteDayNight`, `LoadCompressedPaletteDayNight`,
`DoLoadSpritePaletteDayNight`. **Nothing in this tree called them** — expansion
loads every palette through its own path. This also resolves the outstanding
`day_night.c:254` "source potentially unaligned" static assert, which came from
`LZDecompressWram` into the old decompression buffer.

Kept, and still CrystalDust's: the palette-override table (`gPaletteOverrides`,
per-slot palettes swapped in between given hours — Crystal's lit windows),
`ShouldSetTintToNight`, the day-of-week strings, and the time-of-day rollover
hook that re-picks the ambient cry and forces the time-based events.

### Constraints accepted (not silent)

1. **Forced-night maps are now done by pinning the apparent hour.** Ilex Forest,
   Dragon's Den, the Safari Zone office and the unlit Lighthouse 6F set
   `sHoursOverride` on warp instead of forcing a night tint. Same effect, and it
   now also makes encounters and events there behave as night, which is
   arguably more correct than CrystalDust's tint-only version.
2. **`gPaletteOverrides` is currently never populated.** CrystalDust filled it
   from tileset/map load code we do not have. The machinery is kept because it
   is the hook Crystal's animated lit windows need, but it does nothing yet.
   Another Phase 1 loss, logged for later.
3. **The debug tint sliders (`gDNTintOverride`) are inert.** The time-cycle
   override (`gDNPeriodOverride`) still works — it now drives expansion's
   `SetTimeOfDay`. The RGB sliders have nothing to drive.
4. `ForceTimeBasedEvents` was restored to `src/field_tasks.c`, and
   `ChooseAmbientCrySpecies` exposed via `ForceChooseAmbientCrySpecies`.

Errors 62 -> 50.

## D32 — GBS song selection wired into the m4a API

CrystalDust's whole `m4a` entry-point API carries a `gbsEnabled` argument —
`m4aSongNumStart(n, gbsEnabled)` and friends — because a song may have a Game
Boy Sound counterpart in `gGBSSongTable`, a *sparse* table keyed by song id and
terminated with `0xFFFFFFFF`. Phase 1 kept expansion's one-argument API, so
`gGBSSongTable` (which survived in `sound/song_table.inc`) was unreachable.

**Decision: keep expansion's one-argument public API and consult
`FLAG_SYS_GBS_ENABLED` inside a new `GetSong()`**, rather than threading a
second argument through 72 call sites in 18 files. Every CrystalDust call site
passed `FlagGet(FLAG_SYS_GBS_ENABLED)` anyway, so behaviour is identical and the
diff is five lines in `m4a.c` instead of seventy-two across the tree.

For the two places that genuinely need to override the player's setting — the
sound test auditioning both formats — `m4aSongNumStartGbs` / `m4aSongNumStopGbs`
save, set, call and restore the flag.

`src/debug/sound_check_menu.c` also needed `TEXT_SPEED_FF` → `TEXT_SKIP_DRAW`.

**Note:** this does *not* resolve D14. The main `gSongTable` still has 43 songs
pointing at the wrong music. GBS lookups will be correct; the m4a fallbacks
will not, until D14 is fixed.

**Correction (Phase 4 re-audit).** The note above is stale — D14 *was* resolved in
its own entry, and re-checking all 588 numeric song constants against the 666
`gSongTable` rows confirms it: every constant lands on its own label. The only
four that do not are `SE_INTRO_PICHU_WOOPER` and `SE_INTRO_SUICUNE1`–`3`, which
are GBS-only sound effects with a deliberate, comment-annotated vanilla m4a
placeholder (`song se_bike_hop, @ SE_INTRO_PICHU_WOOPER`). CrystalDust does the
same. `MUS_ROUTE118` (32767) and `MUS_NONE` (65535) are sentinels, and
`MUS_DESERT` is a vanilla alias of `mus_route111`. Nothing outstanding.

Errors 50 -> 32. Every remaining error is in `src/pokegear.c`.

## D33 — The Pokegear map card gets its own module; expansion's region map stays

**Problem.** `src/pokegear.c` needed fifteen region-map symbols that expansion's
`src/region_map.c` does not have. CrystalDust's region map is a different engine:
it scrolls, it carries multiple region images (Johto / Kanto / three Sevii sheets),
it has a two-layer mapsec system (primary + secondary), per-mode permissions, and
a landmark-info mode. Its `struct RegionMap` shares almost nothing with
expansion's beyond the name.

**Rejected: swap in CrystalDust's `region_map.c` wholesale.** Twenty-six files in
this tree include `region_map.h` — the Pokédex area screen, the Fly map, the
summary screen, DexNav, map preview, Pokénav, TV, Buena's password. Expansion's
version also carries `RegionMapType` and the FRLG/Sevii layouts that Q4 depends
on, plus the D17 retargets. Replacing it would break all of that to fix one screen.

**Decision.** CrystalDust's region map is ported as a *separate* module,
`src/pokegear_map.c` / `include/pokegear_map.h`, backing the Pokegear map card
only. Every function it defines is prefixed `CDMap_`, and its struct is
`struct CDRegionMap`, so the two engines coexist with no namespace collision.
Expansion's `region_map.c` is untouched and keeps serving its other consumers.

Crystal's map card is therefore not dropped, and nothing that works today regresses.

**What this cost, explicitly:**

1. **CrystalDust's Fly map is not ported.** `CB2_OpenFlyMap` and its icon/入力
   code were removed from the new module; expansion's Fly map in `region_map.c`
   stays in use. CrystalDust's version indexed a `sMapHealLocations` table keyed
   by `HEAL_LOCATION_*` constants for Johto, none of which exist in this tree yet
   — Johto heal locations are Phase 3 work. Revisit once they land: the Fly map
   is the one remaining consumer that would genuinely benefit from CD's
   multi-region map.
2. **`REGION_*` was a live trap.** CrystalDust's region map used its own
   `enum { REGION_JOHTO, REGION_KANTO, REGION_SEVII1..3 }` starting at 0, which
   collides by name with expansion's `enum Region` (`REGION_KANTO` = 1,
   `REGION_JOHTO` = 2). Every `currentRegion` comparison would have silently
   picked the wrong map image. Renamed to `CDMAP_REGION_*` throughout, including
   in `src/data/region_map/mapsec_to_region.h`.
3. **`MAPSEC_SEVII_ISLE_6` … `_9` do not exist here**, so their rows were dropped
   from `mapsec_flags.h` and `mapsec_to_region.h`. `region_map_sections.json` is
   at 252/252, its hard ceiling, so they cannot simply be added. The four are
   uninhabited islets; this matters only if Q4's Sevii support later wants them.
4. **`MAPSEC_ROUTE_10_FLYDUP` / `MAPSEC_ROUTE_3_FLYDUP` likewise do not exist.**
   CrystalDust used them for a two-pixel cursor nudge on those Kanto routes; the
   nudge is now unconditional. Purely cosmetic, and only on the Kanto map.
5. **`src/data/region_map/region_map_names_emerald.h` was dropped from the
   include set.** Its `MAPSECEM_*` constants and `EMERALD_MAPSEC_START` were lost
   in the Phase 1 merge, so the table has never been buildable here. Nothing in
   the module referenced it. This is another one-side merge loss, logged for the
   `phase1-asset-merge.txt` re-audit rather than fixed now.
6. **CrystalDust's `GetMapName` family was discarded** in favour of expansion's,
   which is already the one the other twenty-six consumers use.

**Also settled here:** `SaveBlock2` gains `twentyFourHourClock:1`, taken from the
four spare padding bits after `regionMapZoom`. SaveBlock2 does not grow, so the
`SaveBlock1FreeSpace` assert is unaffected, and CrystalDust's 12/24-hour clock
toggle survives rather than being cut.

`MultichoiceList_PrintItems` was added to `menu.c`;
`InitMenuInUpperLeftCornerPlaySoundWhenAPressed` is a macro alias for expansion's
identical `InitMenuNormal`.

**This clears the last C compile error in Phase 2.** What remains are asset-level
failures: tileset tile counts over the 256 maximum, and `events.inc` generation
for the Johto maps.

## D34 — Poryscript wired into the build

CrystalDust writes its map scripts in Poryscript. All 401 `scripts.pory` files
were carried over in Phase 1, but nothing compiled them: Poryscript had no build
rule, so 400 of the 401 maps — every map in Johto — had no `scripts.inc` at all.
This was the Phase 3 blocker.

**Wiring.** `tools/poryscript/poryscript` is a prebuilt release binary, not
something this repo compiles, so it stays out of `make_tools.mk`'s inclusive
`TOOL_NAMES` list. Instead `Makefile` gains `PORYSCRIPT` / `PORY_FONTCFG`, a
pattern rule, and a guard rule that errors with a fetch URL if the binary is
missing.

**Two traps worth recording:**

1. **The pattern rule must be scoped.** CrystalDust's is `data/%.inc:
   data/%.pory` paired with a `%.pory: ;` catch-all. That catch-all tells Make
   it can *create* any `.pory` out of nothing, so Make happily decided to build
   `data/script_cmd_table.inc` from a `data/script_cmd_table.pory` that has never
   existed. The rule here is scoped to `data/maps/%/scripts.inc` and there is no
   `%.pory: ;`.
2. **scaninc cannot see an `.include` whose target does not exist yet.** With the
   pattern rule in place Poryscript still never ran, because the only thing that
   would ask for `scripts.inc` is the dependency file scaninc generates for
   `event_scripts.s`, and scaninc silently skips includes it cannot open. Fixed
   with an explicit `$(DATA_ASM_BUILDDIR)/event_scripts.o: $(MAP_SCRIPTS)` in
   `map_data_rules.mk`.

**Tenth one-side merge loss, found on the way.** `data/event_scripts.s` kept
expansion's list of map script includes wholesale. 408 of CrystalDust's were
gone, Johto entirely among them — so even once Poryscript generated the files,
nothing assembled them. Restored in CrystalDust's own order.

**Result: all 400 Johto maps compile with zero Poryscript errors.** The generated
`scripts.inc` are gitignored under `data/maps/*/`; the Hoenn maps' hand-written
ones predate this and stay tracked, which git's ignore rules leave alone.

## D35 — The Sevii Islands are cut

**Reverses Q4.** The Sevii Islands are FireRed/LeafGreen content, introduced in
Gen 3 and built around the Network Machine and the roaming beasts. Gold, Silver
and Crystal have exactly two regions, Johto and Kanto. Nothing in the Gen 2 story
reaches the Sevii Islands, and CrystalDust does not ship them.

Consequences, all of them simplifications:

- D33's missing `MAPSEC_SEVII_ISLE_6` … `_9` stop mattering.
- The Sevii `MAPSEC_*` entries become the obvious reclaim candidates for the
  252/252 `region_map_sections.json` ceiling we have been pressed against since
  D30, ahead of Marine/Terra/Underwater Marine Cave.
- Expansion's `RegionMapType` FRLG/Sevii layouts in `region_map.c` are dead
  weight, though harmless; D33's reasoning for keeping that file stands on its
  other 25 consumers regardless.
- The `*_Frlg` map directories can go. Their missing `LAYOUT_*` entries are a
  large share of the current mapjson failures, so this removal is folded into the
  map-data pass rather than done as a standalone deletion, and it should ride
  along with the deferred 504-map Hoenn removal.

## D36 — Executing D35: FRLG maps out, object-event types normalised

Three pieces of work that had to land together, because each unblocked the next.

**420 map directories deleted.** Every `data/maps/*_Frlg/` (419) plus
`data/maps/SevenIsland_UnusedHouse/`. These were pure orphans: not one of them
appears in `data/maps/map_groups.json`, so nothing referenced them and the
deletion is purely subtractive. 1349 → 929 map directories. The 418 matching
`.include` lines came out of `data/event_scripts.s` with them.

**`"type": "original"` → `"type": "object"` in 473 map.json files, 2283 object
events.** `tools/mapjson/mapjson.cpp:267-292` accepts `""`, `"object"` and
`"clone"` and calls `FATAL_ERROR` on anything else, so every one of those maps —
all of Johto among them — was failing to generate its `events.inc`. CrystalDust's
older mapjson accepted `"original"` as the default kind. We normalise the data
rather than patch the vendored tool: expansion's tool is the one that also knows
about clone objects, and a patched tool is a merge hazard at every future pull.
This took the assembly error count from 674 to 0.

**`src/data/heal_locations.json` was corrupt.** Line 315 held 20 NUL bytes where
`"id": "HEAL_LOCATION_` should have been, so JSONPROC could not parse the file at
all. Repaired, then rewritten through `json.dumps(indent=2)`.

With that file parsing again, the Sevii work followed: the seven Sevii heal
locations are gone (42 → 35 entries), and expansion's fly-destination table in
`src/region_map.c` has its Sevii rows retargeted to `HEAL_LOCATION_NONE`. The
rows themselves stay — the table is indexed by `MAPSEC_*`, so removing rows would
shift nothing but would leave holes that read as deliberate. Fly cannot reach the
Sevii Islands, which is the intent.

**Six Kanto Pokémon Centers gained a nurse `local_id`.** `heal_locations.json`
named `LOCALID_VERMILION_NURSE`, `LOCALID_CELADON_NURSE`, `LOCALID_FUCHSIA_NURSE`,
`LOCALID_CINNABAR_NURSE`, `LOCALID_SAFFRON_NURSE` and `LOCALID_LEAGUE_NURSE` as
respawn NPCs; those ids lived in the FRLG Pokémon Center maps we just deleted.
CrystalDust's own Kanto Pokémon Centers declare no local ids at all, but in every
one of them the nurse is object index 0, so the ids were added there. Indigo
Plateau is the exception: `LOCALID_LEAGUE_NURSE` already exists as Ever Grande's
nurse in Hoenn, exactly the name-unification trap D19 documented, so Indigo
Plateau got its own `LOCALID_INDIGO_NURSE` instead of silently respawning the
player against a Hoenn object id.

**Still open, and not caused by this work:** 293 "Failed to find matching layout"
failures and 153 `undefined map` assembly errors. Both are Hoenn leftovers —
directories that survive on disk but are absent from CrystalDust's
`map_groups.json` (`MAP_BATTLE_FRONTIER_OUTSIDE_WEST`, `MAP_SOUTHERN_ISLAND_EXTERIOR`,
`MAP_FARAWAY_ISLAND_ENTRANCE`, `MAP_LILYCOVE_CITY_CONTEST_LOBBY`,
`MAP_NAVEL_ROCK_HARBOR`, `MAP_BIRTH_ISLAND_HARBOR`). They resolve with the
deferred Hoenn removal, not before.

**Gap to flag:** `src/data/heal_locations.json` contains no Johto heal locations
whatsoever. Every Johto Pokémon Center is currently a dead respawn point, and
D33's deferred Fly-map port cannot be finished without them. Phase 3 work.

## D37 — The Hoenn orphans go, and the build reaches zero C errors

The FRLG removal (D36) left 153 `undefined map` assembly errors and 293
"Failed to find matching layout" failures. Both turned out to be the same thing:
364 Hoenn map directories that survive on disk but appear nowhere in
CrystalDust's `data/maps/map_groups.json`. Every *live* map has its layout —
verified by walking all 565 of them against `layouts.json` — so the layout
failures were entirely orphan maps, and the removal is again purely subtractive.
929 → 565 map directories, with 361 `.include` lines dropped from
`data/event_scripts.s`.

Five further things had to be fixed for the data to assemble.

**The `map` macro lost its typo guard.** CrystalDust's mapjson emits a
"Constants for unused maps" block of plain `#define`s (group 112) so that
leftover scripts referencing removed maps still compile. Those reach gas as bare
numbers, and expansion's `map` macro guards itself with `.ifdef \map_id`, which
is an error on a numeric literal — not a false result, an error. CrystalDust
drops the guard for exactly this reason and we follow it. **Cost, stated
plainly: a mistyped map constant in map data now assembles to a bogus map
instead of failing the build.**

**`MAP_NONE` is now an alias for `MAP_DYNAMIC`.** Same value in both projects,
`(0x7F | (0x7F << 8))`; 36 CrystalDust map.json files use CrystalDust's name for
dummy warp destinations.

**18 clone object events were converted.** CrystalDust writes them as
`source_id`/`source_map` with no `graphics_id`; expansion's mapjson wants
`target_local_id`/`target_map` and requires a `graphics_id`. Each clone's
graphics were resolved from the object it clones, so Azalea Town's cloned hiker
is a hiker and not a placeholder.

**52 songs had no `midi.cfg` entry.** CrystalDust drove mid2agb from per-song
rules in its own `songs.mk`, which Phase 1 did not carry across; expansion drives
it from `sound/songs/midi/midi.cfg`. All 52 sets of arguments were carried over
verbatim with `$(STD_REVERB)` resolved to 50. This is an **eleventh one-side
merge loss** — every Johto track, from `mus_new_bark` to `mus_vs_johto_leader`,
was silently unbuildable. Note the file is parsed by a `make` `foreach`, so it
takes no comments and no blank lines.

**Four Hoenn-only features referenced local ids from deleted maps.** The mart
employee table in `field_specials.c` walked eleven Hoenn marts to return a value
its own comment admits is always 1, so it returns 1 directly. Gabby and Ty roam
Routes 111/118/120 and now report `LOCALID_NONE`. The Slateport Energy Guru
PokeNews check returns FALSE. The moving-truck intro keeps its code — its
specials are still in the specials table — and defines the three box local ids
locally at the values `InsideOfTruck` gave them.

**Result: 0 C errors, 0 assembly errors in map data, all 581 songs building.**

Left over, and being worked next: 12 tilesets over the 256-tile maximum, three
missing multiboot `.gba` images, and `ITEM_MACHINE_PART`.

## D42 — the last assembly errors, and the first link

The build now compiles every C file and assembles every script. What follows is
what it cost.

**Six CrystalDust script commands took more arguments than expansion's macros.**
Three were pure cosmetics and are **accepted and discarded**: `updatemoneybox`
and `updatecoinsbox`/`hidecoinsbox` take CrystalDust's `x, y`, and `showmoneybox`
takes its third "suppress the box" flag. Expansion's money and coins boxes
remember their own position, and the suppress flag is `FALSE` at every call site
in this tree, so nothing is lost in practice. Three were real and were ported
into expansion's script commands: `checkmoney`/`removemoney` regained
CrystalDust's `isVar` argument (`checkmoney VAR_TEMP_1, TRUE` is a live call
site, in the Goldenrod prize scripts), and `showmonpic` regained its shininess
argument, which meant threading `isShiny` through `ScriptMenu_ShowPokemonPic`
and `CreateMonSprite_PicBox`.

**CrystalDust's TM item names were renumbered away.** Twenty constants of the
form `ITEM_TM24_THUNDERBOLT` became expansion's `ITEM_TM_THUNDERBOLT`. Expansion
no longer pins a TM to a number, so the number in the name would be a lie.

**`TRAINER_BATTLE_SET_TRAINER_A`/`_B` do not exist in expansion.** CrystalDust
used them in two never-executed scripts in the Team Rocket Base, whose only
purpose is to be a byte layout that `battle_tower.c` reads the Lance double
battle's opponents out of. Expansion's `trainerbattle` emits a single
`TrainerBattleParameter` block, and `trainerbattle_no_intro` emits exactly the
block that was wanted, so that is what those two scripts now use.

**The Lance double battle was rebuilt on expansion's partner system.**
`SPECIAL_BATTLE_LANCE` is new, and `PARTNER_LANCE` (Dragonite, level 40, Brave,
Fly/Twister/Thunder/Extreme Speed) is a new entry in `battle_partners.party`.
CrystalDust hand-rolled that Dragonite inside `battle_tower.c` with a fixed OT id
of 149 and a rejection loop to force it male and non-shiny; expansion's partner
generator covers the same ground, so the hand-rolled version is gone. **Cost:
the Dragonite's OT id is no longer pinned to 149.**

**CrystalDust's move tutor index is now a move id.** CrystalDust kept a
`TUTOR_MOVE_*` index in `VAR_0x8005`; expansion's tutor reads a `MOVE_*` id from
the same variable. The two live call sites (Bugsy's Fury Cutter, Route 31's
Nightmare) and the ported Poke Seer specials set move ids.

**Two CrystalDust overworld sprites were restored:** Janine (Fuchsia Gym) and the
tailless Slowpoke (Azalea Town). Both were referenced by scripts and map data
that had survived Phase 1 while the graphics had not — the one-side merge loss
again, now the twelfth and thirteenth instances.

**Six specials were restored.** `PokeSeerGetMoveToTeachLeadPokemon` and
`HasLearnedAllMovesFromPokeSeerTutor` (the Cianwood ultimate-move tutor) and
`NameRaterWasNicknameChanged` were ported; `FindPhoneContactNameFromFlag`,
`CopyBugCatchingContestRemainingMinutesToVar1` and `GetPlayerBugContestPlace`
existed in C but had no `def_special` entry.

**One duplicate symbol.** CrystalDust's Bugsy script and expansion's Verdanturf
tutor both defined `MoveTutor_EventScript_FuryCutterDeclined`. CrystalDust's is
Bugsy-specific (it prints Bugsy's outro), so it was renamed
`AzaleaTown_Gym_EventScript_FuryCutterDeclined`.

**Result: 0 C errors, 0 assembly errors. The link stage is reached for the first
time.** It does not link yet: 884 undefined symbols and 6 duplicate definitions
remain. ROM is at 27,213,184 bytes (81% of 32MB) with the sound bank still
incomplete — **this is past the 28MB watch line's neighbourhood and needs
attention before Phase 7 adds anything.**

## D43 — the first pass at the link errors

Build p70 reached the linker with 884 undefined symbols and 6 multiple
definitions. This pass cleared the duplicates and the three largest
"constant that never got defined" buckets. Constraint decisions:

- **Prof. Oak's Pokedex rating is CrystalDust's, not expansion's.** `src/prof_pc.c`
  (CrystalDust) and `src/birch_pc.c` (expansion) both defined `ScriptGetPokedexInfo`,
  `GetPokedexRatingText` and `ShowPokedexRatingMessage`. CrystalDust wins:
  `birch_pc.c` is deleted, `data/text/pokedex_rating.inc` now carries CrystalDust's
  21 `gPokedexRatingText_*` strings instead of expansion's `gBirchDexRatingText_*`,
  and `data/scripts/pokedex_rating.inc` was rewritten to CrystalDust's flow
  (Johto dex rated in 15-mon bands, then the National dex) in expansion's macro
  syntax. **Dropped with it:** expansion's FRLG `GetFrlgPokedexCount` and
  `GetProfOaksRatingMessage` specials and the Hoenn-only `data/scripts/prof_birch.inc`.
- **`gRegionMapEntries` is defined once, in `src/region_map.c`.** `src/pokegear_map.c`
  (added in D33) included `data/region_map/region_map_entries.h` a second time; it
  now uses the extern from `include/region_map.h`.
- **`gMonIcon_Egg` / `gMonIcon_QuestionMark`**: the stale pokeemerald `INCBIN_U8`
  definitions in `src/graphics.c` were removed in favour of expansion's generated
  ones in `src/data/graphics/pokemon.h`.
- **Johto heal locations added** (16 entries) to `src/data/heal_locations.json`
  from CrystalDust's table. Expansion's format needs a respawn point, which
  CrystalDust's did not have: towns use their own Pokemon Center; **New Bark Town
  respawns in the player's house** (it has no Center), **Lake of Rage respawns at
  Mahogany Town** and **Route 23 at the Indigo Plateau Center**. These three are
  judgement calls, not ported data.
- **36 CrystalDust multichoice lists restored** (`MULTI_MOM_BANK`, the Game Corner
  prize and tutor menus, the five Dragon Shrine questions, and the fossil-revival
  combinations that add Root and Claw Fossil to expansion's Helix/Dome/Amber set).
- **53 CrystalDust overworld sprites restored.** The PNGs came across in Phase 1;
  only the C tables were lost. Eight more are **aliases onto expansion's existing
  sprites** rather than ports, so they will look like Emerald rather than
  CrystalDust: `OBJ_EVENT_GFX_BATTLE_GIRL` -> `GIRL_3`, `GIRL` -> `GIRL_2`,
  `GYM_GUIDE` -> `MAN_2`, `WOMAN` -> `WOMAN_1`, and CrystalDust's four
  `Z`-prefixed Hoenn leftovers (`ZMR_BRINEYS_BOAT`, `ZNINJA_BOY`, `ZRICH_BOY`,
  `ZSCOTT`) onto their unprefixed expansion equivalents.
- **Twenty of those sprites collide with expansion's follower graphics**
  (`gObjectEventPic_Abra` and friends are already defined from
  `graphics/pokemon/*/overworld.png`). CrystalDust's field versions are kept under a
  `Gen2` suffix (`gObjectEventPic_AbraGen2`), so the map NPCs use CrystalDust's
  Gen-2-styled sprites while followers keep expansion's.
- **Eleven CrystalDust overworld palettes restored** with new tags at 0x1170-0x117A
  (Gold, Kris, Prof. Elm, Silver, Butterfree, Will, Red Gyarados, Murkrow, Eusine,
  Dragonite, S.S. Aqua). The S.S. Aqua palette is taken from its PNG, since
  CrystalDust shipped it as a loose `.gbapal` that did not come across.
- **S.S. Aqua's sprite is converted without metatile arguments.** Its PNG is
  128x64 (16x8 tiles), which is not a multiple of the 8x16 metatile CrystalDust's
  pic table declares; the frame is the whole image either way.

Remaining at the end of this pass: 737 undefined symbols, 0 multiple definitions.

## D44 — the Johto tilesets

`src/data/tilesets/{headers,graphics,metatiles}.h` are what expansion actually
compiles (via `src/tilesets.c`); `data/tilesets/*.inc` is dead assembly-era code.
62 `gTileset_*` symbols were undefined at link. 24 were the Kanto/FRLG set, which
expansion hides behind `#if !IS_FRLG ... #else ... #endif`; CrystalDust needs both
halves, so the split was removed in all three files and both branches now always
compile. The other 38 were CrystalDust's Johto tilesets: the assets were all
present under `data/tilesets/{primary,secondary}/<name>/` from the Phase 1 merge,
but the C declarations had been lost. They were regenerated from CrystalDust's
`headers.inc`/`graphics.inc`/`metatiles.inc` in expansion's C form.

Constraint decisions, none of them silent:

1. **Johto tileset animations are not running yet.** CrystalDust's 38 headers name
   `InitTilesetAnim_*` callbacks that do not exist in our `src/tileset_anims.c`;
   every new header is emitted with `.callback = NULL`. Animated water, flowers
   and the like on Johto maps will be static until the anim functions are ported.
2. **`gTilesetPalOverrides_*` is dropped.** CrystalDust's assembly header has a
   seventh word for per-metatile palette overrides; expansion's `struct Tileset`
   has no `palOverrides` field. The override assets (e.g. `palettes/09_over.pal`)
   stay in the tree, unused, pending the D31 `gPaletteOverrides` follow-up.
3. **Palette lists are contiguous.** A few of CrystalDust's `.inc` palette lists
   skip indices (Azalea lists 13 of 16). Expansion indexes `tileset->palettes[i]`
   absolutely, so skipping would shift every later slot; each header now lists
   every `NN.pal` present in the asset directory, in order.
4. `gTileset_Goldenrod` keeps CrystalDust's cross-reference to
   `gTilesetPalettes_Rustboro` rather than getting its own copy.

Link errors: 713 undefined -> 675 undefined, 0 multiple definitions, 0 compile
errors.

## D45 — the sound bank

455 of the 675 undefined symbols were the CrystalDust sound bank. The assets were
all in the tree; the build simply wasn't reaching them.

- `sound/voice_groups.inc` kept only CrystalDust's 191 numbered voicegroups, so
  expansion's 203 named ones (`voicegroup_abandoned_ship`, ...) were dropped even
  though every `.inc` file was present. Both lists are now included.
- `sound/direct_sound_data.inc` kept expansion's side; CrystalDust's 128 sample
  blocks were re-appended, `programmable_wave_data.inc` and `keysplit_tables.inc`
  gained the 25 and 5 blocks each had lost from the other side.
- CrystalDust ships its samples as `.aif`; expansion's `audio_rules.mk` only knew
  how to convert `.wav`. `tools/aif2pcm` was brought over and a `%.bin: %.aif`
  rule added.
- `sound/songs/gbs` (126 GB-sound songs, CrystalDust's own player) was never
  wired into the Makefile. It now builds, along with `asm/macros/gbs.inc` and
  `asm/macros/phone.inc`, which the merge had also dropped.
- `data/sound_data.s` pulled `sound/song_table.inc` with a gas `.include`, which
  runs after cpp, so the `MUS_*`/`SE_*` constants in `gGBSSongTable` never
  resolved. It is a cpp `#include` now, with `constants/songs.h` above it.

Constraint decisions:

1. **CrystalDust's `sound/songs/*.s` are not built.** All 110 are byte-for-byte
   the same songs expansion generates from `sound/songs/midi/*.mid`, and building
   both gave 110 duplicate definitions. The `.mid` pipeline wins; the `.s` files
   stay in the tree unused. This is a build-order choice, not a content cut - no
   song is lost.
2. **CrystalDust's `gCryTable2` block is not appended.** It uses a `cry2` macro
   that expansion's `asm/macros/m4a.inc` does not define, and no cry symbol was
   undefined, so expansion's cry tables (which already cover every species) are
   kept whole. Revisit if Gen 2 cries sound wrong.

Link errors: 675 undefined -> 218 undefined, 0 multiple definitions, 0 errors.

## D46 — CrystalDust's script commands

`data/script_cmd_table.inc` carried 24 CrystalDust opcodes past expansion's
0xe6, but none of their handlers survived the merge. Five of the 24 were not
commands at all: `specialvar_`, `trainerbattlebegin`, `vloadptr`,
`warpteleport2` and `buffercontesttypestring` are macros CrystalDust wrote for
opcodes that already exist (0x26, 0x5d, 0xbe, 0xd1, 0xe1), and in expansion
those opcodes mean something else entirely. Nothing in our scripts uses them, so
the five table entries and the five macros are removed and the table renumbered
(250 commands, last opcode 0xf9); the `.macro` opcodes were renumbered to match.

The other 19 are ported from CrystalDust's `src/scrcmd.c`, along with four
helpers the merge had also dropped: `GetPriceReduction` and `IsPriceDiscounted`
(`src/tv.c`), `SetObjectPriority`/`ResetObjectPriority`
(`src/event_object_movement.c`) and `DoSootopolisLegendWarp`
(`src/field_screen_effect.c`).

Constraint decisions:

1. **`unownmessage` is expansion's `braillemessage` without the 6-byte header.**
   CrystalDust's version used the pre-1.0 window API (`gBrailleWindowId`, raw
   font 6, `CopyWindowToVram(..., 3)`). It is re-expressed against expansion's
   current API, so it now honours `FONT_BRAILLE` and the standard window border.
2. **`MON_DATA_EVENT_LEGAL` is `MON_DATA_MODERN_FATEFUL_ENCOUNTER`.** Same field,
   renamed upstream; `checkmoneventlegal`/`setmoneventlegal` use the new name.
3. **`IsPriceDiscounted` is partly inert.** CrystalDust had already gutted its
   Hoenn map checks (they read `MAP_GROUP(NONE)`); the Slateport case keeps the
   `gSpecialVar_LastTalked == 25` test and the rest returns TRUE. It wants a real
   Goldenrod/Radio Tower rule once the Poke News content is ported.
4. Every ported command declares `Script_RequestEffects(SCREFF_V1)`, expansion's
   script-effect contract, which CrystalDust predates. Commands that touch the
   screen may need a wider mask once they are exercised in game.

Link errors: 218 undefined -> 194 undefined, 0 errors.

## D47 — the phone system

`data/phone.s` — CrystalDust's entire Pokegear phone script bank, including
`gPhoneScriptCmdTable` — was missing from the tree, which is where the 24
`PhoneScript_*` and both `gPhoneScriptCmdTable*` symbols were going. It and
`data/phone_script_cmd_table.inc` are restored. `data/text/match_call.inc` had
kept CrystalDust's 112 lines and lost expansion's 32 gym-leader ones, which
`src/pokenav_match_call_data.c` still references; those are appended.

Getting `phone.s` to assemble needed two constant fixes, both of which are worth
knowing about generally:

1. **`MAPSEC_*` and `REMATCH_*` are `#define`s again.** Expansion had turned both
   anonymous enums into C enums, and enum members do not exist as far as the
   assembler is concerned - CrystalDust's phone scripts compare against them, so
   they assembled to undefined symbols. Both headers are back to preprocessor
   macros with identical values (this is also what vanilla pokeemerald did).
2. **`include/constants/asm_enums.h` is new.** `enum MapType` and
   `enum TimeOfDay` are genuine C types used in function signatures, so they
   cannot be converted the same way. The new header restates their members as
   macros for assembly consumers only, and `data/phone.s` includes it. **It must
   be kept in sync by hand** with `constants/map_types.h` and `constants/rtc.h`.
3. `data/phone.s` included `constants/battle_setup.h` (gone; nothing in the file
   used it) and `constants/gym_leader_rematch.h` (now `constants/rematches.h`,
   which already carries CrystalDust's rematch names from D19).

Link errors: 194 undefined -> 133 undefined, 0 errors.

## D48: restore CrystalDust's field specials and get the ROM to link

This is the decision that finally produced a ROM. It closed out the last 194
link-time undefined symbols by restoring the CrystalDust code Phase 1 had
dropped, and by reconciling the handful of places where expansion and
CrystalDust disagree about an API.

### What was restored

- **`src/script.c`**: CrystalDust's ~60 field specials (the legendary-beast
  awakening and palette patching, the Kimono Girls, the fossil and bike checks,
  the whole `SetUpRoomDecor*` family, the msgbox-walkaway lock, the
  trapped-player checks). `CountBadges` was dropped as a duplicate of
  expansion's.
- **27 room-decoration object-event graphics.** Another *table-lost-but-assets-
  kept* case: the PNGs and palettes had survived Phase 1, but the constants,
  pic tables, graphics infos and pointer-table rows were all gone. Regenerated
  from CrystalDust.
- **Three roamers.** `ROAMER_COUNT` 1 -> 3 so Raikou, Entei and Suicune roam at
  once, with `src/roamer.c` gaining CrystalDust's regenerate/active specials.
- **Gabby and Ty radio hooks** (`src/tv.c`), `TurnObjectInRandomDirection`,
  `GetFreePokemonStorageSpace`, `IsPokeFluteChannelPlaying`, and CrystalDust's
  post-Red save (`RedClear` / `SaveGameRed` in `src/start_menu.c`).
- **The Bug-Catching Contest's party selection**, as a new facility
  (`FACILITY_BUG_CATCHING_CONTEST`) threaded through `src/party_menu.c`.
- **Poryscript for `data/scripts/`**: a new Makefile rule builds `.inc` from
  `.pory` there too, so CrystalDust's `move_tutors`, `day_care` and
  `bug_catching_contest` scripts are used as-is.
- **The radio, the nurse, the furniture and the gym-trainer setters.** CD's
  `check_furniture.inc`, `pkmn_center_nurse.inc` and `set_gym_trainers.inc` were
  strict supersets of expansion's, so those three files are CrystalDust's now,
  plus the matching texts (`Text_ItsATV`, the time-of-day nurse greetings).
- **`EventScript_ElevatorButton`** and `Std_MsgboxContinue` (as a new
  `gStdScripts` slot 13, `MSGBOX_CONTINUE`), and `CableClub_OnResumeFunc`.

### Constraint decisions

Nothing below was dropped silently; each is a place where CrystalDust and
expansion could not both be honoured.

1. **CrystalDust's `disableReflectionPaletteLoad` flag is gone.** Field 10 of
   `ObjectEventGraphicsInfo` is `compressed` in expansion and
   `disableReflectionPaletteLoad` in CrystalDust - same offset, different
   meaning. All 27 restored decoration graphics set `.compressed = FALSE`; none
   of them has a reflection, so the loss is cosmetic, but if a decoration ever
   does reflect oddly, this is why.
2. **`LoadPaletteDayNight` -> plain `LoadPalette`.** D31 removed the day/night
   palette variant, so the beast/Kimono Girl palette patches use
   `LoadPalette(pal, OBJ_PLTT_ID(n), PLTT_SIZE_4BPP)`. Those palettes no longer
   shift with the clock.
3. **`ROAMER_COUNT` 1 -> 3 changes the SaveBlock1 layout.** Saves from earlier
   builds of this project are not compatible.
4. **Expansion's Hoenn `move_tutors.inc` and `day_care.inc` were deleted** in
   favour of CrystalDust's Poryscript versions. Expansion's
   `MoveTutor_AfterChooseBoxMon` was carried over into the new `.pory` so boxed
   Pokemon can still be taught.
5. **`CanMonLearnTMHM` -> `CanLearnTeachableMove`.** The HM checks in the
   trapped-player specials now ask by species and move id.
6. **Tutor scripts now pass a `MOVE_` id, not a `TUTOR_MOVE_` index.**
   CrystalDust had its own 37-entry `TUTOR_MOVE_*` enum in `VAR_0x8005`;
   expansion's `ChooseMonForMoveTutor` reads a move id from the same variable.
   The `.pory` was rewritten rather than reintroducing the parallel enum.
7. **`FLDEFF_CAMERA_FLASH` -> `FLDEFF_PHOTO_FLASH`.** Expansion already had the
   same effect under its own name; the Cianwood Photo Studio uses it.
   CrystalDust's `FLDEFF_USE_WHIRLPOOL` is still unported - nothing references
   it yet, but Whirlpool as a field move is an open item.
8. **TVs are flavour text now, radios carry the shows.** `EventScript_TV` is
   CrystalDust's one-line sign. Expansion's Hoenn TV-show menu survives, renamed
   `EventScript_HoennTVShow`, but nothing reaches it; the Gabby and Ty and
   PokeNews content is on the radio instead.
9. **`gTrainerClassNames[][13]` -> `gTrainerClasses[].name`** and
   **`gPCText_Cancel` -> `gText_Cancel`**, both pure name-unification.
10. **`data/scripts/hoenn_stubs.inc` is new and deliberately inert.** D37 cut the
    Hoenn maps, but expansion's C code still names eleven scripts, five local
    IDs and five texts that lived there (closed Sootopolis doors, the Trick
    House, the Regi braille puzzle, the Wally/Scott/Roxanne/Rayquaza match
    calls, the S.S. Tidal step counter). Every guard in front of them is a Hoenn
    metatile behaviour or a Hoenn story flag that a Johto game never sets, so
    they are stubs. Deleting the file and restoring the maps is the way back.
11. **The player's bedroom PC is New Bark Town's.** `src/player_pc.c` and
    `src/field_control_avatar.c` no longer branch on player gender for the
    Littleroot bedrooms; they use `EventScript_PlayerPC` and
    `NewBarkTown_PlayersHouse_2F_EventScript_TurnOffPlayerPC`.
12. **Three scrollable-multichoice specials were not added.** CrystalDust
    exports `Close`/`Redraw`/`ResumeScrollableMultichoice` as specials for its
    department-store and Blue Card menus; no script in the tree calls them yet,
    and expansion's equivalent is `static`. To be revisited when the Goldenrod
    and Celadon dept store scripts land.

### Result

**The ROM links.** `pokeemerald.gba` builds clean, 28,992,608 bytes of content
(27.65 MiB, 86.4% of the 32MB cartridge) before padding.

**Size warning:** this is already past the ~28MB watch line agreed at the start
of Phase 2, and the remaining work (D27's 124 wild-encounter maps, the missing
trainer parties, Phase 7's expansion features) only adds. Compression of the
Johto tilesets or trimming unused Hoenn graphics will be needed before Phase 7.

Link errors: 194 undefined -> 0. Compile errors: 0. **Phase 2 gate: passed.**

## D49: CrystalDust's wild encounters, with time of day

`src/data/wild_encounters.json` was the Phase 1 merge's single biggest content
loss (recorded under D27): expansion's 388 Hoenn and FRLG maps were kept and
CrystalDust's 132 were discarded, so the game had **no Johto wild encounters at
all**. This replaces the `gWildMonHeaders` group with CrystalDust's.

The formats differ. CrystalDust nests three tables per field, one per band of
its three-band clock; pokeemerald-expansion 1.17 has the same feature but
expresses it as one JSON entry per map per time, with the time named in
`base_label`, gated behind `OW_TIME_OF_DAY_ENCOUNTERS`. That flag is now `TRUE`
and CrystalDust's tables were converted.

### Constraint decisions

1. **Evening reuses the night table.** D7 set `OW_TIMES_OF_DAY` to `GEN_LATEST`,
   a four-band clock (morning/day/evening/night); CrystalDust and real Crystal
   have three. Rather than invent evening encounters, evening gets Crystal's
   night table. This is a deliberate reading of Crystal's intent, not a port of
   something that existed.
2. **Times identical to morning are not emitted.** Of 132 maps only 209 of a
   possible 396 time variants actually differ from the morning table; the rest
   are left out and fall back at runtime (`OW_TIME_OF_DAY_DISABLE_FALLBACK` is
   `FALSE`). Behaviour is identical and the ROM does not carry four copies of
   every table.
3. **Expansion's 388 Hoenn and FRLG encounter tables are gone.** They belonged
   to maps D37 already cut. This is consistent with the "no Hoenn" scope and is
   why the change costs only ~25KB despite adding all of Johto.
4. `gBattlePikeWildMonHeaders` and `gBattlePyramidWildMonHeaders` are kept as
   expansion had them; the frontier is still deferred (Q3).
5. The `headbutt_mons` field now carries CrystalDust's `common`/`rare` groups.

ROM: 29,017,952 bytes of content (27.67 MiB, 86.48%), up 25,344 bytes.

## D50 — CrystalDust's trainer parties

After the Phase 1 merge, 496 trainer constants existed with no party data. They
are now restored from CrystalDust's C tables (`src/data/trainer_parties.h` +
`src/data/trainers.h`) into `src/data/trainers_crystaldust.party`, which
trainerproc compiles into `src/data/trainers_crystaldust.h` and `src/data.c`
includes alongside expansion's own `data/trainers.h`.

495 of 496 converted. Constraint decisions:

1. **`TRAINER_NONE_GSC` is left without a party.** It is CrystalDust's empty
   placeholder; trainerproc only permits a zero-mon trainer whose id ends in
   `_NONE`, and nothing references the constant. No content lost.
2. **`TRAINER_FLAGS_START` and `TRAINER_PARTNER` are not trainers** — a sentinel
   and expansion's ally slot. Neither needs a party.
3. **IVs are rescaled, not preserved exactly.** CrystalDust uses the old
   single-byte 0–255 `.iv` field; expansion's `.party` format takes 0–31 per
   stat. The conversion is `iv * 31 // 255`, applied uniformly to all six stats,
   which is what the old engine did at runtime. Rounding loses at most 1 point.
4. **CrystalDust's four `AI_SCRIPT_*` flags map onto expansion's `AI_FLAG_*`.**
   `SETUP_FIRST_TURN` becomes `AI_FLAG_FORCE_SETUP_FIRST_TURN`. Expansion's
   richer AI flags are *not* added — the trainers fight as CrystalDust wrote
   them. Revisit in Phase 7 if the battles feel too easy.
5. **Natures, EVs, abilities, balls, held-item variety and Gen 3+ mechanics are
   not set**, because CrystalDust's tables do not carry them. Every mon takes
   the engine defaults.

### Trainer constants restored alongside the parties

The parties referenced 70 constants that the merge had dropped:

- **19 trainer classes** (`BIKER BOARDER BURGLAR FIREBREATHER JUGGLER
  KIMONO_GIRL MEDIUM MYSTICALMAN OFFICER PKMN_TRAINER_3 RIVAL1 RIVAL2 SAGE
  SCIENTIST SKIER SUPER_NERD TEACHER TEAM_ROCKET TEAM_ROCKET_EXECUTIVE`), with
  CrystalDust's names and prize-money multipliers added to `gTrainerClasses[]`.
- **45 front pics** — the Johto gym leaders and Elite Four, Gold, the rival,
  Eusine, the five Kimono Girls, both Rocket Executives and Grunts, the Kanto
  leaders, and the class sprites. **Every one of the 45 PNGs was already in
  `graphics/trainers/front_pics/`**; only the constants, the `INCGFX`
  declarations and the `gTrainerPicInfo[]` rows were missing. No sprite was
  substituted and none had to be copied from CrystalDust.
- **6 encounter themes** (`LASS SAGE OFFICER ROCKET FISHERMAN KIMONO`), all six
  songs already present in the sound bank from D45.

6. **`struct Trainer.encounterMusic` was widened from 4 bits to 6.** The six new
   themes push the count past the 4-bit ceiling of 15. The two bits came from
   the adjacent `u16 padding:2` field, so `struct Trainer` does not grow.
7. **CrystalDust's front pics keep expansion's default mugshot coordinates and
   rotation.** CrystalDust's `front_pic_tables.h` carried per-sprite `.size` and
   `.y_offset` values; expansion's `TRAINER_FRONT_PIC` macro has no equivalent
   for front pics (it uses them only for mugshots and back pics). If a Johto
   leader's sprite sits wrong in a battle intro, this is why.

**Result:** the build is clean (exit 0) and ROM content is 29,092,768 B
(27.75 MiB, 86.7%). Only 3 trainer constants remain without a party, all of them
non-trainers.

## D51 — CrystalDust's sound engine hooks, extra fanfares and the Whirlpool field move

Phase 4 systems verification found three things the Phase 1 merge had dropped
entirely rather than merged badly.

1. **The GBS engine was unreachable.** `src/gbs.c` survived the merge but none of
   its three entry points did. Restored: `GBSMain` and `GBSTrack_Stop` calls in
   `src/m4a_1.s` (a track is a GBS track when the high nibble of
   `o_MusicPlayerTrack_patternLevel` is non-zero), and `ply_gbs_switch` in slot 5
   of `gMPlayJumpTableTemplate` in `src/m4a_tables.c`, which had reverted to
   `ply_fine`.
2. **Three fanfares and the GBS duration column were lost.** Re-added
   `FANFARE_OBTAIN_EGG`, `FANFARE_PKMNCHANNEL_INTERLUDE` and
   `FANFARE_RG_CAUGHT_INTRO`, plus `durationGBS` on every `sFanfares[]` row;
   `PlayFanfare` picks the column on `FLAG_SYS_GBS_ENABLED` because the GBS
   arrangements run shorter than the m4a ones.
3. **Whirlpool did not exist.** `MB_WHIRLPOOL`, `MetatileBehavior_IsWhirlpool`,
   `SetUpFieldMove_Whirlpool`, `FldEff_UseWhirlpool` and
   `EventScript_UseWhirlpool` all had zero references in our tree. Restored via
   a new `src/fldeff_whirlpool.c`, `FLDEFF_USE_WHIRLPOOL` (83),
   `gFieldEffectScript_UseWhirlpool`, and CrystalDust's scripts appended to
   `data/scripts/field_move_scripts.inc`.

### Constraint decisions

- **D51.1 — Hoenn's `MB_BRIDGE_OVER_POND_LOW` is forfeited.** CrystalDust puts
  `MB_WHIRLPOOL` at behavior 0x71, which collides with it. CrystalDust's Johto
  general tileset already uses 0x71 for whirlpool tiles, so CrystalDust wins per
  the standing steer. `#define MB_BRIDGE_OVER_POND_LOW MB_WHIRLPOOL` keeps
  Hoenn's name compiling; `MetatileBehavior_IsBridgeOverWater` and
  `GetBridgeType` now exclude it. The low bridges on Hoenn Route 119/120 and
  Pacifidlog would behave as whirlpools — out of scope under D37 anyway.
- **D51.2 — Whirlpool is gated on the Glacier Badge** (`FLAG_BADGE07_GET`) as
  HM06 is in Gen 2, via `BADGE_UNLOCK` in `gFieldMoveInfo[]`.
- **D51.3 — Headbutt had no party-menu entry either.** `SetUpFieldMove_Headbutt`
  existed but nothing referenced it; added `FIELD_MOVE_HEADBUTT` as
  `ALWAYS_UNLOCKED` (it is a TM, not an HM). Both `SetUpFieldMove_*` functions
  were widened from `bool8` to `bool32` to match expansion's `gFieldMoveInfo`
  function-pointer type.
- **D51.4 — Whirlpool plays no sound effect.** CrystalDust's `PlaySE(SE_M_WHIRLPOOL)`
  is commented out upstream; kept as-is rather than inventing a cue.

**Result:** build exit 0; ROM content 29,097,280 B (27.75 MiB, 86.7%).

## D52 — CrystalDust's tileset animations (D44 follow-up)

`src/tileset_anims.c` was entirely expansion's — pokeemerald's Hoenn file plus
expansion's FRLG additions, with nothing from CrystalDust. `data/tilesets/headers.inc`
*is* CrystalDust's and byte-identical to theirs, but expansion moved tileset headers
to `src/data/tilesets/headers.h`, so it is an orphan the build never reads
(the orphaned-header variant of the Phase 1 merge failure, again). The result was
18 tilesets with `.callback = NULL` and a general tileset animated at Hoenn's tile
offsets. All 45 of CrystalDust's animation graphics directories were already present
— only the code that points at them was lost.

Restored: the 12 Johto `InitTilesetAnim_*` entry points (New Bark, Violet, Azalea,
Goldenrod, Azalea Gym, Goldenrod Gym, Blackthorn Gym, Pagoda/Sprout Tower, Pokémon
Day Care, National Park, Pokémon League, Dragon's Den Shrine), their 8 callbacks,
15 queue helpers and 15 frame tables, plus the 18 tileset-header callbacks.

### Constraint decisions

- **D52.1 — Hoenn's shore and waterfall animations are dropped.**
  `QueueAnimTiles_General_SandWaterEdge`, `_Waterfall` and `_LandWaterEdge` had no
  corresponding tiles in CrystalDust's general tileset; CrystalDust animates water,
  fast water, flowers and **whirlpools** there instead. The PNG frame directories
  are left in the tree but are no longer referenced. Primary counter max goes
  256 → 640 to fit CrystalDust's longer cycle, and the flower cycle 4 → 5 frames.
- **D52.2 — `gTileset_Cave`, `_BikeShop` and `_VermilionGym` lose their expansion
  callbacks.** CrystalDust sets all three to `NULL`; its versions of those tilesets
  have no animated tiles, so keeping expansion's callbacks would have written
  animation frames over static tiles. CrystalDust wins per the standing steer.
- **D52.3 — The Battle Frontier flower and Battle Dome floor-light animations were
  left on expansion's versions.** CrystalDust has its own, but the Battle Frontier
  is out of scope for now (Q3), and swapping them is pure risk with no visible
  benefit until it is back in.

**Result:** build exit 0; ROM content 29,115,232 B (27.77 MiB, 86.8%), +17,952 B.

## D53 — CrystalDust's day/night tileset palette overrides (D31 follow-up)

`gPaletteOverrides` and `LoadPaletteOverrides` survived Phase 1 and run every
frame, but the array was permanently `{NULL, NULL, NULL, NULL}`: the data lived in
`data/tilesets/overrides.inc`, another file expansion orphaned when it moved
tileset headers out of assembly (same failure as D52). Windows stayed dark at
night, street lamps unlit, and Goldenrod's neon never came on.

Generated `src/data/tilesets/palette_overrides.h` from `overrides.inc` and the
`gTilesetPalOverride_*` incbins in `graphics.inc` — 29 override palettes and 19
per-tileset tables, all of whose `_over.pal` sources were already in the tree.
Added `const struct PaletteOverride *paletteOverrides` at offset 0x18 of
`struct Tileset`, wired the 22 tilesets that have overrides, and restored the three
`gPaletteOverrides[n] = tileset->paletteOverrides` assignments in
`LoadTilesetPalette`.

### Constraint decisions

- **D53.1 — The override palettes are `static` in `palette_overrides.h`.**
  CrystalDust exported them globally from assembly; nothing outside `tilesets.c`
  referenced them, so they stay file-local rather than adding 29 externs.
- **D53.2 — `gPaletteOverrides[3]` is still never written.** CrystalDust only ever
  populates slots 0–2 (primary, secondary, compressed); the fourth slot is unused
  upstream too. Kept for struct compatibility.

**Result:** build exit 0; ROM content 29,117,280 B (27.77 MiB, 86.8%), +2,048 B.

## D54 — Silph Co.'s tileset

Auditing every field of `src/data/tilesets/headers.h` against CrystalDust's
orphaned `data/tilesets/headers.inc` turned up one more substitution:
`gTileset_SilphCo` was pointing at FRLG's `gTilesetTiles_Condominiums` and
`gTilesetPalettes_Condominiums`, while CrystalDust's own
`data/tilesets/secondary/silphco/` tiles and 16 palettes sat unreferenced in the
tree. Added both as C and repointed the header. (`BurnedTower` also differed, but
only because CrystalDust misspells the symbol `gMetatilesAttributes_`; same data.)

That audit closes the orphaned-`.inc` sweep: of 132 non-map `.inc` files, exactly
four are unreachable from `data/*.s` — `graphics.inc`, `headers.inc`,
`metatiles.inc` and `overrides.inc` — and all four have now been reconciled
against their C replacements. `metatiles.inc` had no losses.

**Result:** build exit 0; ROM content 29,117,280 B (27.77 MiB, 86.8%).

## D55 — Headbutt battle cosmetics (D26 follow-up), and D45 closed

**D26's two deferred behaviours are in.** `BATTLE_TYPE_TREE` was being set by
`battle_setup.c` but nothing read it:

- `battle_message.c` now prints `"<mon> fell out\nof the tree!"` instead of the
  generic wild-encounter line.
- `SpriteCB_WildMonAnimate` suppresses the cry and passes `arg3 |= 0x80` when the
  mon is asleep, so a Headbutt encounter on a sleeping mon stays silent.

**D45 is closed with no action.** CrystalDust's `gCryTable2` is simply
pokeemerald's `gCryTable_Reverse` under its pre-rename name; `src/sound.c` already
calls the right table. Our cry table has 1,159 entries to CrystalDust's 388
because expansion ships every generation's cries, so no Gen 2 cry is missing.

**Result:** build exit 0; ROM content 29,123,872 B (27.77 MiB, 86.8%).

## D56 — The two "missing graphics" are both non-issues

- `graphics/pokemon/question_mark/footprint.1bpp` — the `.png` source is present;
  `src/data/graphics/pokemon.h` picks it or `footprint_gba.png` by config.
- `graphics/interface/hp_numbers.4bpp.lz` — CrystalDust's `LoadBattleBarGfx`
  LZ-decompresses a pre-rendered HP-digit sheet into `barFontGfx`. Expansion
  renders the same digits at runtime with `RenderTextHandleBold(..., FONT_BOLD, ...)`
  in `battle_interface.c`, so the asset is obsolete rather than lost. No action.

## D57 — D2's charmap residue closed

D2 left two things open: 44 Hoenn `MUS_*`/`SE_*` charmap aliases, and three
battle placeholders said to "need real work". Both are now resolved, and one was
a genuine latent bug.

**The real bug: charmap still held CrystalDust's original song IDs.** D14
renumbered CrystalDust's 56 songs to indices 610–665, but `charmap.txt` was never
updated, so `{MUS_AZALEA}` in any string would have assembled to 427 —
expansion's `MUS_DEWFORD`. Nothing references them today (the sweep D2 called for
found exactly one `{MUS_*}` placeholder in the whole tree, `{MUS_LEVEL_UP}` in
Hoenn's Route 23 script, which is correct and out of scope anyway), so this never
fired — but it was a landmine for every Phase 5 text edit. All 56 entries are
repointed; all 542 song names in charmap now agree with `songs.h`, and no two
names share a byte value.

**The three battle placeholders need no work after all.**
`B_SCR_ACTIVE_NAME_WITH_PREFIX` (FD 13) and `B_SCR_ACTIVE_ABILITY` (FD 1A) are
CrystalDust's names for exactly the codes expansion calls `B_SCR_NAME_WITH_PREFIX`
and `B_SCR_ABILITY` — same battler, same semantics, so the shared byte value is a
correct alias rather than a collision. `B_BUG_CONTEST_MON` (FD 35) is unused in
CrystalDust itself, so `FD 35` keeps expansion's `B_ATK_TRAINER_NAME`. The three
`@ RETIRED by Crystal Expansion` comments in `charmap.txt` claimed the opposite
and were actively misleading — expansion's names are still defined and used
hundreds of times in `battle_message.c` — so they have been corrected.

**`FREE_MATCH_CALL` stays `FALSE`, and this is not optional.** CrystalDust's
phone system (D47) is built on top of `match_call.c`: `gPhoneContacts` is indexed
against `sMatchCallState` and `GetTrainerMatchCallId`. Setting the flag would free
104 save bytes and break the phone.

**Result:** build exit 0; ROM content 29,123,936 B (27.77 MiB, 86.8%).

### D58 Johto Pokédex ordering restored; corrects D24

**Loss.** CrystalDust carries a real Johto Pokédex order — `gJohtoToNationalOrder`
plus `NationalToJohtoOrder`, `JohtoToNationalOrder` and `SpeciesToJohtoPokedexNum`
in `src/pokemon.c`, driven by 464 `JOHTO_DEX_*` constants in its `species.h`. The
Phase 1 merge kept expansion's file, so none of it survived. Our `pokedex.c`
carried a comment claiming "the Johto Dex is simply National #1-251 in national
order, so no reordering table is needed. See D24" — that is wrong. The Johto Dex
runs Chikorita (1) … Snorlax-era Johto natives … Bulbasaur (226) … Mew (250),
Celebi (251). Every dex count, the numerical list order and the printed dex
number were showing National numbering.

**Fix.** `include/constants/pokedex.h` gains `FOREACH_SPECIES_IN_JOHTO_DEX_ORDER`
(251 entries lifted from CrystalDust's table) and `enum JohtoDexOrder`, mirroring
expansion's Kanto idiom; `JOHTO_DEX_COUNT` is now `JOHTO_DEX_CELEBI` rather than
`NATIONAL_DEX_CELEBI` (same value, correct meaning) and `REGIONAL_DEX_COUNT` is
Johto's, not Hoenn's. `src/pokemon.c` gains `sJohtoToNationalOrder` and the three
lookup functions, and the three `*Regional*` dispatchers now fall through to
Johto instead of Hoenn. `src/pokedex.c`'s `GetJohtoPokedexCount` walks through
`JohtoToNationalOrder`, and `HasAllJohtoMons` (missing entirely; CrystalDust uses
it for the trainer-card star and `def_special HasAllJohtoMons`) is restored,
excluding Celebi. The special is re-registered in `data/specials.inc`.

**Constraint noted.** Expansion's trainer card field is still spelled
`caughtAllHoenn`; it now holds the Johto result. Renaming it touches the save
struct, so it stays as-is — cosmetic only.

Verified against the linked ROM: `sJohtoToNationalOrder[0..2]` = 152/153/154,
`[225]` = 1, `[249..250]` = 150/151, `[250]` = 251.

### D59 Player is Gold/Kris; trainer card shows Johto badges

**Loss.** The merge kept expansion's trainer-card and trainer-class data, so the
player was Brendan/May throughout: `PlayerGenderToFrontTrainerPicId`, the debug
sprite helper, the trainer card's `sTrainerPicFacilityClass`, and the two
PokéNav Match Call entries all pointed at Hoenn's protagonists. Worse, the card
drew `graphics/trainer_card/badges.png` — the **Hoenn** badges — in a Johto game.
`kris_front_pic.png` was on disk with no `TRAINER_PIC_KRIS` constant, no entry in
`gTrainerFrontPicTable` and no facility class: the table-lost-but-assets-kept
variant again.

**Fix.** `TRAINER_PIC_KRIS` added to the pic enum with its `gTrainerFrontPic_GscKris`
/ `gTrainerPalette_GscKris` pair; `FACILITY_CLASS_GOLD` and `FACILITY_CLASS_KRIS`
appended to the facility-class enum (appended, not inserted — the values index
link/Battle-Tower save data) and mapped in `trainer_class_lookups.h`. All five
Brendan/May player sites now resolve to Gold/Kris. The Hoenn card's badge
graphics and palette are repointed to `badges_johto.png`, same 128x16 4bpp
dimensions, so the layout is untouched.

**Deferred, not dropped.** CrystalDust also ships a whole third card type,
`CARD_TYPE_CRYSTALDUST`, with its own tilemaps, palettes and stat layout — the
`*_cd` assets, all of which are present in our tree and currently referenced by
nothing. Adding a third type means touching every one of expansion's binary
`cardType != CARD_TYPE_FRLG` branches, which is a Phase 7 visual job rather than
a Phase 4 correctness one. Until then the card keeps expansion's Emerald layout
(consistent with Q2) but with Johto content. Tracked as a Phase 7 item.

### D60 Furniture, radio and window metatile behaviours restored

**Loss.** CrystalDust's tilesets ship metatile attributes that reference six of
its own metatile behaviours. The merge kept expansion's
`constants/metatile_behaviors.h`, so those attribute bytes survived in the `.bin`
files while the names, the `MetatileBehavior_Is*` predicates and the
`GetInteractedMetatileScript` hooks all went. The scripts themselves survived
(`data/scripts/check_furniture.inc`, the New Bark Town 2F map script), reachable
only from object interactions — the tiles did nothing.

Affected, with live occurrences counted in our own `metatile_attributes.bin`
files: `MB_RADIO` 0x79 (4 tiles, `players_house`/`playersroom`/`building`),
`MB_WINDOW` 0x88 (15 tiles across nine Johto tilesets), `MB_DISTINGUISHED_STATUE`
0xDF (1, `pagoda_tower`), `MB_ANCIENT_POKEMON_REPLICA` 0xDE, and
`MB_DECOR_POSTER`/`MB_DECOR_CONSOLE` (13 and 2 in `playersroom`).

**Collision, and how it was resolved.** CrystalDust's poster and console sit at
0xEB and 0xEC, which expansion already uses for `MB_UP_RIGHT_STAIR_WARP` and
`MB_UP_LEFT_STAIR_WARP` — and the FRLG tilesets in our tree do use them as stair
warps. Naming them CrystalDust's way would have made every poster in the player's
bedroom a stair warp. The poster and console were therefore **relocated to the
free 0xDC/0xDD slots**, and `playersroom/metatile_attributes.bin` was patched
accordingly (15 entries). Expansion's own unreferenced `MB_WINDOW` at 0xAB became
`MB_UNUSED_AB` so CrystalDust's could take 0x88; nothing referenced it.

This is the first case where the two sides' behaviour numbering actually
conflicted rather than one simply being absent, so it is worth re-checking the
remaining `MB_UNUSED_*` slots if more CrystalDust tiles surface.

### D61 `walk_fastest` movement actions restored

**Loss.** CrystalDust adds four movement actions — the normal walking animation
played at `MOVE_SPEED_FASTEST`, distinct from expansion's `WALK_FASTER`. The
merge kept expansion's `event_object_movement` constants, implementation and
dispatch table, and an earlier phase had quietly downgraded the six script uses
to `walk_faster_*` so the scripts would still assemble. That substitution was
never recorded, which is exactly the silent drop the project rules forbid.

**Fix.** `MOVEMENT_ACTION_WALK_FASTEST_{DOWN,UP,LEFT,RIGHT}` appended at
0xBE–0xC1 (appended, not inserted at CrystalDust's 0x2D–0x30, which expansion
already uses), the four `MovementAction_WalkFastest*` pairs added, the func
tables and `asm/macros/movement.inc` extended, and the six downgraded script
lines restored across Cerulean Gym, Fast Ship B1F, Lance's Room and the Route 40
frontier gate.

### D62 Bedroom PC turned on the wrong metatile

**Loss.** CrystalDust defines `PC_LOCATION_PLAYERS_HOUSE = 1` and swaps the
bedroom PC between metatiles 0x3 and 0xD of the `playersroom` tileset. The merge
kept expansion's `enum PCLocation`, in which 1 is `PC_LOCATION_BRENDANS_HOUSE`.
New Bark Town's `EventScript_PlayerPC` still passes a literal `1` — the constant
is a C enum and so cannot be named from the assembled scripts — so booting the PC
in the player's bedroom stamped Brendan's Hoenn PC metatile over it, in all three
of `PCTurnOnEffect_SetMetatile`'s branches and in `PCTurnOffEffect`.

**Fix.** Slot 1 is renamed `PC_LOCATION_PLAYERS_HOUSE` and given CrystalDust's
0x3/0xD, matching the literal the script already passes. `PC_LOCATION_MAYS_HOUSE`
stays at 2; no script in the tree references it now that Hoenn is cut.

### D63 Pokédex area screen matched no maps at all

**Loss.** `pokedex_area_screen.c` decides which maps to glow or mark by comparing
each wild-encounter header's **map group** against three anchors. The merge kept
expansion's, which anchor on `MAP_PETALBURG_CITY`, `MAP_METEOR_FALLS_1F_1R` and
`MAP_SAFARI_ZONE_NORTHWEST`. With Hoenn cut (D37) those constants are stubs
resolving to group 112, so every comparison failed: the area screen showed
nothing for any species, in a game whose dex is the point.

**Fix.** Repointed to CrystalDust's anchors — `MAP_VIOLET_CITY` (group 0),
`MAP_UNION_CAVE_1F` (26) and `MAP_BATTLE_FRONTIER_OUTSIDE_EAST` (58); our map
group numbering already matches CrystalDust's exactly. `MAP_GROUP_TOWNS_AND_ROUTES_FRLG`
is dropped: Kanto and Johto towns share group 0 here, so it duplicated the Johto
case label and the compiler caught it. The Feebas special-case row, which pointed
at Hoenn's Route 119, is now `MAP_UNDEFINED` as in CrystalDust.

## D64 — Roaming legendaries roam Johto, and it's Raikou and Entei

**Found:** the stub-Hoenn-anchor sweep opened after D63.

`src/roamer.c` kept expansion's side wholesale. Two losses, both silent:

1. `sRoamerLocations` listed Hoenn routes 110–134. Those map constants are now
   stubs resolving to a nonexistent map group, so the roamer sat on a map the
   player can never reach — roaming was dead.
2. `InitRoamer` spawned Latias or Latios off `gSpecialVar_0x8004`. But the only
   caller is `data/maps/BurnedTower_B1F/scripts.pory`, CrystalDust's beast
   release, which passes no such variable. Releasing the beasts gave you an
   Eon duo instead.

**Done:** ported CrystalDust's 16 Johto route sets (Routes 29–39, 42–46) into
expansion's 6-column/`___` table shape, and rewrote `InitRoamer` to
`DeactivateAllRoamers()` then add Raikou and Entei at level 40. `RegenerateRaikou`
/ `RegenerateEntei` / `IsRaikouActive` / `IsEnteiActive` from D48 now have a
matching spawn path.

**Two deliberate divergences from CrystalDust, both flagged rather than silent:**

- *Route 39 gains Route 42 as a third destination.* CrystalDust's Route 39 set
  offers only Route 38. Both engines pick a destination in a `do/while` that
  rejects the map the player stood on two moves ago, so a player who was on
  Route 38 two moves ago hangs the game. This is a live bug in CrystalDust; the
  third entry is the minimal fix and matches the invariant expansion documents
  above the table.
- *Suicune does not roam.* CrystalDust's `NUM_ROAMERS` is 2 and its
  `CreateInitialRoamerMon` loop computes `species += i`, which yields Raikou,
  Entei, then Larvitar — a bug that never fires because the loop stops at 2.
  Suicune is scripted (Burned Tower, Routes 36/42, Bell Tower), not roamed, so
  the `ROAMER_SUICUNE` slot from D48 stays reserved and unused.

Build exit=0, ROM 29,126,724 B (86.80%).

## D65 — Pokémon Centers set the save-warp again

**Found:** stub-Hoenn-anchor sweep, `src/save_location.c`.

`sSaveLocationPokeCenterList` kept expansion's 39 Hoenn maps. Those constants are
stubs now, so `IsCurMapPokeCenter()` was never true: `POKECENTER_SAVEWARP` never
got set, and saving inside a Center did not register it as your reload point.

**Done:** replaced the list with CrystalDust's 20 Johto Centers plus the link-room
entries it keeps (`BATTLE_COLOSSEUM_2P`, `TRADE_CENTER`, `RECORD_CORNER`,
`BATTLE_COLOSSEUM_4P`, `BATTLE_FRONTIER_POKEMON_CENTER_1F/2F`).

**One deliberate addition, flagged:** CrystalDust's own list stops at Blackthorn
and omits all ten Kanto Pokémon Centers, even though it ships Kanto. That looks
like an oversight rather than a design choice — it would leave Kanto Centers not
registering as save warps. The 20 Kanto Center maps are added.

`sSaveLocationReloadLocList` (Battle Tower lobby) is unchanged; it is parked with
the rest of the Frontier under Q3.

Build exit=0, ROM 29,126,724 B (86.80%).

## D66 — A new game starts in New Bark Town

**Found:** stub-Hoenn-anchor sweep, `src/new_game.c`.

`WarpToTruck` still warped to `MAP_INSIDE_OF_TRUCK` (or `MAP_PALLET_TOWN_PLAYERS_HOUSE_2F`
under `IS_FRLG`). Both are stub constants resolving to a map group that no longer
exists, so a brand-new game had no valid starting map.

**Done:** warps to `MAP_NEW_BARK_TOWN_PLAYERS_HOUSE_2F` with CrystalDust's own
arguments (`WARP_ID_NONE, -1, -1`), matching `sources/crystaldust/src/new_game.c:134`.
The `IS_FRLG` branch is dropped — there is no FRLG start in a Johto game.

The function keeps its upstream name for now; renaming it touches the forward
declaration and reads as churn. Noted for the Phase 5 sweep.

Build exit=0, ROM 29,126,724 B (86.80%).

## D67 — Fly goes to Johto

**Found:** stub-Hoenn-anchor sweep, `src/region_map.c`.

Expansion's side survived Phase 1 intact, so the whole Fly path was Hoenn's:

- `sMapHealLocations` — 39 Hoenn rows of stub map constants. Every Fly target
  resolved to a nonexistent map.
- `GetMapsecType` — the Hoenn town cases can never be reached now, so no Johto
  town ever reported `MAPSECTYPE_CITY_CANFLY`. The Kanto cases were fine.
- `sFlyLocations` — the Fly-icon table, 17 Hoenn entries.
- `FilterFlyDestination` — Littleroot's gendered house, Ever Grande's league and
  Southern Island.

**Done:** ported CrystalDust's 24-row `sMapHealLocations` (10 Johto towns, Route 32's
Pokémon Center, Lake of Rage, Silver Cave, 10 Kanto cities, Indigo Plateau via
Route 23); replaced the Hoenn `GetMapsecType` cases with the Johto ones keyed on
`FLAG_VISITED_*` / `FLAG_LANDMARK_*`; replaced the Hoenn `sFlyLocations` block
with the matching 13 Johto entries; and dropped the Hoenn special cases from
`FilterFlyDestination`, which CrystalDust does not have.

`MAPSEC_ROUTE_3_FLYDUP` / `MAPSEC_ROUTE_10_FLYDUP` are still absent — the mapsec
table is at its 252-entry ceiling, per D33. Kanto's Routes 3 and 10 therefore Fly
to their single map section rather than CrystalDust's split pair.

**Rename:** `REGION_MAP_HOENN` → `REGION_MAP_JOHTO`. That enumerator is the "home
region" slot and `GetRegionMapType` already returns it for every non-Kanto
section; the old name was actively misleading. Two files touched.

The Sevii entries in `sFlyLocations` are left in place but unreachable (D35).

Build exit=0, ROM 29,126,468 B (86.80%).

## D68 — Hoenn's weather-trio music no longer fires on Johto maps

**Found:** stub-Hoenn-anchor sweep, `src/overworld.c`.

`ShouldLegendaryMusicPlayAtLocation` tested `warp->mapGroup == 0` and then
switched on bare `MAP_NUM(...)` of Hoenn constants. Those constants are stubs
now, and `MAP_NUM` takes the low byte — which lands squarely on real Johto and
Kanto maps in group 0:

| Hoenn stub | low byte | actually matches |
|---|---|---|
| `MAP_LILYCOVE_CITY` | 5 | Ecruteak City |
| `MAP_MOSSDEEP_CITY` | 6 | Olivine City |
| `MAP_SOOTOPOLIS_CITY` | 7 | Cianwood City |
| `MAP_EVER_GRANDE_CITY` | 8 | Mahogany Town |
| `MAP_ROUTE124`–`MAP_ROUTE131` | 39–46 | Cinnabar, Saffron, Routes 1–6 |

So once `FLAG_SYS_WEATHER_CTRL` was set, `MUS_ABNORMAL_WEATHER` would have
replaced the normal music in half of Johto and Kanto. This is the first stub-anchor
case that misfires rather than silently doing nothing — worth calling out as a
new sub-variant of the Phase 1 pattern.

**Done:** deleted `ShouldLegendaryMusicPlayAtLocation`,
`NoMusicInSootopolisWithLegendaries`, `IsInfiltratedSpaceCenter` and
`IsInfiltratedWeatherInstitute` along with their four calls in `GetLocationMusic`.
CrystalDust stubs all four to `return FALSE` and comments the calls out; deleting
them is the same behaviour without the dead weight. Map-header music and D7's
night-music lookup are untouched.

**Not changed, and why:** the other Hoenn anchors left in `overworld.c`
(`MetatileBehavior_IsSurfableInSeafoamIslands`, the Route 111 sandstorm check, the
Mauville and Sootopolis warp checks, the Route 130 Mirage Island check) all
compare `mapGroup` against the stub's *group* byte before looking at `mapNum`.
Group 112/113/117/118 match no real map, so they are inert rather than wrong.
They are left alone rather than churned.

Build exit=0, ROM 29,126,372 B (86.80%).

## D69 — The TV in the player's house works again

**Found:** stub-Hoenn-anchor sweep, `src/tv.c`.

`CheckForPlayersHouseNews` and `GetMomOrDadStringForTVMessage` both gated on
`MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_1F` / `MAYS_HOUSE_1F`. Those are stubs, so the
group test failed immediately: the TV downstairs in New Bark Town never showed the
home movie or the roaming-legendary news, and the TV narration never resolved to
"Mom".

**Done:** both now test `MAP_NEW_BARK_TOWN_PLAYERS_HOUSE_1F`.

**Divergence from CrystalDust, flagged:** CrystalDust's own copies keep Emerald's
gender split and send the female branch to `NEW_BARK_TOWN_ELMS_HOUSE` — Gold gets
the TV news, Kris gets it in Professor Elm's house instead of her own. That is a
stale find-and-replace on their side: Gold and Kris live in the same house. The
gender split is dropped rather than reproduced.

Build exit=0, ROM 29,126,308 B (86.80%).

## D70 — The department store elevator is Goldenrod's

**Found:** stub-Hoenn-anchor sweep, `src/field_specials.c`.

`SetDeptStoreFloor` switched on `dynamicWarp.mapNum` alone, with no map-group
guard, against stub `MAP_LILYCOVE_CITY_DEPARTMENT_STORE_*` constants — the same
misfire shape as D68, so it could set `VAR_DEPT_STORE_FLOOR` from an unrelated
map whose number happened to collide. `GetDeptStoreDefaultFloorChoice` did guard
on the group, so it was merely inert.

**Done:** both now use `MAP_GOLDENROD_CITY_DEPT_STORE_*` — eight floors
(B1F, 1F–6F, rooftop) for `SetDeptStoreFloor` and CrystalDust's seven-entry
default-cursor order (6F first down to B1F) for `GetDeptStoreDefaultFloorChoice`.

**Still open, not dropped:** the Goldenrod and Celadon scrollable-multichoice
floor lists from D48.12 are still parked — the store scripts have not landed yet,
and `SCROLL_MULTI_GOLDENROD_DEPT_STORE_FLOORS` has no case block. This decision
fixes the elevator's floor bookkeeping, not the floor menu. Celadon has no
department store maps in the tree at all yet.

The static's name is still `sLilycoveDeptStore_DefaultFloorChoice`; renaming is
Phase 5 text-sweep work.

Build exit=0, ROM 29,126,404 B (86.80%).

## D71 — Swarms are CrystalDust's Pokégear calls, not Emerald's TV outbreaks

**Phase 4.** `src/mass_outbreak.c` carried expansion's five Hoenn outbreaks
(`OUTBREAK_ID_ROUTE102` … `ROUTE116`), every `.location` a stub map constant, so
no static outbreak could ever resolve to a real map. CrystalDust's own swarm
system — announced by Pokégear phone calls in `match_call.c` — was ported in D20
but had never been wired to the encounter code, and carried two upstream bugs.

Five things changed:

1. **`sPokeOutbreakSpeciesList` is now Johto's three swarms** — Dunsparce in
   Dark Cave (Anthony), Qwilfish on Route 32 (Ralph), Yanma on Route 35 (Arnie).
   These mirror the phone-call data exactly. Gen 2's other swarms (Remoraid,
   Marill, Snubbull) were never implemented in CrystalDust either, so none are
   invented here — see the constraint note below.
2. **`.location_map_group` was set with `MAP_NUM`** in all three CrystalDust
   phone-call structs. Fixed to `MAP_GROUP`.
3. **`MatchCall_StartMassOutbreak` never stored the map group at all** (the line
   was commented out with "Map group seems not to be used"). It works upstream
   only because every swarm route happens to be in group 0 and the field is
   zeroed on end. Now stored properly.
4. **Swarms respect their encounter kind.** `DoMassOutbreakEncounterTest` takes
   a `wildState`, so Ralph's Qwilfish is fished up rather than walking out of the
   grass on Route 32. `src/fishing.c` gains the guaranteed-bite path and
   `FishingWildEncounter` takes an `outbreakCaught` flag; rod choice picks
   `specialLevel1` (Old Rod, lv5), the base level (Good Rod, lv20) or
   `specialLevel2` (Super Rod, lv40).
5. **Moveless swarms keep their level-up moveset.** `SetUpMassOutbreakEncounter`
   blanked all four move slots unconditionally; the phone-call swarms carry no
   move list, so it now only overwrites when one is present.

`OUTBREAK_WALKING` / `OUTBREAK_SURFING` / `OUTBREAK_FISHING` moved from
`include/match_call.h` to `include/constants/mass_outbreak.h`.

**Constraint decisions (nothing silently dropped):**

- **Surf swarms are unreachable.** `OUTBREAK_SURFING` exists in CrystalDust's
  enum but no swarm uses it and expansion's water encounter path has no outbreak
  hook. Left unwired rather than invented.
- **The TV mass-outbreak show now advertises Johto.**
  `PrepareTvShowForRandomOutbreak` picks from the same three-entry table, so
  Emerald's TV segment survives with CrystalDust content rather than Hoenn's.
- **Three of Gen 2's six swarms are absent** (Remoraid/Route 44, Marill/Route
  42, Snubbull/Route 38, delivered by Tully, Arnie's relatives and Derek in the
  original). CrystalDust never implemented them; adding them would mean writing
  new phone-call text, which belongs in Phase 6 with the rest of CD's TODO.

## D72 — Post-credits landing, PC release guard, and the rest of the stub-anchor sweep

**Phase 4.** Two live fixes plus the close-out of the stub-Hoenn-anchor sweep
opened after D63.

**Fixed:**

- **`GameClear` in `src/post_battle_event_funcs.c`** set the continue-game warp
  to `HEAL_LOCATION_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F` /
  `..._MAYS_HOUSE_2F` by gender. This is the live path — the Hall of Fame
  script calls `special(GameClear)` — so after the credits the player resumed in
  a Hoenn bedroom that no longer exists. Now `HEAL_LOCATION_NEW_BARK_TOWN`,
  matching CrystalDust, and the gendered split is gone with the shared house
  (see D69). `EnterHallOfFame` (the FRLG variant, currently unreachable) had
  `HEAL_LOCATION_PALLET_TOWN` and was corrected to match.
- **`sRestrictedReleaseMoves` in `src/pokemon_storage_system.c`** stopped you
  releasing your Strength or Rock Smash user inside Hoenn's Pokémon League,
  both stub maps. Replaced with one row for Johto's Victory Road, the only map
  in CrystalDust where releasing your Strength user can strand you. The two
  `MAP_GROUPS_COUNT` rows (Surf, Dive — restricted everywhere) were always fine.

**Checked and deliberately left alone — none is lost CrystalDust content:**

| File | Stub anchor | Why it is not a loss |
| --- | --- | --- |
| `follower_helper.c` | 10 `MATCH_MAP` conditional-message rows (Ever Grande, Route 112, Route 117 Day Care, Mauville Bike Shop, New Mauville, Stern's Shipyard…) | Expansion's follower flavour text, group-guarded so it simply never fires. Writing Johto equivalents is Phase 7 polish, not a port gap. |
| `wild_encounter.c` | `CheckFeebasAtCoords` (Route 119), `AreLegendariesInSootopolisPreventingEncounters` | Feebas and the Kyogre/Groudon standoff are Hoenn-only; CrystalDust has neither. Both group-guarded. |
| `braille_puzzles.c`, `mirage_tower.c` | Sealed Chamber, Desert Ruins, Ancient Tomb, Island Cave, Route 111 | The Regi and Mirage Tower questlines went with Hoenn (D37). |
| `seagallop.c` | the six island harbours | Sevii was cut (D35). |
| `credits_frlg.c`, `hall_of_fame_frlg.c` | Route 21 North, Indigo Plateau Exterior | The FRLG credits and Hall of Fame are unreachable; CrystalDust uses the Emerald path. Left as dead code rather than deleted, since Phase 7 may want the FRLG credits scroller. |

That closes the sweep: no unguarded `MAP_NUM(stub)` use remains anywhere in
`src/`, and every guarded one is now either repointed at Johto or recorded above
as intentionally dead.

**Constraint decision:** the ten follower conditional messages and the FRLG
credits/Hall-of-Fame paths are features that exist in the tree but can never
trigger. They are not dropped — they are parked, and listed here so Phase 7 can
pick them up.

## D73 — The Moss Rock and Ice Rock move to Johto

**Phase 4.** Three location-gated evolutions pointed at stub Hoenn maps and so
could never trigger:

- **Leafeon** — `MAP_PETALBURG_WOODS`
- **Glaceon** — `MAP_SHOAL_CAVE_LOW_TIDE_ICE_ROOM`
- **Crabominable** — `MAP_SHOAL_CAVE_LOW_TIDE_ICE_ROOM`

Repointed to `MAPSEC_ILEX_FOREST` and `MAPSEC_ICE_PATH`, the natural Johto
stand-ins for the Moss Rock and Ice Rock. `IF_IN_MAPSEC` is used rather than
`IF_IN_MAP` so every floor of the Ice Path counts, instead of the player having
to find the one correct room.

All three keep their evolution-stone alternative (`ITEM_LEAF_STONE`,
`ITEM_ICE_STONE`), so none was fully unobtainable before this — but the
location method was dead, and with it the Gen 4 way of getting them.

**Constraint decision:** Johto has no in-game Moss Rock or Ice Rock object, so
these are whole-area conditions rather than a specific overworld landmark. If
Phase 7 adds the rocks as objects, tighten the condition back to a single map.

## D74 — 713 flags were all defined as zero

**Phase 4.** A new and much larger instance of the Phase 1 pattern, found by
scanning `include/constants/flags.h` for colliding values rather than by
chasing map constants.

The `// FRLG flags` block in `flags.h` held **713 `#define FLAG_X 0`** lines.
Zero is not a spare flag — it is `TEMP_FLAGS_START`, i.e. `FLAG_TEMP_1`, aliased
as `FLAG_TEMP_SKIP_GABBY_INTERVIEW`. Temp flags are **cleared on every map
load**. So every one of those 713 names read and wrote the same single bit, and
that bit was wiped each time the player walked through a door.

**165 of them are actually referenced** by code or scripts in this tree, and
several are live CrystalDust content, not FRLG leftovers:

- `FLAG_OPENED_ROCKET_HIDEOUT` — used five times in
  `data/maps/MahoganyTown_Shop/scripts.pory`. The Rocket hideout under the
  Mahogany souvenir shop would have re-sealed itself on every map change.
- `FLAG_GOT_TM19_FROM_ERIKA` — `data/maps/CeladonCity_Gym/scripts.pory`. Erika's
  TM was infinitely farmable.
- `FLAG_OPENED_START_MENU` (`src/field_control_avatar.c`),
  `FLAG_SYS_ON_CYCLING_ROAD` and `FLAG_SYS_UNLOCKED_TANOBY_RUINS`
  (`data/event_scripts.s`), the eight `FLAG_HIDE_UNION_ROOM_PLAYER_n`, the four
  Saffron fan-club flags, 40 `FLAG_WORLD_MAP_*` used by `region_map.c` and
  `map_preview_screen.c`, the 18 FRLG move-tutor flags, the 21 Silph Co door
  flags and the Pokémon Mansion switch state.

All 165 now get real numbers from a new `KANTO_FLAGS_START` block placed after
`CRYSTAL_FLAGS_END`, with `FLAGS_COUNT` extended to match. Cost: 21 bytes of
SaveBlock1, which fits the remaining budget described in D12 — the build's
`SaveBlock1FreeSpace` static assert still passes.

**Constraint decision:** the other **548 stub flags stay at 0**. Nothing in the
tree references them; they belong to Sevii (cut, D35), the FRLG Silph Co and
Lorelei's-house interiors, and FRLG hidden-item lists for maps that do not
exist. Giving them save bits would spend the rest of the flag budget on content
that was deliberately cut. They are recorded here rather than deleted, so that
if Phase 7 revives any of it the aliasing is a known hazard, not a fresh
surprise.

Also fixed while verifying: `src/debug/sound_check_menu.c` referenced
`gCryTable2`, which expansion renamed to `gCryTable_Reverse`. The normal build
never linked it; the **test build did, and `make check` failed to link because
of it**. That was the only thing standing between this port and its first run of
the expansion test suite.

## D75 — the same scan applied to the var pool, and the first green test suite

D74 found 713 flags stubbed to `0`. Running the identical collision scan over
`include/constants/vars.h` / `include/constants/vars_frlg.h` turns up a smaller
but similar problem. `vars.h` includes `vars_frlg.h` and then redefines most of
it, but **93 FRLG var names are never redefined**, so they keep FRLG numbering
and land on top of unrelated Emerald vars. 26 of the 93 are still referenced.

Two of those aliases touch live code and are fixed here:

| FRLG name | aliased | why it mattered |
| --- | --- | --- |
| `VAR_PREV_TEXT_COLOR` (0x8013) | `VAR_MON_BOX_POS` | `Std_ReceivedItem` calls `EventScript_RestorePrevTextColor`, so **every item the player receives** copied the PC storage cursor into the text colour var. Now aliased to `VAR_TEXT_COLOR_BACKUP`, which is what Emerald reserved for it. Note `ScrCmd_textcolor` is a no-op outside FRLG, so the whole colour path is inert either way — this removes a bogus read, not a visible bug. |
| `VAR_MASSAGE_COOLDOWN_STEP_COUNTER` (0x4025) | `VAR_MIRAGE_RND_L` | `clock.c` rewrites the mirage seed once a day. Daisy's massage is Goldenrod content we intend to wire up in Phase 6, so it gets its own var at 0x4133 (0x4134–0x4137 remain free). |

**Constraint decision — the other 24 referenced orphans keep their aliases.**
Every one is dead-vs-dead or dead-vs-live-where-the-dead-side-never-runs:
the `VAR_MAP_SCENE_*` set (Kanto/Sevii scenes, referenced only from
`debug.inc`, `seagallop.inc`, `hall_of_fame_frlg.inc`, `cable_club_frlg.inc`,
`route23.inc`, `trainer_tower.c`, `trainer_fan_club.c`) sits on Hoenn
`*_STATE` vars whose maps are gone; the trainer-card brag states sit on
Hoenn lottery/cruise vars; `VAR_ELEVATOR_FLOOR`, the three
`VAR_RESORT_GORGEOUS_*` and `VAR_HERACROSS_SIZE_RECORD` likewise. This is a
hazard register, not a clean bill of health: **if Phase 7 revives any Sevii,
Trainer Tower or Kanto-scene content, these must be given real numbers first**,
exactly as with D74's 548 dead flags.

### The expansion test suite now runs green

First full `./tools/build.sh -j8 check` of the project. It found six real
failures, all ours, all fixed:

- **`test/save.c`** expected vanilla SaveBlock sizes. Ours are *smaller*
  (13676 / 2648 vs 15568 / 3884) despite D12/D13's grown flag and var pools,
  because the Hoenn-only blocks left with the maps. Expectations updated with a
  comment rather than deleted, so a future accidental shift still trips.
- **`test/mass_outbreak.c`** (×3) used `MAP_OLDALE_TOWN`, now a stub constant
  with no map behind it, so `MAP_NUM`/`MAP_GROUP` resolved to nothing. Switched
  to `MAP_ROUTE32`, a real map and a real swarm site (D71).
- **Item descriptions** — 15 CrystalDust key items carried FRLG's two-wide
  description lines, which overflow Emerald's 102px bag window. Rewritten to
  Emerald's three-narrow-line shape (Squirtbottle, Secret Potion, Red Scale,
  Machine Part, Clear Bell, Rainbow Wing, Silver Wing, GS Ball, Mystery Egg,
  Pass, Lost Item, Blue Card, Egg Ticket, Slowpoke Tail, GB Player). Wording
  changed, meaning preserved.
- **Map name popup** — `GOLDENROD CITY ROOFTOP` is 86px in an 80px window.
  Emerald handles Celadon by special-casing the name, but Goldenrod's dept
  store 1F shares a layout with an ordinary building, so it can't be detected
  the same way. The popup suffix is now `ROOF` instead of `ROOFTOP`, which
  fixes every rooftop at once; the elevator floor list still says ROOFTOP.

**Known issue, pre-existing and not from this work:** 32 battle tests on test
runner 5 die with `malloc.c` heap assertions, starting at "Confirm behavioural
match with other -ate abilities". Identical before and after these changes.
Parked for Phase 7.

## D76 — the new game opens with Oak, not Birch

`src/main_menu.c`'s `ACTION_NEW_GAME` still ran `Task_NewGameBirchSpeech_Init`:
a new save began in Hoenn, with Prof. Birch, a Lotad and a Torchic. That is the
single largest piece of CrystalDust content the merge had left behind.

CrystalDust does not have a separate intro file — its intro lives inside its own
fork of `src/main_menu.c`. Expansion, meanwhile, factored FireRed's intro out
into `src/oak_speech.c` behind `IS_FRLG`. This port follows expansion's shape:
the intro is now `src/oak_speech_crystal.c` / `include/oak_speech_crystal.h`,
and `main_menu.c` keeps expansion's menu, handing off with a single call to
`StartNewGameSceneCrystal(taskId)`.

What the scene does, in CrystalDust's order: the clock-set prompt ("... you woke
me up"), then Oak's welcome, the Wooper demonstration, the boy/girl choice
(Gold / Kris) and the naming screen. Text came across verbatim as
`data/text/oak_speech.inc`, included from `data/event_scripts.s`.

API drift fixed while porting, CrystalDust name on the left:

| CrystalDust | expansion |
| --- | --- |
| `CreatePicSprite2(species, otId, ...)` | `CreateMonPicSprite_Affine(species, isShiny, personality, MON_PIC_AFFINE_FRONT, ...)` |
| `AddTextPrinterForMessage_IgnoreTextColor(1)` | `AddTextPrinterForMessage(TRUE)` |
| `PrintTextArray(...)` | `PrintMenuTable(...)` |
| `CopyWindowToVram(w, 2)` / `(w, 3)` | `COPYWIN_GFX` / `COPYWIN_FULL` (same values) |
| `LoadMessageBoxGfx(0, 0xFC, 0xF0)` | `OAK_INTRO_DLG_BASE_TILE_NUM`, `BG_PLTT_ID(15)` (same values) |
| `LoadPalette(pal, 0x40, n)` | `BG_PLTT_ID(4)` (same value) |

Four helpers that `main_menu.c` keeps `static` (`CB2_MainMenu`,
`VBlankCB_MainMenu`, `LoadMainMenuWindowFrameTiles`, `DrawMainMenuWindowBorder`)
have local copies here rather than being exported, so `main_menu.c` is untouched
apart from the one hand-off line.

### Constraint decisions

- **The Birch speech is now unreachable but not deleted.** ~1000 lines of
  `src/main_menu.c` and all of `data/text/birch_speech.inc` still build into the
  ROM and still cost space. Removing them is a separate change (see the Phase 5
  text sweep); nothing calls them.
- **Expansion's FireRed intro (`src/oak_speech.c`) also stays**, still gated by
  `IS_FRLG`, so `graphics/oak_speech/` now holds *both* games' intro assets side
  by side. The filenames do not collide; CrystalDust's are `bg0`, `gold`,
  `kris`, `oak`, `map.bin`, `platform`.
- **Sixteen forward declarations and `SpriteCB_MovePlayerDownWhileShrinking`
  were dropped**, because they are declared-but-never-defined or
  defined-but-never-referenced in CrystalDust itself — leftovers from the Birch
  speech it was forked from. Behaviour matches CrystalDust exactly; no feature
  is lost with them.
- **The scene has not been run.** `mgba-perf` has no input injection (its flags
  are `-b -c -C -d -g -l -t -p -s`, `-F/-N/-T/-P/-S`), so a menu-driven path
  like this cannot be exercised headlessly. The 900-frame smoke test only proves
  the ROM boots to the title. **This needs a human play-test**: new game →
  clock set → Oak → gender choice → naming screen → New Bark Town.

Also in this gate: `src/pokedex.c`'s regional dex header said "HOENN region's
POKéDEX". The mode itself has walked the Johto order since D24/D58 — only the
label was wrong. `DEX_MODE_HOENN` keeps its expansion name; renaming the
constant is Phase 5 rename debt.

Build: exit 0, ROM 29,134,724 B (86.83%), smoke test clean, `make check` 0 FAILs
and the same 34 pre-existing runner-5 `malloc.c` crashes as before (D75).

## D77 — a new game now hides the right people, and 130 KB of Hoenn text goes

Three findings, all in the same area: what a brand-new save looks like.

**`EventScript_ResetAllMapFlags` was still Emerald's Hoenn list.** This script
runs once, from `NewGameInitData`, and decides which object events exist at the
start of a game. Ours set `FLAG_HIDE_LITTLEROOT_TOWN_BIRCHS_LAB_POKEBALL_*`,
the Winstrates, Mr Briney, the Lilycove museum patrons — for maps that no longer
exist — and never set any of CrystalDust's. That meant Elm's aide, Kurt, the
rival at Cherrygrove and Azalea, the Ilex Forest cast, Bill, Floria, Sierra, Red
and both Rocket-takeover sets started a new game in whatever state their flag
happened to be in, rather than hidden. Replaced wholesale with CrystalDust's
list. This is another "Phase 1 merge kept one side".

*Constraint decision:* CrystalDust's list also contained `FLAG_UNUSED_0x2F8`.
In our `flags.h` that number is a live flag (`FLAG_HIDE_LITTLEROOT_TOWN_
BRENDANS_HOUSE_RIVAL_BEDROOM`), and in CrystalDust setting it was a no-op, so
the line is dropped rather than carried over onto an unrelated flag.

*Not changed:* `EventScript_ResetAllBerries` still plants Hoenn's berry trees,
because CrystalDust's copy does too — it never revisited that script. The trees
have no maps to sit on, so this is inert. `EventScript_ResetAllMapFlagsFrlg`
also stays; `IS_FRLG` is false, so it is unreachable but still linked.

**`data/text/trainers.inc` was 1145 labels of dead Emerald trainer dialogue.**
Every Johto trainer's lines live in that trainer's own map `scripts.pory`.
Scanning the whole tree for references found exactly three live labels in the
file — the VS Seeker strings — and the assembler emits all of it into the ROM
regardless. Trimmed to those three. ROM: 29,134,308 B -> 29,067,236 B (86.83% ->
86.63%).

**Two Phase 5 worries turned out to be nothing.** `data/text/match_call.inc` and
`data/text/tv.inc` are already CrystalDust's: of 713 shared match-call labels
exactly one differs from CrystalDust, and only by a comment we added in D47;
`tv.inc`'s 298 diff lines are all label renames and `{POKE}` vs `POKé` macro
style. `data/text/pokemon_news.inc` is the same story. Ours does carry 32 extra
Emerald gym-leader rematch labels (Roxanne, Brawly, Flannery, Norman, Juan);
they are unreferenced and left alone for now.

`WarpToTruck` is renamed `WarpToPlayersBedroom` (D66 rename debt); it already
warped to `MAP_NEW_BARK_TOWN_PLAYERS_HOUSE_2F`.

A dead-text scan over the rest of `data/text/` found only small change left —
`move_tutors.inc`, `berries.inc`, `braille.inc` and `day_care_frlg.inc` are
fully unreferenced but total about 23 KB between them. Left in place; they are
cheap and some may come back with Phase 7 features.

Build: exit 0, smoke test clean.

## D78 — Phase 5 closes: the rename debt, and what the sweep actually found

**The text sweep found much less than expected.** Scanning every `.inc` and
`.pory` under `data/` for Hoenn place names turned up 129 hits, and almost all
of them are in files that are already CrystalDust's byte for byte
(`match_call.inc`, `tv.inc`, `pokemon_news.inc`) — CrystalDust simply never
rewrote the unreachable Emerald lines in them, so neither do we. The same holds
for `src/strings.c`: `gText_HOFDexRating`, `gText_BirchInTrouble`,
`gText_CheckMapOfHoenn` and `gText_ProfBirchMatchCallName` are identical in
CrystalDust's own `strings.c`. Where they are used at all it is from code paths
Johto never enters (`starter_choose.c`'s Birch bag, `STRINGID_DONTLEAVEBIRCH`'s
first-battle escape message).

Renames done: `DEX_MODE_HOENN` -> `DEX_MODE_JOHTO` (21 sites),
`sLilycoveDeptStore_DefaultFloorChoice` -> `sDeptStoreDefaultFloorChoice` (it
reads Goldenrod's dept store now), `WarpToTruck` -> `WarpToPlayersBedroom`
(D77).

### Constraint decision: the trainer card is still Emerald's, with Johto badges

Renaming `gHoennTrainerCard*` to `gJohtoTrainerCard*` collided — because
`src/graphics.c` **already** has a `gJohtoTrainerCard*` block, pointing at
CrystalDust's card art (`card_cd`, `bg_cd`, `front_cd`, `back_cd`,
`front_link_cd`, `0star_cd`). Nothing references it. `src/trainer_card.c` draws
Emerald's card and only the badge strip was swapped for Johto's in D59.

So **CrystalDust's trainer card design is in the tree but not on screen.** The
rename is reverted: `gHoennTrainerCard*` is an accurate name for Emerald's card
art, and the CrystalDust block keeps the Johto name it already had. Wiring it
up means porting CrystalDust's card-style selector — the same
`CARD_TYPE_CRYSTALDUST` / `VERSION_CRYSTAL_DUST` work already parked in D59 —
and is a Phase 7 item, not a rename.

Build: exit 0, ROM 29,067,236 B (86.63%), smoke test clean.

## D79 — `gettime` never told a script what day it was

Phase 6 opens on CrystalDust's TODO list. The first item chased there —
"The Rival is in Dragon's Den on the wrong days and before becoming Champion" —
turned out to sit on top of a much larger engine bug.

**`ScrCmd_gettime` was Emerald's.** It filled `VAR_0x8000` with hours,
`VAR_0x8001` with minutes and `VAR_0x8002` with *seconds*, and set no weekday at
all. CrystalDust's fills `0x8002` with the time of day and `0x8003` with the day
of the week, and every Johto script reads them that way: 19 map scripts branch
on `VAR_0x8003` as a weekday, and every `var(VAR_0x8002) == TIME_NIGHT` test in
the tree — the Pokémon Center nurse's greeting, Route 29, the National Park
gatehouses, the Goldenrod dept store, Mt. Moon Square — was comparing a
seconds-hand against a time-of-day constant. Nothing anywhere read seconds.
Now `gettime` sets `GetTimeOfDay()` and `GetDayOfWeek()`, matching CrystalDust.

*Note on the evening:* the user's Q1 decision put `OW_TIMES_OF_DAY` on
`GEN_LATEST`, which has four times of day; Crystal had three. CrystalDust's
scripts only ever test morning, day and night, so between the evening hours they
take the else branch. Left as is — it is a consequence of a decision already
made, not a merge loss.

**Dragon's Den.** Crystal's `DragonsDenB1FCheckRivalCallback` hides the Rival
unless you have beaten him at Mt. Moon *and* it is Tuesday or Thursday.
CrystalDust's port had the beaten-check inverted and nested, so the day test
only ran for players who had *not* beaten him — he appeared on the wrong days
and before the player was Champion, exactly as their TODO says. Rewritten as a
single "hide unless both hold".

Build: exit 0, ROM 29,067,236 B (86.63%), smoke test clean.

## D80 — the player was still Brendan and May

Found while porting CrystalDust's per-NPC text colours: `PLAYER_AVATAR_GFX_MALE_
NORMAL` resolved to `OBJ_EVENT_GFX_BRENDAN_NORMAL`, and
`gObjectEventGraphicsInfo_BrendanNormal` drew `graphics/object_events/pics/
people/brendan/walking.png`. Gold and Kris existed only as NPC graphics — the
one place they appeared was the Copycat's house in Saffron. So after choosing
Gold or Kris in the new intro (D76), the player walked out of New Bark Town as
Brendan or May.

Every one of CrystalDust's player sheets was already sitting in
`graphics/object_events/pics/people/gold/` and `kris/` — walking, running,
mach bike, acro bike, surfing, underwater, field move, fishing, watering,
decorating. Only the wiring was missing.

The fix follows what CrystalDust itself does: keep expansion's `Brendan*` /
`May*` graphics-info and pic-table names — every avatar table, map JSON and
script in the tree points at `OBJ_EVENT_GFX_BRENDAN_*` — and repoint the art and
the palette tags (`OBJ_EVENT_PAL_TAG_GOLD` / `_KRIS`) underneath them. Nine
infos each for Gold and Kris.

One sheet needed more than a repoint: **field move**. Gold's and Kris's are five
16x32 frames where Brendan's and May's are 32x32, so those two infos get new
`gObjectEventPic_{Gold,Kris}FieldMove` symbols, new pic tables, and a 16x32
OAM/subsprite shape — matching CrystalDust's `gObjectEventGraphicsInfo_
GoldFieldMove`.

### Constraint decisions

- **`Rival*` and `Link*` infos share the same pic tables**, so they now draw
  Gold's and Kris's art too. For the rival infos that is dead content (the Hoenn
  rival's maps are gone; Johto's rival is Silver, with his own sprite). For
  `LinkBrendan` / `LinkMay` in the Union Room it is arguably more correct.
- **Underwater keeps `OBJ_EVENT_PAL_TAG_PLAYER_UNDERWATER`**, as both games do.
- **Acro bike:** CrystalDust has no acro bike art at all — Johto has no acro
  bike — but `gold/acro_bike.png` and `kris/acro_bike.png` are in the tree with
  the same dimensions as Brendan's, so they are wired up rather than left on
  Brendan's.
- **This cannot be verified headlessly.** `mgba-perf` has no input injection, so
  the ROM boots clean but nobody has seen the sprite move. It wants the same
  human play-test as D76, and in particular a check that Gold's bike, surf and
  fishing sheets really do share Gold's palette — CrystalDust assumes they do.

Build: exit 0, ROM 29,069,860 B (86.63%), smoke test clean.

## D81 — dialogue takes its colour from the speaker again

`ScrCmd_textcolor` was a stub: it read the byte off the script and dropped it,
under a comment saying the command does nothing in Emerald. In CrystalDust it
does a great deal — Johto's scripts call `textcolor` 104 times, and every NPC
who is never named in a script still gets a colour from a lookup on their
overworld sprite. Men and boys speak in blue, women and girls in red, and
anything that is not a person — signs, machines, Pokémon, the PC — in the
standard dark grey. None of that survived the merge.

Ported from CrystalDust:

- `gSpecialVar_TextColor` / `gSpecialVar_TextColorBackup` (EWRAM, unsaved).
- The real `ScrCmd_textcolor`, including `MSG_COLOR_PREV` to pop back to the
  previous colour.
- `sTextColorByGraphicsId` — all 271 of CrystalDust's rows — plus
  `ContextNpcGetTextColor`, in `src/field_specials.c`.
- The colour switch in `AddTextPrinterForMessage`: blue is `TEXT_COLOR_BLUE`,
  red `TEXT_COLOR_RED`, everything else `TEXT_COLOR_DARK_GRAY`, all on the
  existing white/light-grey background and shadow.
- The `gSpecialVar_TextColor = MSG_COLOR_PREV` reset at the top of
  `ProcessPlayerFieldInput`, so a colour a script sets lasts exactly one
  conversation.

### Constraint decisions

- **All 271 rows are carried over, none dropped.** 192 of CrystalDust's names
  exist verbatim here; 67 are its `Z`-prefixed duplicates of Emerald objects and
  map onto the unprefixed name; 12 are remapped by hand —
  `GOLD_BIKE`/`SURFING`/`FIELD_MOVE`/`FISHING` onto `BRENDAN_*` and the `KRIS_*`
  four onto `MAY_*` (which is where the Gold and Kris art now lives, D80), and
  `EM_BRENDAN`/`EM_MAY`/`RS_BRENDAN`/`RS_MAY` onto `LINK_BRENDAN`/`LINK_MAY`/
  `LINK_RS_BRENDAN`/`LINK_RS_MAY`. No two rows collided.
- **`MSG_COLOR_BLACK` prints as dark grey**, because CrystalDust's own switch
  has no case for it either — it is a name for "not blue and not red", and the
  two scripts that use it get the standard colour in both games.
- **A sprite with no row gets blue**, since `MSG_COLOR_BLUE` is 0 and the table
  is zero-filled. That is CrystalDust's behaviour, not an accident, so it is
  kept rather than "fixed" to grey.
- **CrystalDust's `OBJ_EVENT_GFX_ZBARD` special case is dropped as redundant** —
  there it was an out-of-range id needing a hand-written blue; here `BARD` is a
  real enum member with its own row.
- **`AddTextPrinterForMessage_IgnoreTextColor` is not ported.** Nothing in this
  tree calls it; the two callers CrystalDust has are in code we do not have.
- **Not verifiable headlessly.** The ROM boots clean and the test suite is
  unchanged (32 failures, all the pre-existing `malloc.c` battle-test crashes),
  but seeing a blue line of dialogue needs a human to talk to an NPC.

Build: exit 0, ROM 29,070,500 B (86.64%), +640 B for the table.

## D82 — The Bug Catching Contest is wired back into the engine

`src/bug_catching_contest.c` came through the merge whole (170 references, the
whole contest state machine, the NPC roster, the results screen, the swap
prompt), but a per-file reference audit against CrystalDust found **fifteen
files that reference it there and zero times here** — the core was ported and
every single integration hook was lost. The contest was dead code: nothing
started a contest battle, nothing drew the Ball menu, nothing ran the clock,
nothing handled a white-out in the park, nothing placed the contestants.

Restored: `src/start_menu.c` (the Retire menu), `src/battle_setup.c`
(`DoBugCatchingContestBattle`), `src/field_control_avatar.c` (the timer check,
ahead of anything that could start a script), `src/overworld.c` (contestant
placement in National Park, the gatehouse music, contest teardown on
Fly/Teleport/Dig), `src/field_poison.c` + `data/scripts/field_poison.inc` (the
contest white-out path), `src/battle_main.c`, `src/battle_util.c`,
`src/battle_script_commands.c`, `src/battle_controller_player.c`,
`src/battle_message.c`, `src/battle_bg.c`, `src/pokedex.c`, and the constants
and macro headers behind them.

Constraint decisions, none of them silent:

- **`BATTLE_TYPE_BUG_CATCHING_CONTEST` is bit 14, not CrystalDust's bit 12.**
  Bit 12 is `BATTLE_TYPE_RAID` in expansion. Bit 14 was expansion's unused
  `BATTLE_TYPE_14`, so nothing is displaced. It is also added to
  `BATTLE_TYPE_RECORDED_INVALID`, matching CrystalDust's illegal-types list.
- **`B_ACTION_PARK_BALL` reuses expansion's unused `B_ACTION_UNK_15`**, so no
  existing action id renumbers.
- **Two unused battle-script opcodes are repurposed**: `B_SCR_OP_UNUSED_41` and
  `_42` become `setcaughtbugcontestmon` and `swapbugcontestmon`. Opcode numbers
  are unchanged, so no script data shifts.
- **The contest start menu offers POKÉNAV, not CrystalDust's POKéGEAR.** This
  tree has no Pokégear menu action; the surrounding start menu is expansion's.
- **`B_BUG_CONTEST_MON` (CrystalDust's FD 35 buffer) is deliberately not
  ported.** It is unused in CrystalDust itself and its control code collides
  with expansion's `B_ATK_TRAINER_NAME` (see D57 in `charmap.txt`).
- **`GAME_STAT_POKEMON_CAPTURES` is still incremented for a contest catch**,
  unlike CrystalDust, because the counter lives in shared capture code that
  expansion has restructured; a contest catch is a real catch here. Cosmetic,
  affects only the trainer-card statistic.
- **The contest battle window type is new (`B_WIN_TYPE_BUG_CATCHING_CONTEST`,
  3)**, a copy of the standard templates with the action menu one tile wider
  and one tile left (for "Ball×N") and its base block moved from 0x0190 to
  0x0180 so the wider window does not run into the action prompt.
- **`B_TRANSITION_WAVE` needs no change.** CrystalDust's TODO flags a bug there;
  that is expansion's own untouched transition code and does not apply.
- **None of it is verifiable headlessly.** The ROM boots clean and the test
  suite is unchanged (32/14/521/9/4615/5191, identical to the baseline), but the
  contest is entirely menu- and script-driven, and mgba-perf cannot inject
  input. A human play-test of National Park is required.

Build: exit 0, ROM 29,072,164 B (86.64%).

## D83 — Johto's fruit trees are drawn again

The same audit that found D82 flagged `fruit_tree.h`: CrystalDust includes it
from three files, this tree from one (`day_night.c`, which does not even call
into it). `src/fruit_tree.c` was ported whole and 23 maps carry
`BG_EVENT_FRUIT_TREE` bg events, but nothing ever called
`SetFruitTreeMetatiles`, so every Johto berry/Apricorn tree drew as whatever
metatile the layout happened to hold and never changed when picked or regrown.

Restored CrystalDust's three call sites: `InitMap` and `InitMapFromSavedGame`
in `src/fieldmap.c` (draw without redrawing, the map is not up yet),
`FillConnection` (so a tree on a connected map is correct before the player
crosses the border), and `UpdatePerDay` in `src/clock.c` next to
`ClearDailyFlags` (regrowth, with a live redraw).

No constraint decisions were needed — every call site transferred unchanged.
`GetFruitTreeItem` and `SetFruitTreeMetatileTakenFromId` were already registered
in `data/specials.inc`, so the picking scripts were waiting on this. Needs a
human play-test: the tree art and the day-rollover regrowth cannot be checked
headlessly.

Build: exit 0, ROM 29,072,324 B (86.64%).

## D84 — Saves are stamped with a build number; the load-time rejection is not

`game_build.h` was the second file the D82/D83 audit flagged: CrystalDust
includes it from four files, this tree from one. `src/game_build.c` is present
and `struct SaveBlock1` still carries `gameBuild` and `saveBlockMagic`, but
nothing wrote them and nothing read them.

- **`SetBuildNumber()` is restored** in `NewGameInitData`, next to
  `ResetContestLinkResults` as in CrystalDust, so a new save is stamped.
- **CrystalDust's load-time rejection is deliberately NOT ported.** In
  CrystalDust, `save.c` maps a bad magic to `SAVE_STATUS_CORRUPT` and a build
  mismatch to `SAVE_STATUS_BUILD_MISMATCH`, and `main_menu.c` then shows a big
  error window pointing at CrystalDust's own save-updater URL. Three reasons to
  leave it out: this tree has no `SAVE_STATUS_BUILD_MISMATCH` and its main menu
  is expansion's, not CrystalDust's; the error text
  (`gText_BuildVersionMismatch`, already in `strings.c`) sends the player to
  `domoreaweso.me/cdupdate`, which is not this project's updater and has no save
  converter for it; and switching it on now would make every save created
  before this commit read as corrupt, since their magic is zero. This is a
  parked feature, not a dropped one — the check can be turned on once this
  project has its own version line and a reason to break save compatibility.

Build: exit 0, ROM 29,072,676 B (86.64%).

## D85 — The player's room is furnished on a new game

`SetDefaultRoomDecor` is the third hook the `new_game.c` audit turned up, and
unlike D84 it was missing outright rather than merely uncalled: the function did
not exist in this tree at all. Everything it touches survived —
`VAR_ROOM_BED`/`VAR_ROOM_TABLE`/`VAR_ROOM_POSTER`, the `RoomDecor` bitfield in
`SaveBlock1`, `constants/room_decor.h`, and
`data/maps/NewBarkTown_PlayersHouse_2F/scripts.pory`, which already reads the
vars to draw the room. With nothing setting them, a new game started with all
three at zero: no bed, no desk, no poster in the player's bedroom, on the very
first map of the game.

Re-created verbatim from CrystalDust and called from `NewGameInitData` in
CrystalDust's position, after `SetBuildNumber`. No constraint decisions; every
constant and field transferred unchanged.

Note this is the *starting* decor only. CrystalDust's decorating menu in the
bedroom PC is unimplemented upstream too (it is on their TODO), so the player
still cannot change it. That remains a genuine gap, now recorded.

Build: exit 0, ROM 29,072,740 B (86.64%).

## D86 — CrystalDust's diagonal staircases pointed at the wrong behaviours

CrystalDust defines four metatile behaviours for its diagonal staircase warps
at `0x2C`–`0x2F` (`MB_STAIRCASE_UP_EAST`, `UP_WEST`, `DOWN_EAST`, `DOWN_WEST`).
Expansion uses those four ids for something else entirely — `MB_FAST_WATER`,
`MB_CYCLING_ROAD_WATER`, `MB_UNUSED_2E`, `MB_UNUSED_2F` — and its own
directional stair warps live at `0xEB`–`0xEE`. The CrystalDust tilesets came
through the merge with their attribute data untouched, so **every diagonal
staircase in the game was being read as fast water or an unused behaviour**: 76
metatiles across 19 tilesets, including `playersroom`, `building`, `lighthouse`,
`radio_tower`, `department_store`, `underground`, `silphco`, `rockethideout`,
`inside_ship` and `pagoda_tower`. These are the ordinary indoor staircases of
Johto and Kanto, on maps the player uses constantly.

The good news is that **no code needed porting**. Expansion already implements
the whole feature natively and better — `MetatileBehavior_IsDirectionalStairWarp`,
`IsDirectionalStairWarpMetatileBehavior`, `DoStairWarp`, `Task_StairWarp`,
`Task_ExitStairs`, `gExitStairsMovementDisabled`, the `COLLISION_STAIR_WARP`
path in the player avatar, and the walk-in/walk-out slide animation are all
present and wired. CrystalDust's versions of those functions (in
`field_screen_effect.c`, `field_control_avatar.c`, `overworld.c`,
`field_player_avatar.c` and `main_menu.c`) were therefore deliberately not
ported — porting them would have duplicated working engine code, and the
standing rule to let CrystalDust win applies to *content*, not to an engine
feature expansion already does.

Constraint decisions:

- **The fix is a data remap, applied in place** to the 19 CrystalDust-origin
  tilesets: `0x2C→0xEB`, `0x2D→0xEC`, `0x2E→0xED`, `0x2F→0xEE`. East maps to
  Right and West to Left, one-for-one, with no reinterpretation.
- **FRLG-origin tilesets were deliberately left alone.** Ten `*_frlg` tilesets
  also use `0x2C`/`0x2D`, but those are expansion's own imports in expansion's
  numbering, where the values really do mean water. They were excluded by
  requiring the tileset to exist in CrystalDust's tree.
- **This is safe because CrystalDust has no fast-water behaviour at all** — its
  `metatile_behaviors.h` defines neither `MB_FAST_WATER` nor
  `MB_CYCLING_ROAD_WATER`, so in a CrystalDust tileset `0x2C`–`0x2F` can only
  ever have meant a staircase.
- **`SLOW_MOVEMENT_ON_STAIRS` is left `FALSE`**, expansion's default.
- Not verifiable headlessly; walking a staircase needs a human.

Build: exit 0, ROM 29,072,740 B (unchanged — data-only).

## D87 — Every other metatile-behaviour collision, including signposts

D86 was not an isolated case, so the whole of CrystalDust's behaviour numbering
was reconciled against expansion's, value by value, using CrystalDust's own
tileset attribute data as ground truth. Most differences are the same slot under
a different name (`MB_UNUSED_CAVE`/`MB_CAVE`, `MB_SEMI_DEEP_WATER`/
`MB_INTERIOR_DEEP_WATER`, the Pacifidlog logs, the Route 120 bridges) and needed
nothing. Seven were genuine collisions, all now remapped in the tileset data:

| CrystalDust | meaning | our value | metatiles | tilesets |
|---|---|---|---|---|
| `0x7E` | `MB_SIGNPOST` | `0x1D` | 83 | 40 |
| `0x81` | `MB_POKEMON_CENTER_SIGN` | `0x1E` | 15 | 8 |
| `0x82` | `MB_POKEMART_SIGN` | `0x1F` | 16 | 9 |
| `0xAE` | `MB_CYCLING_ROAD_PULL_DOWN` | `0xC8` | 38 | 1 |
| `0xAF` | `MB_CYCLING_ROAD_PULL_DOWN_GRASS` | `0xC9` | 1 | 1 |
| `0xEB` | `MB_DECOR_POSTER` | `0xDC` | 13 | 1 |
| `0xEC` | `MB_DECOR_CONSOLE` | `0xDD` | 5 | 1 |

The signpost row is the significant one: **83 signposts across 40 tilesets**, in
essentially every town and gym in the game, were reading as
`MB_UNUSED_BRIDGE`. Signs did not behave as signs.

Constraint decisions:

- **The remap is keyed off CrystalDust's original files, offset by offset**, not
  off a blind value sweep of ours. Each metatile is only rewritten if our byte
  still holds CrystalDust's original value. That matters because D86 had already
  moved staircases into `0xEB`/`0xEC`'s neighbourhood; keying off the originals
  makes the two passes commute. 15 offsets in `playersroom` were skipped
  because an earlier phase had already corrected them by hand.
- **`0xEB`/`0xEC` had to move anyway**, independently of D86: CrystalDust's
  poster and games-console decor behaviours sit exactly where expansion puts two
  of its stair warps. Both meanings now have their own slot.
- **Two mismatches are knowingly left as they are, because nothing places
  them.** `0x23` (CrystalDust: unused; ours: `MB_STRENGTH_BUTTON`) appears on 35
  `halloffame` metatiles, and `0x54`–`0x58` (CrystalDust: unused; ours: the four
  spin tiles and `MB_STOP_SPINNING`) on 14 metatiles across five Kanto indoor
  tilesets. A spin tile underfoot would trap the player, so this was checked
  rather than assumed: every layout's blockdata was scanned against its two
  tilesets' attributes and **not one of these metatiles is placed on any map in
  the game**. They are dead tileset entries. Recorded here so that anyone adding
  a map with them knows to fix the attribute first.
- **FRLG-origin tilesets were again excluded**, verified by re-scanning
  afterwards: the only remaining `0xAE` uses are four `*_frlg` tilesets, where
  the value legitimately means `MB_NEATLY_LINED_UP_TOOLS`.
- Not verifiable headlessly; reading a sign needs a human.

Build: exit 0, ROM 29,072,740 B (unchanged — data-only).

## D88 — Phase 6 close-out: what CrystalDust's TODO means for this port

Phase 6 was scoped as "work through CrystalDust's `TODO.md`". Working through it
produced a scoping rule worth stating plainly, because it decides a lot of
items at once:

> **A feature CrystalDust never implemented is not CrystalDust content.** This
> project ports CrystalDust's content onto expansion. Where CrystalDust's TODO
> says something is "entirely unimplemented" or "needs art/music", there is
> nothing to port; implementing it would be new authoring for a game neither
> upstream has. Those items are *out of scope*, not dropped.

That rule is what the real Phase 6 work turned out to be instead: not
implementing CrystalDust's wish-list, but finding the places where CrystalDust
*did* have the content and our merge lost or broke it. That hunt produced
D82–D87 — the Bug Catching Contest, the fruit trees, the build stamp, the room
decor, the staircases and the signposts — every one of them a working
CrystalDust feature that was silently dead here.

### Out of scope: needs authoring CrystalDust never did

Missing music and placeholder tunes; missing OW sprites (DJ Mary, Janine,
Bayleef, Morty's and Clair's side frames, legendary beast walk frames, Peeko and
Mr. Briney); the Tin Tower roof tileset; Magnet Train tiles and cutscene; the
credits sequence; Pokédex diploma graphics; Ilex Forest tree-wiggle art; the
Unown Dex; the IR Mystery Gift replacement; the bedroom decorating menu; the
Trainer House basement; Mom's obscure item calls; the Game Director and Artist
scripts; Vermilion Gym's puzzle. All of these are unimplemented *upstream*.

### Out of scope by earlier decision

Sevii Islands (D35) and everything downstream of them, including the
Pokémon Communication Center unlock and Pokémon Contests. Battle Frontier
(Q3, parked). Hoenn (D37).

### In scope, still open, and deliberately parked

- **Fishing encounter percentages are Emerald's, not Crystal's.** Confirmed by
  comparison with `sources/pokecrystal/data/wild/fish.asm`: Crystal's model is
  per-fishing-group with three or four cumulative slots per rod, while ours is
  Emerald's fixed 2/3/5-slot model, and our constants are byte-identical to
  CrystalDust's — CrystalDust never fixed this either. Fixing it properly means
  changing the number of fishing slots per rod, which changes the encounter JSON
  schema and every water map's data. Real work, real risk, and CrystalDust
  itself deferred it. Parked, not dropped.
- **Cut-tree and smashable-rock pop-in at map connections.** CrystalDust has a
  workaround (`ShouldTreeOrRockObjectBeCreated`, `IsConnectionTreeOrRockOnScreen`,
  `IsTreeOrRockOffScreenPostWalkTransition` in `event_object_movement.c`) which
  this tree lacks. Not ported: it hooks object spawning, which expansion has
  restructured heavily around follower NPCs, and the payoff is cosmetic. Parked
  with the call sites recorded here.
- **Phone-call timing follows Emerald, not Crystal** (calls can be re-rolled by
  soft-resetting), contact ordering is fixed rather than acquisition-ordered, and
  the rematch-roster-repeat question is unanswered. CrystalDust's own TODO lists
  all three as open.
- The Azalea map-name popup, the radio channel-change text overflow, and the
  Dragon's Den rival's day/Champion conditions are CrystalDust script bugs that
  came across with the scripts. Inherited as-is.

### Verified as *not* problems

- `B_TRANSITION_WAVE` (CrystalDust's bug report) is expansion's own untouched
  transition code.
- CrystalDust's `TextboxUseSignBorder` has no counterpart here because expansion
  does the same job with `gMsgIsSignPost` — and as of D87 the signpost
  behaviours finally reach it.
- CrystalDust's layered region map is present, as `src/pokegear_map.c` with a
  `CDMap_` prefix.
- All 515 map script includes and all 401 `scripts.pory` files are present,
  matching CrystalDust exactly.
- Every CrystalDust special absent from `data/specials.inc` is either Hoenn
  content (the Mauville old men, the storyteller, the decoration trader), Battle
  Frontier, or an expansion rename. Three scrollable-multichoice specials remain
  parked from D48.12/D70.

Test suite at the gate: 32 FAILED / 14 KNOWN_FAILING / 521 TO_DO /
9 EXPECT_FAILING / 4615 PASSED / 5191 TOTAL — identical to the baseline.
ROM 29,072,740 B (86.64%).

## Phase 7 — expansion features

### D89: the 32 "FAILED" battle tests were one runaway `StringCopy`

The 32 failures carried over from the Phase 0 baseline were not 32 defects.
They were 32 *crashes* on a single test runner, all cascading from one event,
plus the framework's own `EXPECTED_FAIL` self-tests (which report as "FAIL" by
design). Upstream pokeemerald-expansion 1.17.0 builds clean here — 0 failures,
5769 tests — so the defect was ours.

The single culprit is `test/battle/front_anim.c` "Front anims work", the only
test that ends a *wild* battle against a *shiny* opponent. That combination is
the sole path into `TryPutBreakingNewsOnAir()`, whose second statement is
`StringCopy(show->breakingNews.playerName, gSaveBlock2Ptr->playerName)`.

In test builds the save blocks are never initialised, so `playerName` is all
zero bytes and contains no `EOS`. `StringCopy` therefore never terminates at the
name: it walks forward through EWRAM until it happens to find an `0xFF`, writing
as it goes. Destination trails source by about 0xBF0 bytes, so by the time the
source reaches `gHeap` the destination has already zeroed the heap's first
blocks — head magic, sizes, `next`/`prev`, everything. The next `Free()`
(from `FreeBattleResources` during battle teardown) trips
`AGB_ASSERT(block->magic == MALLOC_SYSTEM_ID)`, the assertion kills the ROM, and
every subsequent test on that runner is reported as a crash.

**Why upstream survives and we don't:** the runaway is data-dependent. It stops
at the first `0xFF` byte after `playerName`. CrystalDust's additions rearranged
`SaveBlock2`, leaving a long zero run where upstream has an early `0xFF`, so the
copy overshoots into the heap here and does not there. The bug is latent
upstream; our save-block layout is what exposes it.

**Fix:** `test/test_runner.c` terminates `gSaveBlock2Ptr->playerName` next to the
per-test `InitHeap`. Test-build only; the real game always has a terminated
name, so no game code changed.

With the fix in, the whole suite completes on every runner: 0 FAILED /
16 KNOWN_FAILING / 586 TO_DO / 9 EXPECT_FAILING / 5158 PASSED / 5769 TOTAL —
the same total upstream reports, i.e. no test is being lost any more.

Diagnosis notes worth keeping: `Test_MgbaPrintf` has no `%x` handler, and a `%x`
silently corrupts the rest of the format string — use `%d`/`%p`/`%s`. And an
`AGB_ASSERT` failure presents as an "Illegal opcode" plus a ROM restart, which
reads exactly like a jump to NULL; the assertion line in the log is the real
first event, not the reset.

### D90: CrystalDust's own trainer card, and why `CARD_TYPE_EMERALD` is now unreachable

CrystalDust ships a fourth card type with its own sheet, tilemaps, star palettes and
female background (the `*_cd` assets). Every one of them was in our tree and referenced
by nothing; D59 had settled for dressing the Hoenn card in Johto badges and CD player
sprites. The card is now real: `CARD_TYPE_CRYSTALDUST` in `include/constants/trainer_card.h`,
the `gJohtoTrainerCard*` assets moved from the old LZ pipeline to smol (the LZ blobs
could not be read by `DecompressDataWithHeaderWram` at all, so they would have decoded
to garbage the moment anything pointed at them), and three-way branches in
`LoadCardGfx()`, `SetCardBgsAndPals()` and `DrawStarsAndBadgesOnCard()`.

Constraint decisions, none of them silent:

- **`VERSION_CRYSTAL_DUST 7` collides with expansion's `VERSION_HEART_GOLD 7`.** We do
  not add CrystalDust's constant. `GAME_VERSION` stays `VERSION_EMERALD`, and the card
  type is chosen in `GetSetCardType()` instead: a save whose `version` is `VERSION_EMERALD`
  now yields `CARD_TYPE_CRYSTALDUST`. The consequence is that **`CARD_TYPE_EMERALD` is
  unreachable for our own saves** — it survives only for cards received over link from a
  genuine Emerald. That is deliberate: our saves *are* CrystalDust saves wearing Emerald's
  version byte.
- **The card carries Emerald's data set, not CrystalDust's.** `VersionToCardType()` is
  untouched, so `SetPlayerCardData()` and the link-copy switch still fill and read the
  Emerald fields (trades, link contests, Pokéblocks, frontier BP) at the Emerald offsets.
  CrystalDust's back page is `{CONTESTS, BATTLE_POINTS, NONE}`; ours shows the Emerald
  stat list. Approximation, not a drop — changing it means moving fields inside the saved
  `struct TrainerCard`, which breaks link compatibility with Emerald for no gain.
- **No stickers and no party-icon strip on the CD card.** Those live in FRLG-only save
  fields (`shouldDrawStickers`, `stickers[]`, `monIconTint`) that an Emerald-shaped card
  never populates. Same reason as above.
- **Text placement follows the Kanto card**, via the new `IsKantoStyleCard()` helper and
  `isHoenn = FALSE`. This is not a guess: every offset row CrystalDust defines for
  `CARD_TYPE_CRYSTALDUST` is byte-identical to the Kanto row. The trainer-pic offset
  `{13, 4}` is Kanto's and is the one item here that CrystalDust may place differently;
  it is on the play-test list.
- **Tile layout is ours, not CrystalDust's.** CrystalDust's sheet is 224 tiles where
  expansion loads 192, so it overruns bg3's base tile of 192 where the badges live.
  CrystalDust solved this with wrapping arithmetic across its own bg layout; we instead
  load `0x1C00` for the CD sheet and push the CD badges to `CD_BADGE_TILE_OFFSET` (160,
  i.e. VRAM tile 352), which is clear of the mon icons (224–319) and stickers (320–351).

Not verifiable by build: whether the card actually *looks* right. It joins the human
play-test list with D76/D80–D87.

### D91: `{POKEMON}` printed "ARCHIE" — the last live charmap alias

Reported from a play-test. CrystalDust's text uses three placeholders of its own,
`{POKEMON}`, `{POKE}` and `{POKEDEX}`, which expand to POKéMON / POKé / POKéDEX. The
Phase 1 charmap merge (D2) gave them the byte values `FD 0A`, `FD 0B`, `FD 0C` — which
in expansion's charmap already mean ARCHIE, MAXIE and KYOGRE — and recorded them as
"deliberate aliases". They were not harmless aliases: the placeholder byte is what
`StringExpandPlaceholders()` switches on, so every one of CrystalDust's 2,684 `{POKEMON}`
strings printed the word ARCHIE, 157 `{POKE}` printed MAXIE, and 62 `{POKEDEX}` printed
KYOGRE.

The strings themselves were never missing: `gText_ExpandedPlaceholder_Pokemon/Poke/Pokedex`
have been in `src/strings.c` since D9. Only the wiring was absent.

Fix: `PLACEHOLDER_ID_POKEMON/POKE/POKEDEX` = 0xF/0x10/0x11, the three bytes immediately
after `PLACEHOLDER_ID_REGION`, with matching expander functions and charmap entries
`FD 0F`/`FD 10`/`FD 11`. Contiguity matters — `GetExpandedPlaceholder()` indexes a
designated-initialiser table, so a gap would be a NULL entry to jump through. Those byte
values are also used by the battle-string namespace (`B_ATK_NAME_WITH_PREFIX` and
friends), but the two are expanded by different functions and no battle string in the tree
uses `{POKEMON}`, `{POKE}` or `{POKEDEX}`, so the namespaces do not meet.

Verification: the built ROM holds 3,050 `FD 0F` sequences where it previously held none.

This is the fourth defect of the "Phase 1 merge kept one side" family to surface after a
gate closed, and the first found by a human playing rather than by a build or a test. It
is worth assuming there are more of its kind in D2's alias list.

### D92: the ROM is `pokecrystal.gba`

`BUILD_NAME` is `crystal` and the cartridge title is `POKEMON CRYS`. `GAME_VERSION` stays
`EMERALD` — it is load-bearing throughout the source, and D90 already documents why the
game identifies as Emerald internally — and `GAME_CODE` stays `BPEE` so save files and
emulator per-game detection keep working. The object directory moves to `build/crystal`,
so the first build after this change is a full one.

### D93: the new game ran Emerald's moving-truck sequence

Found by the headless harness (below), reproducing the play-test report of wrong
tiles in the player's house.

`CB2_NewGame()` set `gFieldCallback = ExecuteTruckSequence` for everything that
isn't FRLG. In Johto there is no truck: the sequence overwrote three metatiles
of the bedroom at (4,1)-(4,3) with `METATILE_InsideOfTruck_*`, cleared the faded
palette buffer, and called `LockPlayerFieldControls()` -- and `Task_HandleTruckSequence`
only unlocks along the truck's own path, so field controls stayed locked
forever. A new game began with the player unable to move or open the menu.

CrystalDust's own `CB2_NewGame` uses `FieldCB_WarpExitFadeFromBlack`. Ours now
does the same, unconditionally.

Another "Phase 1 merge kept one side": the Emerald branch of this function
survived and CrystalDust's did not.

### D94: primary tilesets were split at 512 instead of 640

The reported "high number of tiles are the wrong tile", and the reason the
player could surf on dry land and meet level-20 Tentacruel.

CrystalDust uses the FRLG-style split -- 640 tiles, 640 metatiles and 7 palettes
in the primary tileset -- and all 419 of its layouts, tilesets and `map.bin`
files are built for it. The merge kept Emerald's `NUM_TILES_IN_PRIMARY 512`,
`NUM_METATILES_IN_PRIMARY 512`, `NUM_PALS_IN_PRIMARY 6`. Every metatile id at or
above 512 was therefore treated as a secondary-tileset id 128 blocks too early,
so maps drew the wrong blocks *and* read the wrong entry from
`metatile_attributes.bin` -- which is where the bogus surf behaviour came from.

Fixed by taking CrystalDust's constants in `include/fieldmap.h`. The expansion's
per-layout `isFrlg` flag was the wrong lever: it also switches metatile
attributes to FRLG's 4-byte format, and CrystalDust's attribute files are
Emerald's 2-byte format. Every layout in the repo is CrystalDust's, so a global
change is both correct and safe.

### D95: a headless play-test harness

`tools/playtest/` builds a small C program against the local libmgba 0.10.5. It
boots `pokecrystal.gba` with no window, runs a script of waits, button presses,
screenshots and memory reads, and prints every read. `tools/playtest/playtest.py`
resolves symbol names against `pokecrystal.elf` (which carries the file-local
statics the linker map omits), expands `player`/`where` into the right reads,
names metatile behaviours from `metatile_behaviors.h`, and converts the frames
to PNG.

    tools/playtest/build.sh
    tools/playtest/playtest.py tools/playtest/scripts/newbark.txt --out /tmp/shots

`scripts/newgame.txt` plays a new game from boot to the bedroom; `newbark.txt`
continues to the house's ground floor. Both D93 and D94 were found with it
within an hour, having survived every gate of the test suite -- the suite runs
code, this runs the game.

### D96: a debug-only warp hook, so the harness can reach every map

Walking to each of the 565 maps is not practical, so `CB1_Overworld` gained a
hook, compiled only when `DEBUG_OVERWORLD_MENU` is on (i.e. never in a release
build). Writing a map group, map number and a pair of coordinates into
`gPlaytestWarp` warps there on the next frame.

The hook only fires from an idle field: warping mid-fade corrupts the heap, and
cutting a script short mid-message leaves its text window allocated for the next
map's script to free a second time. `tools/playtest/sweep.py` drives it, picking
a walkable non-warp tile near the middle of each map from `map.bin`'s collision
bits, and restoring a savestate of the opening before each map so that one
cutscene cannot derail the rest of the run.

### D97: half the overworld sprite set was behind `#if IS_FRLG`

The first full sweep found 45 maps that reset the console a couple of seconds
after the player arrived -- Cerulean City, every Pokémon Center, most Marts,
several gyms -- and the crash was always `TrySpawnObjectEventTemplate` calling
`LoadSheetGraphicsInfo` through a NULL graphics info.

Expansion gates its FRLG-derived object event graphics -- the Kanto NPCs, the
gym leaders, the overworld Pokémon -- on `IS_FRLG`, in four files: the pics, the
pic tables, the graphics infos and the info pointer table. CrystalDust uses
those sprites everywhere, in Johto as much as in Kanto: 51 distinct graphics ids
used by this repo's maps resolved to NULL, `OBJ_EVENT_GFX_COOLTRAINER_M` alone
on 64 object events. Whenever the player walked close enough for one to spawn,
the game jumped into nothing.

The gates are gone, and `gObjectEventGraphicsInfo_Janine` and
`..._SlowpokeTailless` gained the forward declarations they had never needed
while the block was dead. ROM went from 86.64% to 87.38% of the 32MB cart --
about 740 KB of sprites, the cost of the Kanto half of the game having NPCs.

The Z-prefixed doll and trophy ids (443-469) are still unmapped; they are secret
base decorations, which this port does not reach.

### D98: a clock set behind the RTC hung the game for good

Nineteen more maps never finished loading: Route 29, 32, 36, 37 and 40,
Blackthorn City, Lake of Rage, the Goldenrod underground, the Game Corner,
Dept. Store 5F, several gatehouses. The program counter sat in
`DateTime_AddDays`, called from `GetDayOfWeek` -- and those are exactly the maps
with a day-of-week event.

`struct Time` is signed and `gLocalTime` goes negative whenever the in-game
clock is set behind the RTC, which Crystal's own clock prompt allows. The
`DateTime_Add*` helpers take `u32`, so -6 days became four billion iterations of
a loop with a division in it: not a crash, a permanent hang, inside the
blocking `DoMapLoadLoop`, so the screen stayed black forever.

`ConvertTimeToDateTime` now clamps a backwards clock to the epoch. A wrong date
is a cosmetic problem; a hang is not.

### Sweep results

With D97 and D98 fixed, the remaining flags are all understood: unlit caves
(correct -- they want Flash), maps smaller than the screen (correct -- the black
is past the map edge), the 24 secret bases and the Battle Frontier (parked
content, Q3), and `UnionRoom`/`LilycoveCity_ContestLobby` (link and Hoenn
leftovers). `tools/playtest/sweep.py` now discounts both expected-black cases so
that a flag means something.

Still open from the static audit (`tools/playtest/audit_tilesets.py`): 30
layouts whose `map.bin` references a metatile past the end of its tilesets, in
the Kanto cities and routes and the department stores. Those maps render, so the
overflow is reading a neighbouring tileset's blocks rather than crashing, but it
is a real content defect and the likely cause is the Kanto secondary tilesets
being included from `data/tilesets/secondary/*_frlg/` while CrystalDust's own,
larger, versions sit unused beside them.

## D99 — the Kanto tilesets came from FRLG, not from CrystalDust

`src/data/tilesets/metatiles.h` and `graphics.h` pointed 25 Kanto tileset
symbols at `data/tilesets/secondary/*_frlg/`, expansion's FreeRed/Green
imports, while CrystalDust's own versions of the same tilesets sat unused
beside them. CrystalDust's `data/tilesets/metatiles.inc` references no
`*_frlg` directory at all — this is another instance of the Phase 1 merge
keeping expansion's side of a shared file.

Two things were wrong as a result:

* **The tilesets were too small.** CrystalDust's maps are drawn against its
  own, larger metatile sets (Cerulean City 216 metatiles vs FRLG's 134,
  Fuchsia City 270 vs 180), so 30 layouts referenced metatile ids past the
  end of the tileset — the Kanto cities and routes, the Goldenrod and
  Celadon department stores, Silph Co 1F, Diglett's Cave, Cianwood
  Pharmacy, the Silver Cave rooms and Goldenrod Flat2 3F.
* **The attributes were in the wrong format.** The `*_frlg` directories
  carry FRLG's 4-byte-per-metatile attributes, and the engine reads
  `metatileAttributes` as `u16`, so every Kanto metatile took the behaviour
  of the metatile two slots before it — wrong collision, wrong encounter
  type, wrong door.

Fix: repoint all 25 symbols (tiles, palettes, metatiles and attributes
together) to CrystalDust's directories. The palette overrides already
pointed there, so the two halves now agree. The static audit's Kanto
overflows drop from 30 to 0, and a 158-map sweep of Kanto flags only the
five maps already understood (camera at a map edge). ROM 87.38% -> 87.39%.

The 39 remaining `*_frlg` tilesets are referenced by no layout and are
candidates for deletion (see the orphan list); CrystalDust's own
`powerplant` directory is itself in the 4-byte attribute format, which is
an upstream quirk, not a regression from this change — noted for the human
play-test list.

### Deferred: deleting the 39 dead FRLG tilesets

After D99 no layout references any `*_frlg` tileset, and their tiles,
palettes, metatiles and attributes measure 307,868 bytes of ROM (0.9% of
the cartridge), plus their animation data. They are not deleted yet
because `src/field_door.c` still carries FRLG door animation entries that
point at them, `src/field_specials.c` references
`gTileset_GenericBuilding1` inside a dead `IS_FRLG` branch, and
`src/tileset_anims.c` has an `InitTilesetAnim_` callback per tileset. That
is a wider edit than the ROM headroom currently justifies (87.4% of 32 MB).
Revisit if the ROM approaches the limit; nothing else depends on it.

## D100 — a progression harness, and the three ways its first answers were wrong

The map sweep (D93) proves a map draws. It says nothing about whether the map's
cutscene still fires, and a cutscene is exactly what a three-way merge breaks:
an object event that moved, a flag that was renumbered, a script command whose
macro and implementation disagree.

`tools/playtest/progression.py` reads every `map_script_2 VAR, value, Label`
out of the on-frame tables in `data/maps/*/scripts.inc` — 98 of them — and for
each one restores a shared after-intro save, sets the trigger variable, warps
the player onto the map, answers dialogue until the script ends, then reads
back the flags and variables that the script's own source says it sets. The
expectation is derived from the script rather than written by hand, so it
cannot rot: `effects()` follows `call`, `goto` and the `goto_if_*` branches.

Three things it got wrong at first, all of them worth recording because each
made the harness agree with a broken game:

- **`CONTEXT_WAITING` is not "finished".** The first version watched
  `sGlobalScriptContextStatus != CONTEXT_RUNNING`, but a script sits in
  `CONTEXT_WAITING` for the whole of every `msgbox` and every `waitmovement`.
  Every beat therefore "passed" at its first line of dialogue. Only
  `CONTEXT_SHUTDOWN` means ended.
- **Settling before watching hides short scripts.** Waiting 150 frames for the
  warp to land let a cutscene run and finish unobserved, which reads exactly
  like a cutscene that never fired. The harness now watches for the map from
  the frame the warp is requested.
- **A script with no waits in it is never seen running at all.** It executes to
  completion inside one `ScriptContext_RunScript` call, so "never seen running"
  is only evidence of a problem when nothing changed either.

What it cannot judge is listed in `tools/playtest/beats.py` with a reason each,
so nothing is dropped silently: 61 Battle Frontier / Battle Tent / Trainer Hill
/ Hoenn contest beats (parked or cut), the Dragon's Den quiz (mashing A answers
question five wrong and the elder asks again for ever, which is the script
working), the bug-contest award ceremony (needs contest results that only a
real contest produces), and two scenes whose own transition script switches the
trigger off for a save that has not earned them (Dragon's Den Clair, Mt Moon
rival). Those stay on the human play-test list.

Temporary variables cannot be read back after a script that warps: the next map
load clears them, so a beat whose only expectation is a `VAR_TEMP_*` set before
a `warp` is judged on finishing, not on the variable.

## D101 — every overworld phone call was dead

`pokegearcall` sets a phone call up on its own script context and the calling
script then sits in `waitstate` until it ends. Only `src/pokegear.c` ticked that
context — the Pokégear UI's own loop — so a call raised from the overworld was
set up and never run, and the script that asked for it waited for ever. Mom's
call on Route 31, the first scripted call in the game, froze the player on their
second map.

CrystalDust's `OverworldBasic()` calls `PhoneScriptContext_RunScript()` before
`ScriptContext_RunScript()`; the merge kept expansion's side of that function
and the line went with it. Restored, with the include it needs. This is the
same failure as D97 and D99 and is now the twelfth confirmed instance.

Found by the progression harness: Route31's beat was the one that stalled in
`CONTEXT_WAITING` with an empty screen.

## D102 — `showmonpic` emitted four bytes and read five

CrystalDust added a fourth argument to `showmonpic` (shiny) and expansion's
macro does not have it. The merge took expansion's macro and CrystalDust's
command implementation, so every `showmonpic` in the game wrote a four-byte
argument list that `ScrCmd_showmonpic` read five bytes out of. The script
stream then resumed one byte short: the starter-choice script showed the wrong
species' picture and ended in the middle of Elm's question, leaving the player
locked in a lab with no Pokémon and no dialogue.

Added `.byte \shiny` to the macro. A sweep comparing every macro's emitted byte
count against its command's `ScriptRead*` calls found no other genuine
mismatch (the rest resolve through the `map`, `warp_arg` and `stringvar`
sub-macros or through `.ifb` branches).

## D103 — two scripts left the script context during a warp

`EcruteakCity_Gym`'s "the gym leader is out" escort and
`NewBarkTown_PlayersHouse_2F`'s room randomiser both end `warp` / `release` /
`end` with no `waitstate`. Expansion asserts on that (`src/script.c:112`,
"Leaving script while a warp is in progress"), so both were a red error screen
in any build with assertions on. Added `waitstate` to the `.pory` source and the
generated `.inc` in both.

## D104 — the pre-play-test gates: a release build, and a save that can be loaded

Everything before this point was verified on a debug build, from a run that
started at power-on. Two things a human play-test depends on had therefore
never been exercised at all.

**The release build.** `RELEASE=1` turns off the debug overworld menu — and with
it the D96 warp hook — and switches on LTO. It builds clean, boots through the
CrystalDust title, and plays the whole opening to the ground floor exactly as
the debug ROM does. ROM is 28,790,724 bytes, 85.80% of the cart: the release
figure is lower than the debug one because the debug menus and their strings are
gone. `gmake check` is green at the same time: 5158 passing, 0 failing.

A note for whoever builds next: use `gmake`. macOS's own `/usr/bin/make` is GNU
Make 3.81 and does not fail on this repo — it grinds, 40 minutes of implicit-rule
search at 100% CPU before the first object file.

**Saving.** `playtest.c` opened the ROM with no save file on purpose, so that
runs could not become order-dependent, and it had no way to power-cycle. The
consequence is that in seven phases nobody had ever loaded a save. Added a
`reset` command — `core->reset`, which keeps the flash memory the game wrote —
and with it the sequence that matters: save from the start menu, reset, CONTINUE.
The main menu offers CONTINUE with the right name, badges and time, and the load
puts the player back in the house where they saved. It works; it is now known to
work.

While the script was passing through, two earlier fixes were confirmed on screen
rather than in memory: the starter prompt shows Chikorita with its question
intact (D102), and the first patch of grass on Route 29 gives a Lv2 Hoothoot
from Johto's table with a working battle.

### The last live 4-byte tileset

D99 noted that CrystalDust's own `powerplant` directory carries FRLG's
4-byte-per-metatile attributes while the engine reads `metatileAttributes` as
`u16`, and left it for the play-test list. It is the only such directory still
referenced by a layout — the other 63 are the dead `*_frlg` orphans — so it is
fixed here instead: converted to Emerald's 2-byte format,
`(v & 0x1FF) | (((v >> 29) & 3) << 12)`, behaviour and layer type kept, FRLG's
separate terrain and encounter fields dropped because Emerald encodes both in
the behaviour. Without it every metatile in the Power Plant took the behaviour
of the metatile two slots before it.

The static audit's remaining 39 problems are all in secret bases, the Battle
Frontier and Hoenn — parked content, no reachable map among them.

## D105 — the summary screen drew two layouts at once, and the harness had been lying about colour

Two findings from the first look at the menus, one in the game and one in the
tool that had been reporting on it.

### The harness swapped red and blue in every screenshot it ever took

`Screenshot()` read mGBA's frame buffer as `0xAABBGGRR` reversed — it wrote
B, G, R. mGBA's desktop build puts red in the low byte (`M_COLOR_RED` is
`0x000000FF` in `mgba/core/interface.h`), so every PNG this harness has
produced since D95 has had its red and blue channels exchanged.

Nothing in D93–D104 rested on colour — the tileset work was about which block
was drawn, not what hue it came out — but it made every screenshot look wrong
in a way that invited wrong conclusions: brown Hoothoot rendered blue, wooden
floors rendered blue, and a red Poké Ball rendered blue. The fix is one line.
Confirmed against palette RAM: the Hoothoot in a Route 29 battle now matches
`graphics/pokemon/hoothoot/normal.pal` byte for byte.

### The summary screen: expansion's code, CrystalDust's art

`src/pokemon_summary_screen.c` is byte-identical to expansion's — CrystalDust's
own 3763-line version was dropped whole in the Phase 1 merge, and its
`gSummaryScreen*Tilemap` symbols survive in `src/graphics.c` referenced by
nothing but `include/graphics.h`. The *assets* under `graphics/summary_screen/`,
however, were CrystalDust's: `tiles.png`, `page_info.bin`, `page_info_egg.bin`,
`page_skills.bin`, `markings.pal` and `move_select.png` are files both projects
ship under the same name, and the merge kept CrystalDust's.

So expansion's window templates were drawn on top of CrystalDust's page art at
CrystalDust's coordinates, over a background whose palette was never loaded:
two sets of labels overlapping on magenta. The screen was unreadable.

Fixed by restoring expansion's six files. Both pages now render correctly.
This is the thirteenth confirmed "Phase 1 merge kept one side", and the first
where the side kept was CrystalDust's rather than expansion's.

**Recorded, not dropped:** CrystalDust's Crystal-styled summary screen is gone,
and was already gone before this change — the code that drew it did not survive
Phase 1. Restoring it means porting 3763 lines against an engine whose summary
screen has since grown IV/EV display, the contest pages and teachable moves.
That is a feature port, not a merge fix, and it is out of scope here.

### Not a defect: the bag's striped background

The Bag draws magenta and blue stripes, which looks broken. It is not: the
tiles, the tilemap and the palette in VRAM all match `graphics/bag/menu.png`,
`menu.bin` and `menu_male.pal` exactly, and all three are byte-identical to
vanilla pokeemerald's. Indices 12 and 13 are the trainer's stripe colours —
blue and purple for the male palette, two pinks for the female one. Stock
Emerald art, kept.

### Still to look at

The same "shared filename, kept the other side's copy" pattern covers other UI
directories, and the ones where the art is coupled to a layout are the ones
that can break the same way: `graphics/pokedex` (18 tilemaps),
`graphics/pokemon_storage` (5), `graphics/battle_interface` (13) and
`graphics/text_window` (3). Each needs the same check — whose code reads it —
before a human play-test trusts those screens.

## D106 — the message box was CrystalDust's art read by expansion's code

Following D105's list of directories to check, `graphics/text_window` gives the
same pattern, and this one is arithmetic rather than judgement.

`src/text_window.c` is byte-identical to expansion's, and expansion's line 101
reads:

    LoadBgTiles(GetWindowAttribute(windowId, WINDOW_BG), gMessageBox_Gfx, 0x1C0, destOffset);

`0x1C0` is 448 bytes: fourteen 4bpp tiles, which is exactly expansion's
`graphics/text_window/message_box.png` at 56x16. CrystalDust's own line reads
`0x280` — 640 bytes, twenty tiles, its 40x32 box. The merge kept CrystalDust's
PNG, so the game was loading the first fourteen tiles of a twenty-tile sheet
laid out for a different frame: the corners and edges came from the wrong
places.

Restored expansion's `message_box.png`. This also settles it in favour of the
wide Emerald textbox, which is the decision already on record from Phase 5.
Verified on screen: talking to Mum now draws a clean wide box with correct
corners.

### Checked and left alone

`graphics/battle_interface`'s thirteen files are CrystalDust's, and the
healthbox PNGs have different dimensions from expansion's — 64x128 where
expansion has 128x64. That one is safe: `src/graphics.c` converts them with
`-mwidth 8 -mheight 8`, which walks the image in 64x64 metatiles, so a 64x128
image and a 128x64 image produce the same tile order and the same byte count.
They are restyled art of the right shape.

`graphics/text_window/1.png` (24x24 both sides) and `text_pal2.pal` are frame
styling with no size mismatch, and stay CrystalDust's.

Still unchecked: `graphics/pokedex` (18 files, several with real dimension
changes — `menu.png` is 128x144 against expansion's 128x128) and
`graphics/pokemon_storage` (5). Both consumers are merged files rather than
either side's, so neither can be judged by the byte-count trick used here; they
need to be opened and looked at.

## D107 — the second round of human-testing observations

Six items from `observations.txt`. Five of the six are the same failure: a file
taken whole from one upstream, paired with the other upstream's assets. The
sixth (the shadows) is an expansion feature that does not survive contact with
this tree's day/night code.

**1. Title screen.** `src/title_screen.c` was expansion's, so the ROM built
Emerald's Rayquaza title over CrystalDust's art. Ported CrystalDust's title
screen onto this tree's pipeline: `INCBIN_U32(".4bpp.lz")` → `INCGFX_U32(".png",
".4bpp.smol")`, `LZ77UnCompVram` → `DecompressDataWithHeaderVram`,
`m4aSongNumStart` → `m4aSongNumStartGbs` (D32). Restored the four CrystalDust
palettes — `pokemon_logo.gbapal` had been expansion's even though
`pokemon_logo.png` was CrystalDust's — and rebuilt `gTitleScreenBgPalettes` in
`src/graphics.c` from logo + emblem + press-start, 448 + 32 + 32 = 0x200.
CrystalDust's sound-test combo is dropped: there is no `CB2_StartSoundCheckMenu`
in this tree.

*Dropped, deliberately:* expansion's Rayquaza title screen.
`gTitleScreenEmeraldVersionGfx`, `gTitleScreenEmeraldVersionPal` and
`gTitleScreenCloudsTilemap` are now unreferenced. `gTitleScreenAlphaBlend[64]`
was kept — `src/intro.c` still uses it. CrystalDust's intro is *not* ported;
`src/intro.c` is still expansion's Emerald intro.

**2. Down arrow.** `graphics/fonts/down_arrow.png` was CrystalDust's 128×16
sheet; `src/text.c` is byte-identical to expansion's and reads an 8×48 strip
(`sDownArrowYCoords[] = { 0, 1, 2, 1 }`, blitting 8×16). CrystalDust's
`gflib/text.c` indexes the same art by x. Restored expansion's 8×48 art.
`graphics/fonts/keypad_icons.png` is also CrystalDust's but is 128×32 either
way, so it was left alone.

**3. Town map.** Two faults. `src/field_region_map.c` was expansion's Hoenn wall
map; rewrote it as a port of CrystalDust's onto the `CDMap_*` API (D33), with
`Alloc()` for `malloc()`. And all four
`graphics/region_map/mapsec_layout_*.bin` still held CrystalDust's Johto-first
mapsec numbering (`MAPSEC_NEW_BARK_TOWN = 0x00`) against our auto-generated
Hoenn-first header, so the map named Johto tiles after Hoenn towns. Remapped all
four by matching `#define MAPSEC_*` names: 96 of 98 ids in use matched exactly,
and `ROUTE_3_FLYDUP`/`ROUTE_10_FLYDUP` were folded onto `ROUTE_3`/`ROUTE_10`.

Note for later: nothing in `data/` calls `FieldShowRegionMap`. `EventScript_RegionMap`
exists in `data/event_scripts.s` but no map's `bg_events` reference it, so the
wall map is currently unreachable. Worth wiring up when the Pokémon Centers are
revisited.

**4. Naming screen.** `src/naming_screen.c` is expansion's, and its
`NamingScreen_CreatePlayerIcon` asks for the *rival's* avatar
(`GetRivalAvatarGraphicsIdByStateIdAndGender`) — Emerald's Brendan/May. Now
`GetPlayerAvatarGraphicsIdByStateIdAndGender`, as CrystalDust does.
`NamingScreen_CreateRivalIcon` built a bespoke sheet over
`OBJ_EVENT_GFX_RED_NORMAL`; it now draws `OBJ_EVENT_GFX_RIVAL` as a plain
object-event sprite. `sRival_Gfx`, `sRival_Pal` and `sAnims_Rival` removed;
`graphics/naming_screen/rival.png` and `rival.pal` are now unreferenced.

**5. Shadows.** Expansion ships `OW_OBJECT_VANILLA_SHADOWS FALSE`, giving every
object event a second sprite for a shadow drawn with `ST_OAM_OBJ_BLEND`. The
day/night code owns `BLDCNT` here, so the blend never applies and the shadow
renders as a white blob — the "halo" under every sprite. Set to `TRUE`, so a
shadow is drawn only mid-jump. Matches CrystalDust, and hands back an OAM slot
per object.

*Dropped, deliberately:* expansion's always-on object shadows. Turning them back
on would need `REG_OFFSET_BLDALPHA` set up in `src/field_effect_helpers.c`
(the `BLDALPHA_BLEND(8, 12)` line there is commented out) without fighting the
day/night blend.

**6. Water animation.** `src/tileset_anims.c` had CrystalDust's
`TilesetAnim_General` driver and CrystalDust's art (16×184 frames, 46 tiles at
tiles 416–461), but expansion's `QueueAnimTiles_General_Water`, which writes 30
tiles at tile 432. The copy missed the water entirely and landed on its
neighbours. Restored CrystalDust's `TILE_OFFSET_4BPP(416)`/`0x600`. The
neighbouring `WaterFast` (464), `Whirlpool` (488) and `Flower` (508) queues had
survived the merge intact, which is why only the still water looked frozen.

**Harness.** `tools/playtest/scripts/newgame.txt` was retuned: CrystalDust's
title combs in for about three seconds before it accepts input, and the clock
prompt defaults to NO, so every A is now paired with an UP — harmless on a text
box, picks YES on a yes/no prompt.

Evidence: `docs/port/evidence/D107/index.html`.

**Still unchecked:** `graphics/pokedex` (18 files; `menu.png` is 128×144 against
expansion's 128×128, loaded with an `0x2000` cap) and `graphics/pokemon_storage`,
both carried over from D106. Also noticed but not reported and not investigated:
dialogue text colour varies by speaker (Elm blue, Mum red, signs black).

## D108 — third round human-testing observations

Six more lines in `observations.txt`. Four fixed, two left open and still in the
file. Evidence: `docs/port/evidence/D108/index.html`.

**1. "some npcs are coloured as negatives" — not fixed.** Not reproduced. New
Bark Town, Elm's lab and Route 30 all look right. An audit of object events whose
art comes from one upstream and whose palette tag resolves to the other's `.pal`
turns up mostly Hoenn sprites (out of scope), plus `ApricornTree` (expansion art,
CrystalDust `npc_3`) and `RivalBrendan*`/`RivalMay*` (CrystalDust art, expansion
palettes). Any of those could be the reported NPC; none of them is obviously
inverted on screen. Needs the map and the NPC from the tester.

**2. Poké Ball overworld sprite.** Table-lost-but-assets-kept, again.
`graphics/object_events/pics/misc/item_ball.png` survived the merge but
`gObjectEventPic_ItemBall`, `sPicTable_ItemBall` and
`gObjectEventGraphicsInfo_ItemBall` did not, so `OBJ_EVENT_GFX_ITEM_BALL`
resolved to expansion's `gObjectEventGraphicsInfo_PokeBall` — the *follower*
ball, a five-frame 80×32 sheet drawn 16×32 under `OBJ_EVENT_PAL_TAG_NPC_3`,
which in this tree is CrystalDust's palette file. Restored all three pieces
(16×16, one frame, inanimate, `OBJ_EVENT_PAL_TAG_NPC_4` / `PALSLOT_NPC_4`, to
match CrystalDust's literal slot 5) and repointed the enum.
`OBJ_EVENT_GFX_POKE_BALL` still points at expansion's follower ball; that entry
exists for the follower feature and is correct as it stands.

**3. Apricorn tree crash.** `src/field_control_avatar.c` is expansion's copy and
expansion has no `BG_EVENT_FRUIT_TREE` arm. Everything else CrystalDust's fruit
trees need was already here — `src/fruit_tree.c`, the `GetFruitTreeItem` and
`SetFruitTreeMetatileTakenFromId` specials, D83's metatile work, the
`fruit_tree` bg events in the map JSON — but with no `case` to catch them the
event fell through to `return bgEvent->bgUnion.script;` and the tree *id*, a
small integer, was executed as a script pointer. Added the arm, ported
`EventScript_FruitTree` into `data/scripts/obtain_item.inc`, its three strings
into `data/text/obtain_item.inc`, and the extern into `include/event_scripts.h`.

**4. Battle back sprite was Brendan's.** `GetPlayerTrainerPic` is expansion's and
`GAME_VERSION` is `VERSION_EMERALD`, so every ordinary battle went through
`GetEmeraldTrainerPic`. CrystalDust's Gold and Kris back pics were in the repo
but unreferenced. Added them and their palettes to `src/data/graphics/trainers.h`
with CrystalDust's five-frame throw animation (`sAnimCmd_Johto`/`sBackAnims_Johto`),
filled in `TRAINER_PIC_GOLD` and `TRAINER_PIC_KRIS`, and pointed the
`VERSION_EMERALD` branch at a new `GetJohtoTrainerPic`.

*Deliberate drop:* `GetEmeraldTrainerPic` is **deleted**, not left unused — an
unused static is a build error here. Brendan and May are therefore no longer
reachable as *player* back pics; the pics themselves stay in the table for the
linked-save paths. Consistent with the no-Hoenn scope, but recorded because it is
a removal, not a redirection.

*Not verified in battle.* The harness cannot reach a wild encounter from a fresh
save — `afterintro.txt` leaves the player in the 2F bedroom with no party, so
`mash UP` in Route 30 grass does nothing. The evidence page shows the sheets that
are now wired in, and says so.

**5. "intro screen oak text box still not rendering properly" — not fixed, but
narrowed.** The *field* box is provably correct: in Elm's lab the frame row reads
`0x201, 0x203, 0x204×26, 0x205, 0x206` at palette 15, exactly what
`WindowFunc_DrawDialogueFrame` writes over `gMessageBox_Gfx` at
`DLG_WINDOW_BASE_TILE_NUM`, and it looks right on screen. So D106's fix holds and
this is not a general message-box defect. The main-menu speech path
(`src/main_menu.c`, `BIRCH_DLG_BASE_TILE_NUM` `0xFC`) produces
`0xFC, 0xFD, 0xFE×26, 0xFF, 0x100` on the same row — the right five-piece shape
but *consecutive* tile numbers and no flip bits, which matches no frame function
in `src/menu.c`. Ruled out: the art (ROM bytes == `message_box.png.4bpp`),
palette 15 (VRAM == `message_box.png.gbapal` exactly), and the window geometry
(`sNewGameBirchSpeechTextWindows[0]` is identical to the field window's and both
are vanilla). What remains is whatever re-points the tile base between
`LoadMessageBoxGfx(0, 0xFC, ...)` and the draw.

**6. Frame style not reflected in options.** `src/option_menu.c` is expansion's,
which loads the window border once at init. Changing FRAME TYPE therefore moved
the number but left the menu's own border on the saved style. CrystalDust reloads
tiles and palette from the input handler; added the same, at the offsets init
uses (`0x120` bytes to `0x1A2`, `BG_PLTT_ID(7)`).

## D109 — fourth round human-testing observations

Seven lines in `observations.txt`. Four fixed, three left open and still in the
file. Evidence: `docs/port/evidence/D109/index.html`.

**1. "some npcs are coloured as negatives" — still open, but reproduced.** The
scientist in Elm's lab renders with the right skin and hair and the wrong
clothes: navy, orange and pale blue across the shoulder row. Skin and hair live
in the low palette indices that every NPC palette shares; clothes live in the
high indices where they differ. So the sprite is drawn against the wrong loaded
palette, not the wrong art.

Two dead ends, recorded so the next pass skips them.
`graphics/object_events/palettes/npc_white.pal` and `npc_4.pal` are
byte-identical in this tree and both match `scientist.png`'s own embedded
palette exactly, so `gObjectEventGraphicsInfo_Scientist` naming
`OBJ_EVENT_PAL_TAG_NPC_WHITE` where CrystalDust names `OBJ_EVENT_PAL_TAG_NPC_4`
cannot by itself be the fault. And `.paletteSlot` is vestigial in expansion —
palettes are allocated dynamically by tag, and the only remaining uses of the
`PALSLOT_*` constants in `src/event_object_movement.c` are the reflection and
tag-to-slot tables — so `PALSLOT_NPC_4` in that struct is not doing damage
either. A dump of OBJ palette RAM inside the lab (`dump 0x05000200 0x200`)
found npc_white loaded and intact in slot 4. What is left is to read the
scientist sprite's `oam.paletteNum` at runtime and find who assigned it.

Capture note for whoever picks this up: Professor Elm's opening cutscene cannot
be escaped from a fresh save, so every frame of the assistant is half behind the
message box. Set the story flags first.

**2. Intro Oak text box frame — still open.** Carried unchanged from D108. The
intro path emits `0xFC 0xFD 0xFE×26 0xFF 0x100` as its frame tile run, matching
no frame function in `src/menu.c`. Not touched this round.

**3. Dialogue box ignores FRAME TYPE — still open, and it is a design call.**
Neither upstream ties the overworld message box to `optionsWindowFrameType`.
Emerald's battle box reads the option; its field box does not. CrystalDust's
field box is a fixed Gen 2 frame. "Changes in battle, not in dialogue" is
therefore faithful to both parents, and D107's fix to the *options menu's own*
border did not change that. Making the field box follow the option is a feature
request, and it first needs a decision on whether CrystalDust's frame art gets
the same nine-way set the Emerald frames have.

**4. Followers on by default.** `OW_FOLLOWERS_ENABLED` in
`include/config/overworld.h` set to `TRUE`. Gen 2 introduced the walking
partner and HG/SS — the games this port is dressed as — put the party lead on
screen, so followers off by default reads as missing content rather than as a
setting. `OW_FOLLOWERS_BOBBING`, `OW_FOLLOWERS_POKEBALLS` and
`OW_FOLLOWERS_SCRIPT_MOVEMENT` were already `TRUE` and were left alone.
Confirmed in game. Cost: ROM 29,336,816 B (87.43 %), EWRAM 85.30 %, IWRAM
86.98 %.

**5. Apricorn tree gave an Oran Berry.** Gen 2 mixed berry trees and apricorn
trees on the same sprite and CrystalDust's `sFruitTrees[]` keeps that — six
trees bear apricorns, the rest bear berries — while the tree the player walks up
to is drawn as an apricorn tree either way. Per the explicit call in the
observation, Gen 4 behaviour wins: every entry in `sFruitTrees[]` now bears an
apricorn. The six that already did keep theirs; the rest take the apricorn whose
colour matches the berry they replaced, so the fruit on screen does not change
colour — Oran/Rawst → Blue, Pecha/Persim → Pink, Cheri/Leppa → Red, Chesto →
Green, Aspear → Yellow. Verified on Route 30: the tree at (14, 5),
`FRUIT_TREE_ROUTE_30_2`, formerly Pecha, now yields a Pink Apricorn.

**6. Wrong tiles on the Pokémon Center door animation.** Three faults in
`src/field_door.c`, all the same Phase 1 story: expansion's table survived and
CrystalDust's did not.

- *Geometry.* Five rows still claimed `DOOR_SIZE_1x2` while the art shipped with
  them is 16×48, i.e. three 1×1 frames: `METATILE_General_Door`,
  `METATILE_General_Door_PokeCenter`, `METATILE_General_Door_Gym`,
  `METATILE_BattleFrontier_Door_Elevator` and
  `METATILE_BattleFrontierOutsideEast_Door_BattleTower`. The animation read two
  tile rows where it should read one and painted the second over the metatile
  above the door, which is why the opening swallowed the Poké Ball sign on the
  Center's facade. Changed to `DOOR_SIZE_1x1`.
- *Palettes.* `sDoorAnimPalettes_General`, `_PokeCenter` and `_Gym` held
  Emerald's slot numbers against CrystalDust's General tileset art — the
  teal-and-grey smear in the before capture. Changed to CrystalDust's 2, 7, 7.
- *Missing doors.* CrystalDust's 24 Johto and Kanto rows were not in
  `sDoorAnimGraphicsTable[]` at all, so New Bark, Elm's lab, Violet, Azalea,
  Goldenrod and its department store and elevators, the Radio Tower lifts,
  Ecruteak, Olivine and its lighthouse, the Goldenrod Underground, Pallet Town
  and Fuchsia had no door animation whatsoever. All 24 restored, with 22
  `sDoorAnimTiles_*` and 21 `sDoorAnimPalettes_*` arrays.

The eight Johto tilesets those rows key off — `gTileset_NewBark`, `_Violet`,
`_Azalea`, `_Goldenrod`, `_RadioTower`, `_EcruteakCity`, `_OlivineCity`,
`_Underground` — were defined in `src/data/tilesets/headers.h` but never
declared in `include/tilesets.h`, the orphaned-header variant, so
`field_door.c` could not name them. Declarations added.

Three restored rows needed fresh symbol names because CrystalDust's names
collide with symbols that live inside `#if IS_FRLG` in this file and this is not
an FRLG build: `sDoorAnimTiles_GoldenrodDeptStoreElevator`,
`sDoorAnimPalettes_GoldenrodDeptStoreElevator` and
`sDoorAnimPalettes_KantoFuchsia`.

**7. "game crashed on healing in pkmn center" — not a crash.** The game keeps
running, the nurse finishes her animation, the party heals and the player can
walk away. What stops is text: from the moment the heal completes, every message
box in the save comes up empty, which is what a crash looks like from the
outside.

A new instance of the merge's uninitialised-buffer class — a buffer whose
initialiser moved behind a guard. `src/union_room.c` declared

    static EWRAM_DATA u8 sUnionRoomPlayerName[12] = {};

Emerald zero-filled it and relied on `InitUnionRoom()` to write the `EOS` before
anything read it. CrystalDust's nurse (D48) only calls `InitUnionRoom()` when a
wireless adapter is connected (`CableClub_OnResumeFunc`, guarded by
`IsWirelessAdapterConnected()`), so on a normal single-player save the buffer
was still twelve zero bytes — not a terminated string — when the nurse ran
`specialvar VAR_RESULT, BufferUnionRoomPlayerName` after healing. It reaches
that line because `ShouldCheckForUnionRoom()` returns TRUE
(`OW_UNION_DISABLE_CHECK` is `FALSE`, `OW_FLAG_MOVE_UNION_ROOM_CHECK` is 0) and
`PlayerNotAtTrainerHillEntrance()` returns TRUE outside Trainer Hill, so
`EventScript_PkmnCenterNurse_CheckTrainerHillAndUnionRoom` falls through to the
second `specialvar`.

`StringCopy` then ran off the end of the buffer hunting for an `EOS` and wrote
the overrun into `gStringVar1` (0x02035F3C), whose 256 bytes are followed in
EWRAM by `sFirstTextPrinter` (0x0203603C), `gTextFlags` (0x02036040),
`gDisableTextPrinters` (0x02036044) and `gFonts` (0x02036048). `gFonts` ended up
NULL and `AddTextPrinter` bailed out of every later message. Traced live, opcode
38 being `ScrCmd_specialvar`:

    game: HPP enter gFonts=08D5A68C cnt=1
    game: HPP exit  gFonts=08D5A68C
    game: SCR cmd 38 nulled gFonts
    game: FMB A gF=00000000
    game: ATP no gFonts

Fix: start the buffer terminated (`EWRAM_INIT u8 sUnionRoomPlayerName[12] = {
EOS };`), re-terminate the last byte on each read, and skip the `StringCopy`
when the first byte is `EOS`.

Ruled out before the trace landed, and not worth revisiting: the movement action
tables, `sAnimTable_Nurse` and `ANIM_NURSE_BOW`,
`MovementAction_NurseJoyBowDown_Step0`, `FreezeObjectEvent`, the heal field
effect itself, text speed and instant render, text-printer heap exhaustion,
`DeactivateAllTextPrinters`, the nurse `.inc`,
`TrySpawnNamebox`/`PrepareNamebox`, `ContextNpcGetTextColor` and
`HealPlayerParty`.

**Harness note.** `tools/playtest/playtest.c` has no `walk`-like semantics for
`hold`: `hold KEYS` latches the keys and runs no frames at all. `hold UP 5`
therefore does nothing but latch, and a script of `hold`/`shot` pairs produces N
byte-identical screenshots. Use `walk DIR N` (16 frames down, 4 up, per step) or
`hold` followed by explicit `wait`.

## D110 — fifth round human-testing observations

Three lines in `observations.txt`. Two fixed, one fixed in part and the
remainder written down as a design call for the next round. Evidence:
`docs/port/evidence/D110/index.html`.

**1. "the intro screen oak text box frame still not rendering properly" —
fixed.** D109 left this open with the note that the intro emits `0xFC 0xFD
0xFE×26 0xFF 0x100` as its frame tile run and that no frame function in
`src/menu.c` matches. The observation was right and the search space was wrong:
the frame is drawn in `src/oak_speech_crystal.c`, not `src/menu.c`. That file is
CrystalDust's intro, and it is the one that runs — `src/oak_speech.c` does not.

`NewGameOakSpeech_CreateDialogueWindowBorder` was a hand-written 185-line
tilemap that painted `OAK_SPEECH_WINDOW_BASE_TILE_NUM + {0,1,2,3,4,5,6,8,9,10,
11,12,13}` and their `BG_TILE_V_FLIP` counterparts. That layout belongs to
CrystalDust's twenty-tile message box. D106 replaced `message_box.png` with
expansion's fourteen-tile sheet, so from D106 onward the intro was indexing art
that no longer matched it — the same shape of defect D106 itself fixed, one
caller further on. The tell was the asymmetry: the *clear* path in the same file
already called expansion's `ClearDialogWindowAndFrame`.

Measured live off BG0's tilemap (screen base `0x0600F800`, rows 14-19) to
confirm the run before changing anything.

Fix: `NewGameOakSpeech_ShowDialogueWindow` now calls
`DrawDialogFrameWithCustomTileAndPalette(windowId, copyToVram,
OAK_INTRO_DLG_BASE_TILE_NUM, 15)` — the field message box's own frame, drawn at
the base tile the intro loads the art to. The hand-written border function and
its forward declaration are deleted. `sOakIntroTextWindows[0].width` goes 26 →
27: with `left = 2`, `WindowFunc_DrawDialogueFrame` paints `left-2 .. left+width`,
so 27 spans x = 0..29 and matches the field box exactly, and 27×4 = 108 tiles
fills baseBlock 1..0x6C, abutting window 1's 0x6D with nothing to spare and
nothing overlapping.

The observation's "during battle" qualifier is not reproduced and, on the code,
should not be: the battle message box is `gBattleTextboxTiles` /
`gBattleTextboxTilemap` / `gBattleTextboxPalette`, a prebuilt asset that D106
never touched, and the battle *menu* windows come from `LoadUserWindowBorderGfx`,
which follows the FRAME TYPE option. The qualifier looks like residue from the
deleted frame-type line above it in `observations.txt`. Left in the file for the
tester to confirm or drop.

**2. "some npcs are coloured as negatives, such as the assistant in elm's lab" —
fixed.** Open since D109, and D109's diagnosis was wrong. It reported that "a
dump of OBJ palette RAM inside the lab found npc_white loaded and intact in slot
4". Slot 4's tag is `OBJ_EVENT_PAL_TAG_NPC_4` (0x1106), and
`graphics/object_events/palettes/npc_4.pal` is byte-identical to `npc_white.pal`
— so the bytes looked right while the tag was wrong, and the real question went
unasked.

Measured, in Elm's lab and in the Cherrygrove mart:

* the aide renders with `oam.paletteNum = 15`;
* OBJ palette 15 held `0000×6 04E1 1124 1D87 2DEB 3A2E 4691 56F5 6338 6F9B 7FFF`,
  a ten-step green→white ramp; a byte search for `e1042411871d` across
  `build/assets` matched exactly one file,
  `graphics/intro/scene_1/drops.pal.gbapal` — the boot intro's raindrops, never
  overwritten;
* `sSpritePaletteTags` held only `1200 1170 1100 1172 1106`, slots 5-15 free, so
  this is not slot exhaustion;
* instrumenting `LoadObjectEventPalette` showed
  `FindObjectEventPaletteIndexByTag(OBJ_EVENT_PAL_TAG_NPC_WHITE)` returning
  `0xFF`. `0xFF` stored into `oam.paletteNum`, a four-bit bitfield, truncates to
  **15**. Slot 15 is the last OBJ palette and holds whatever was left there.

The lookup misses because the entry is compiled out. Dumping the built table out
of the ROM (`sObjectEventSpritePalettes` at `0x084B131C`) showed 79 entries
ending at `OBJ_EVENT_PAL_TAG_NONE`, with 0x1125-0x1133 absent: twelve palettes
sat behind `#if IS_FRLG`, which is false for this build.

This is D97 half-applied. D97 ungated the FRLG *pics* (`object_event_graphics.h`)
and the FRLG *graphics infos* (`object_event_graphics_info.h`) because
CrystalDust uses those sprites throughout Johto — but left the *palette table* in
`src/event_object_movement.c` gated. Graphics infos compiled in, their palettes
compiled out. 128 graphics infos name one of the nine missing tags:
`OBJ_EVENT_PAL_TAG_NPC_WHITE` (47), `NPC_BLUE` (38), `NPC_PINK` (26),
`NPC_GREEN` (17), `PLAYER_RED` (10), `PLAYER_GREEN` (5), and one each of
`SS_ANNE`, `SEAGALLOP` and `METEORITE`. Every one of them was rendering in
whatever palette slot 15 happened to hold — the "negatives" the tester saw, in
Elm's lab and everywhere else.

Fix: drop the `#if IS_FRLG` / `#endif` around those twelve rows. 384 bytes of
palette and 96 bytes of table; ROM 29,346,128 → 29,346,256 B (87.46%).

Verified in the Cherrygrove mart, same script, same frame: the shopper by the
shelves is a green-and-white smear before and a purple-hatted customer after. On
Route 29 `sSpritePaletteTags` now carries 0x112C where it previously could not.

**3. "ensure colour of apricorn matches apricorn given by tree" — the item was
already right; the tree art was wrong, and is now neutral rather than matched.**
D109's comment in `src/fruit_tree.c` claimed the trees "take the apricorn whose
colour matches the berry they replaced". The premise was false. There is exactly
one fruit metatile, `METATILE_General_FruitTreeTop` (0x22D), shared by every tree
in the game; its fruit comes from tiles 484/485 drawn with palette 0 indices
9/10/11 of the General primary tileset, which were `(255,197,115) (238,131,106)
(197,49,65)` — red. Every tree in Johto and Kanto bore a red fruit while handing
out seven different apricorns.

Per-tree art is not available at that layer. The General primary tileset has two
free palette entries in the whole file (pal 0 index 7, pal 4 index 2), and a
primary tileset may only use pals 0-6 (`NUM_PALS_IN_PRIMARY`). Indices 9/10/11
are fruit-exclusive — only metatiles 0x4 and 0x22D use them at palette 0, both
fruit-tree tops — so the ramp can be repainted freely, but it is *one* ramp,
shared by every tree drawn at once. Route 37 and Route 42 each show three trees
of three different colours.

Taken: repaint the ramp to a neutral apricorn amber, `(255,213,148)
(246,172,82) (197,106,24)`. The tree now reads as "an apricorn tree" and only the
item it yields carries a colour, so nothing on screen contradicts what is picked.
`sFruitTrees[]` keeps all seven colours, which is what Kurt's ball variety needs.

Not taken, and recorded so the choice is not lost:

* *One colour per map plus a runtime palette patch.* Visually exact. Costs the
  within-map variety on Routes 37 and 42, and a patch of a tileset palette has to
  survive `TimeMixPalettes` and `UpdateTimeOfDayPaletteFade`, which rewrite those
  entries on every time-of-day transition. Fragile for the gain.
* *Draw the fruit as an object event with per-colour OBJ palettes.* Exactly what
  the observation asks for, and the sprite already exists and is unused —
  `OBJ_EVENT_GFX_APRICORN_TREE` / `gObjectEventGraphicsInfo_ApricornTree`
  (16x16, `OBJ_EVENT_PAL_TAG_NPC_3`). It needs thirty maps edited, seven new OBJ
  palettes, and the pick/regrow path moved off `bgEvents` onto object events. A
  feature, not a defect fix. This is the one to do if the tester wants the fruit
  itself to match.
* *Make every tree give a red apricorn.* Matches the art for free and throws away
  six of the seven apricorn types. Not recommended.

**Superseded.** The tester chose the object-event route; it is implemented in
D111. The amber repaint above is kept and is still doing work — it is the base
layer the sprite sits on, and it is all that shows for a tree standing on a
*connected* map, where object events do not spawn.

**Harness note.** `tools/playtest/scripts/newgame.txt` no longer reaches the
overworld: the CrystalDust intro runs longer than it assumes (D109's followers
work is the likely cause), so its trailing `press`/`shot` pairs land mid-intro
and `afterintro.txt` reports map 0.0. Sixty more `press A` pairs get there. The
scripts want retuning onto `untilmap`/`waitfade` rather than frame counts.
Warping with `gPlaytestWarp` sidesteps it entirely and is what this round used.

## D111 — the apricorn on the tree is an object event

D110 item 3 closed with three alternatives and a recommendation to leave the
fruit neutral. The tester picked the object-event route, so the fruit hanging in
a fruit tree now carries the colour of the apricorn that tree gives. Evidence:
`docs/port/evidence/D111/index.html`.

**Shape of the fix.** One 16x16 pic, `graphics/object_events/pics/misc/apricorn_fruit.png`,
lifted straight out of tiles 484/485 of the General primary tileset with the
fruit ramp remapped from tileset indices 9/10/11 to sprite indices 1/2/3, so the
sprite lands pixel-for-pixel on top of the fruit already drawn into
`METATILE_General_FruitTreeTop`. Seven OBJ palettes
(`graphics/object_events/palettes/apricorn_{red,blu,ylw,grn,pnk,wht,blk}.pal`),
seven palette tags `OBJ_EVENT_PAL_TAG_APRICORN_*` (0x117B-0x1181), seven
graphics ids `OBJ_EVENT_GFX_APRICORN_*`, seven graphics infos sharing the one pic
table. Thirty object events across twenty-three maps, each on the tree-top
metatile — one tile above the tree's `BG_EVENT_FRUIT_TREE`, which is where
`src/fruit_tree.c` has always drawn the fruit (`bgEvent.y + 6`, against
`MAP_OFFSET` 7).

The existing, unused `OBJ_EVENT_GFX_APRICORN_TREE` was *not* reused. It is a
whole tree, not a fruit, and it has one palette; the whole point here is seven.

**Priority: the fruit was invisible for the first two builds.** The object events
spawned (`gObjectEvents` showed them active at the right `currentCoords`), their
palettes loaded (`sSpritePaletteTags` carried 0x117B/0x117C/0x1181 on Route 37),
and OAM held three 16x16 entries at the right screen positions — and nothing was
on screen. `sElevationToPriority[]` gives a ground-elevation object
`oam.priority = 2`, and the overworld's top metatile layer is BG1 at priority 1
(`sOverworldBgTemplates`). The tree's own fruit tiles are on that top layer, so
BG1 drew over the sprite exactly where the sprite was. Raising the template
elevation to 4 did not help: `ObjectEventUpdateElevation` overwrites it from the
tile underneath, which is elevation 3. `TrySetupObjectEventSprite` now sets
`fixedPriority`, clears `subspriteTables`, and pins `oam.priority = 1` for the
apricorn graphics ids. `fixedPriority` makes
`UpdateObjectEventElevationAndPriority` return before it can undo that. The fruit
never moves, so nothing else wants its priority.

**Pick and regrow.** The object event's template `flag` is the same
`FLAG_FRUIT_TREES_START + treeId - 1` the pick script sets and
`DoTimeBasedEvents` clears, so `TrySpawnObjectEvents` skips a picked tree on
every later load and spawns it again once it has regrown — no new state. What
template flags do not do is act on an already-spawned object, so
`SetFruitTreeMetatileTakenFromId` (which already had the fruit tile's x,y in
hand) now also calls `GetObjectEventIdByXY` there and removes the apricorn.
Known gap, accepted: if a tree regrows while the player is standing on the map,
the metatile comes back but the sprite does not until the next map load. That is
the same behaviour the metatile-only code had for everything else and it needs a
time-of-day hook to fix properly.

**The amber metatile stays.** D110 repainted the shared fruit ramp from red to
neutral amber and that is still load-bearing: it is what the sprite sits on, and
it is all that shows for a fruit tree standing on a *connected* map, where object
events do not spawn but `SetFruitTreeMetatilesOnConnectedMap` still draws the
tile.

**A flag collision, found on the way — this one was live.**
`FLAG_FRUIT_TREES_START` was `CRYSTAL_FLAGS_START + 849` and the next named flag,
`FLAG_BUENAS_PASSWORD_SET`, was at 850. The block had room for exactly one tree.
Trees 2 through 30 were aliasing 29 named flags: picking the second Route 30
apricorn set Buena's password, the one on Route 31 armed the daily bug-catching
contest, and so on down the daily-event list. This predates the object-event
work — it is a plain indexing bug in shipped code, reachable by any player who
picks a second apricorn. Moved to `CRYSTAL_FLAGS_START + 880` (877 is the highest
offset otherwise used) with thirty named `FLAG_FRUIT_TREE_*` aliases so the next
person to append a flag lands in a named hole rather than on top of the trees,
and `NUM_CRYSTAL_FLAGS` grown 880 -> 912 — 4 bytes of `SaveBlock1`, and the
`SaveBlock1FreeSpace` assertion in `src/save.c` (D12) still passes.

**Cost.** ROM 29,347,712 bytes, 87.46% of 32 MB; the seven palettes and one pic
are 352 bytes of art.

## D112 — the town map freed a block it did not own

Sixth round of human testing, one line:

> pressing B to exit the town map restarts the game

It does, and it is not the B button's fault. Closing the map corrupts the heap;
the reset is the allocator noticing, several frames later, in `src/malloc.c`:

```
game: ASSERTION FAILED  FILE=[src/malloc.c] LINE=[97]  EXP=[block->magic == MALLOC_SYSTEM_ID]
```

`CDMap_InitRegionMapData` in `src/pokegear_map.c` takes a `struct CDRegionMap *`
from its caller and stores it in the file-scope `gRegionMap`. It does not
allocate it. `CDMap_FreeRegionMapResources` nonetheless ended with
`FREE_AND_SET_NULL(gRegionMap)` — the module freeing memory it was only lent.
Both callers are wrong in different ways:

- `src/field_region_map.c` allocates one handler struct and passes the
  `regionMap` member *inside* it, eight bytes in. `Free()` therefore read a block
  header from the middle of the caller's data and unlinked a garbage pointer.
  This is the live path.
- `src/pokegear.c` passes its own `AllocZeroed` block and then freed it again in
  `FreePokegearData`. A plain double free.

Ownership goes back to the caller, which is the only party that knows where the
memory came from. `CDMap_FreeRegionMapResources` now clears the window enable
bits and sets `gRegionMap = NULL`, releasing only the sprites, windows and GPU
state it set up itself. `UnloadMapCard` frees the block `LoadCardBgs` allocates —
that is the right place, because the card allocates a fresh one every time it is
opened and `FreePokegearData` only ever saw the last of them.

**Why it took four cycles to show.** One open/close leaves a corrupt free list
that nothing has walked yet. The assertion fires on a later unrelated `Free()`,
so the repro script drives the whole START → bag → TOWN MAP → B → B cycle four
times; the fourth close came up on the PRET × RHH boot screen with `where`
reporting map 0.0. The fixed build survives eight cycles and ends in the bedroom,
map 1.1.

One wrinkle in reproducing it at all: the "It's the TOWN MAP." field message is
dismissed with **B**, not A, so the first B press opens the map rather than
closing it. The script presses A three times, B to dismiss, then B to close.

**Reachability, since it matters for how much of this is dead code.**
`CB2_InitPokegear` has no callers, so the Pokégear UI is entirely unwired. No
layout in `data/` uses `MB_REGION_MAP` (value 133 — the enum body starts at line
5 of `include/constants/metatile_behaviors.h`, so the value is the grep line
number minus five, a trap I fell into once here), so no wall map is reachable.
And no script grants `ITEM_TOWN_MAP`. The tester got there through the debug
menu, which is currently the only way in. Both call sites were broken all the
same, so the fix lands before either goes live.

Evidence: `docs/port/evidence/D112/index.html`.

## D113 — the play-test harness stops counting frames

No tester observation behind this one. It is the harness note D110 ended on:
`tools/playtest/scripts/newgame.txt` no longer reached the overworld, because
CrystalDust's intro had grown longer than the script's counted presses assumed
(D109's follower work the likely cause), so its trailing `press`/`shot` pairs
landed mid-intro and `afterintro.txt` reported map 0.0. That round was finished
by warping with `gPlaytestWarp`, which sidesteps the opening entirely — and with
it every defect that lives in the opening, which is exactly where D110's own
Oak-frame defect was. Evidence: `docs/port/evidence/D113/index.html`.

**The shape of the bug is that the scripts encoded durations.** `newgame.txt`
was sixty-odd `press A 8 40` lines and `afterintro.txt` a hundred and fifty
more, each one a guess at how long a text box takes. Any content change
invalidates all of them at once, and the failure is silent: the presses run out,
the remaining commands execute against whatever is on screen, and the run
reports a map of 0.0 rather than an error. This was the third retune.

**What replaces them.** The harness already had the right primitives —
`untilmap`, `waitfade`, `advance`, and the `until` they are built on — so the
work was mostly finding a landmark for the parts of the opening that are not on
a map yet. `gMain.callback2` is that landmark, and the new `untilcb <CB2_Name>`
resolves a callback symbol, sets bit 0 for the Thumb entry, and waits for
`gMain+4` to equal it. The opening is then three waits: `CB2_NewGame`, which the
Oak speech's last task sets once the player has shrunk into the map;
`CB2_Overworld`, which is the bedroom; and the fade.

**One new harness command was needed.** The obvious mash for the opening is
`A+UP`: A takes the title screen, NEW GAME, every text box, the gender menu and
the clock, while UP is harmless on a text box and picks YES on a yes/no prompt.
YES matters twice, because both prompts that appear default to NO — the wall
clock's *Is this the correct time?*, and the Oak speech's *So it's <name>?*,
whose NO drops into the naming screen. But a yes/no menu handed A and UP on the
same frame takes the A and keeps the NO, so `mash A+UP` parks on the clock for
ever; thirty thousand frames, callback2 never leaving `CB2_WallClock`. Hence
`mashalt`, five lines in `playtest.c`: a second mashed key tapped in the *other*
half of the mash period, so the two never share a frame. `mash` clears it, so a
plain `mash NONE` still stops everything.

**`newbark.txt` and `afterintro.txt` follow.** Each warp now waits on
`untilmap`, then `untilcb CB2_Overworld`, then `waitfade` — the map number
changes several frames before the fade begins, so `waitfade` alone returns
immediately and the shot lands on a black screen. Mum's speech, which fires the
moment the stairs land and locks the player, is answered with `advance`. The
walks are still counted steps, and should be: a route through a room is a route,
and no state says "four tiles later". Two of them were wrong and had been
hidden by the script never getting that far — the front door is the warp at map
(8,8), seven tiles in from the coordinates `player` prints, and it is an
`MB_SOUTH_ARROW_WARP`, so standing on it does nothing and the last step has to
be a step south *through* it. The old script walked down four tiles into the
middle of the room and shot a picture of the living room called `outside.ppm`.

`afterintro.txt` is now three lines. Its callers, `progression.py` and
`sweep.py`, snapshot the state it leaves and teleport out of it, so where the
player stands matters less than that no script is running; it ends outside in
New Bark rather than on the ground floor.

**One thing the release build needed.** `RELEASE=1` turns LTO on, and LTO
renames file-local statics to `name.lto_priv.N`, so `advance` — which watches
the static `sGlobalScriptContextStatus` — died on an unknown symbol against the
release ROM. `load_symbols` now maps a plain name onto its renamed symbol when
there is exactly one candidate, so a script does not have to know which build
it is running against.

**Verification.** `newgame.txt` reaches the bedroom in 3629 frames against the
old script's 4768, `newbark.txt` reaches New Bark Town, `sweep.py` runs, and
`progression.py` reports 35 beats run, 0 failed against the state the rewritten
`afterintro.txt` leaves. All of it runs unchanged against the release ROM.

**And the next round's build.** `gmake RELEASE=1` is green: 28,830,912 bytes
used, 85.92% of the cart, up 40,188 bytes on D104's figure. D104's other gate,
the save, is now a script rather than an ad-hoc run —
`tools/playtest/scripts/saveload.txt` saves from the start menu, `reset`s, and
takes CONTINUE, landing back in New Bark Town on the release ROM.
