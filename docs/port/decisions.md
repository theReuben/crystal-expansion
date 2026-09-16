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
