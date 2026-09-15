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
