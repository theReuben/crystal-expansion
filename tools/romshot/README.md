# romshot — headless ROM verification

Runs a GBA ROM with no window, optionally taps buttons, and writes a PNG of the
final frame. This is how build gates get verified without a human at a keyboard.

Homebrew's mGBA ships only the Qt app, so `libmgba` has to be built first:

```sh
git clone --depth 1 --branch 0.10.5 https://github.com/mgba-emu/mgba.git sources/mgba-src
cmake -S sources/mgba-src -B sources/mgba-build \
	-DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
	-DBUILD_QT=OFF -DBUILD_SDL=OFF -DBUILD_PERF=ON \
	-DUSE_DISCORD_RPC=OFF -DUSE_FFMPEG=OFF -DUSE_ELF=OFF
cmake --build sources/mgba-build -j8
tools/romshot/build.sh
```

(`-DCMAKE_POLICY_VERSION_MINIMUM=3.5` is needed because current CMake dropped
compatibility with the minimum mGBA 0.10.5 declares.)

## Usage

```sh
tools/romshot/romshot <rom> <out.png> <frames> [frame:keymask ...]
```

Each `frame:keymask` holds that key mask down for 6 frames starting at that
frame. Masks are the GBA bit order:

| A | B | Sel | Start | R | L | U | D | R-trig | L-trig |
|---|---|-----|-------|---|---|---|---|--------|--------|
| 1 | 2 | 4   | 8     |16 |32 |64 |128| 256    | 512    |

Example — boot past the intro to the title screen:

```sh
tools/romshot/romshot CrystalDust.gba title.png 1800 1500:8
```

mGBA logs a lot of routine BIOS/DMA chatter to stderr; redirect it.

`sources/mgba-build/mgba-perf` is also available (`-F frames -N`) for a pure
"does it run without dying" check with no image out.
