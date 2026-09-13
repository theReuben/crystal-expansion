# Crystal Expansion

**Crystal Expansion is an unofficial, fan-made port of [Pokémon CrystalDust](https://github.com/Deokishisu/CrystalDust)'s content onto [RHH's pokeemerald-expansion](https://github.com/rh-hideout/pokeemerald-expansion). It is not affiliated with, endorsed by, or produced by the CrystalDust team, Rom Hacking Hideout, pret, Game Freak, or Nintendo.**

The goal is Pokémon Crystal — Johto, the Pokégear, the radio, apricorns, the day/night cycle — running on a modern Generation 3 engine, with pokeemerald-expansion's battle mechanics, ability and move coverage, and developer tooling underneath it.

This is a source-code project. **No built ROM is distributed, and none ever will be.** The assets in this repository derive from Nintendo's copyrighted work. Building it is for personal use only, and requires that you supply your own legally obtained game.

```
Based off RHH's pokeemerald-expansion 1.17.0 https://github.com/rh-hideout/pokeemerald-expansion/
```

## Status

Early. The project is in **Phase 2** of a seven-phase port; see [`docs/port/`](docs/port/) for the plan, the running [decision log](docs/port/decisions.md), and the current [build error queue](docs/build-errors-phase1.txt). It does not build yet, and it is not playable.

## Building

Requires devkitARM and **GNU Make 4.0 or newer**. On macOS, install `gmake` (`brew install make`) and use it — Apple's bundled `make` is version 3.81 and hangs on this project's rule set. See [decision D4](docs/port/decisions.md).

```sh
gmake -j8 modern
```

## Credits

See [CREDITS.md](CREDITS.md). In short: this project is other people's work, recombined. CrystalDust's team built the Crystal content; RHH built the expansion; pret made both possible by decompiling the games in the first place.

## Upstream documentation

pokeemerald-expansion's own README is preserved at [`docs/port/upstream-README-expansion.md`](docs/port/upstream-README-expansion.md), and its feature list at [`FEATURES.md`](FEATURES.md).
