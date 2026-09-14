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
