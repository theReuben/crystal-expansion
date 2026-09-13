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
